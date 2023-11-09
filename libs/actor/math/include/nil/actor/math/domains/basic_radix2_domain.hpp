//---------------------------------------------------------------------------//
// Copyright (c) 2020-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef ACTOR_MATH_BASIC_RADIX2_DOMAIN_HPP
#define ACTOR_MATH_BASIC_RADIX2_DOMAIN_HPP

#include <vector>

#include <nil/crypto3/math/detail/field_utils.hpp>
#include <nil/crypto3/math/algorithms/unity_root.hpp>

#include <nil/actor/math/domains/evaluation_domain.hpp>
#include <nil/actor/math/domains/detail/basic_radix2_domain_aux.hpp>

namespace nil {
    namespace actor {
        namespace math {

            using namespace nil::crypto3::algebra;

            template<typename FieldType, typename ValueType = typename FieldType::value_type>
            class basic_radix2_domain : public evaluation_domain<FieldType, ValueType> {
                typedef typename FieldType::value_type field_value_type;
                typedef ValueType value_type;

            public:
                typedef FieldType field_type;

                field_value_type omega;

                basic_radix2_domain(const std::size_t m) : evaluation_domain<FieldType, ValueType>(m) {
                    if (m <= 1)
                        throw std::invalid_argument("basic_radix2(): expected m > 1");

                    if (!std::is_same<field_value_type, std::complex<double>>::value) {
                        const std::size_t logm = static_cast<std::size_t>(std::ceil(std::log2(m)));
                        if (logm > (fields::arithmetic_params<FieldType>::s))
                            throw std::invalid_argument(
                                "basic_radix2(): expected logm <= fields::arithmetic_params<FieldType>::s");
                    }

                    omega = crypto3::math::unity_root<FieldType>(m);
                }

                future<> fft(std::vector<value_type> &a) {
                    if (a.size() != this->m) {
                        if (a.size() < this->m) {
                            a.resize(this->m, value_type::zero());
                        } else {
                            throw std::invalid_argument("basic_radix2: expected a.size() == this->m");
                        }
                    }

                    detail::basic_radix2_fft<FieldType>(a, omega).get();
                    return make_ready_future<>();
                }

                future<> inverse_fft(std::vector<value_type> &a) {
                    if (a.size() != this->m) {
                        if (a.size() < this->m) {
                            a.resize(this->m, value_type::zero());
                        } else {
                            throw std::invalid_argument("basic_radix2: expected a.size() == this->m");
                        }
                    }

                    detail::basic_radix2_fft<FieldType>(a, omega.inversed()).get();

                    const field_value_type sconst = field_value_type(a.size()).inversed();

                    detail::block_execution(this->m, smp::count, [sconst, &a](std::size_t begin, std::size_t end) {
                        for (std::size_t i = begin; i < end; i++) {
                            a[i] = a[i] * sconst;
                        }
                    }).get();

                    return make_ready_future<>();
                }

                future<std::vector<field_value_type>> evaluate_all_lagrange_polynomials(const field_value_type &t) {
                    return make_ready_future<std::vector<field_value_type>>(detail::basic_radix2_evaluate_all_lagrange_polynomials<FieldType>(this->m, t));
                }

                future<std::vector<value_type>> evaluate_all_lagrange_polynomials(const typename std::vector<value_type>::const_iterator &t_powers_begin,
                                                                          const typename std::vector<value_type>::const_iterator &t_powers_end) {
                    if (std::distance(t_powers_begin, t_powers_end) < this->m) {
                        throw std::invalid_argument("basic_radix2: expected std::distance(t_powers_begin, t_powers_end) >= this->m");
                    }
                    std::vector<value_type> tmp(t_powers_begin, t_powers_begin + this->m);
                    this->inverse_fft(tmp).get();
                    return make_ready_future<std::vector<value_type>>(tmp);
                }

                field_value_type get_domain_element(const std::size_t idx) {
                    return omega.pow(idx);
                }

                field_value_type compute_vanishing_polynomial(const field_value_type &t) {
                    return (t.pow(this->m)) - field_value_type::one();
                }

                polynomial<field_value_type> get_vanishing_polynomial() {
                    polynomial<field_value_type> z(this->m + 1, field_value_type::zero());
                    z[this->m] = field_value_type::one();
                    z[0] = -field_value_type::one();
                    return z;
                }

                future<> add_poly_z(const field_value_type &coeff, std::vector<field_value_type> &H) {
                    if (H.size() != this->m + 1)
                        throw std::invalid_argument("basic_radix2: expected H.size() == this->m+1");

                    H[this->m] += coeff;
                    H[0] -= coeff;
                    return make_ready_future<>();
                }

                future<> divide_by_z_on_coset(std::vector<field_value_type> &P) {
                    const field_value_type coset = fields::arithmetic_params<FieldType>::multiplicative_generator;
                    const field_value_type Z_inverse_at_coset = this->compute_vanishing_polynomial(coset).inversed();

                    detail::block_execution(this->m, smp::count, [&P, &Z_inverse_at_coset](std::size_t begin, std::size_t end) {
                        for (std::size_t i = begin; i < end; i++) {
                            P[i] *= Z_inverse_at_coset;
                        }
                    }).get();

                    return make_ready_future<>();
                }

                bool operator==(const basic_radix2_domain &rhs) const {
                    return isEqual(rhs) && omega == rhs.omega;
                }

                bool operator!=(const basic_radix2_domain &rhs) const {
                    return !(*this == rhs);
                }
            };
        }    // namespace math
    }        // namespace actor
}    // namespace nil

#endif    // ALGEBRA_FFT_BASIC_RADIX2_DOMAIN_HPP
