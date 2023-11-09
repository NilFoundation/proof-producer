//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
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

#pragma once

#include <boost/range/adaptor/filtered.hpp>
#include <nil/actor/core/scheduling.hh>
#include <nil/actor/core/map_reduce.hh>
#include <array>
#include <vector>

namespace nil {
    namespace actor {

        namespace detail {

            struct scheduling_group_specific_thread_local_data {
                struct per_scheduling_group {
                    bool queue_is_initialized = false;
                    /**
                     * This array holds pointers to the scheduling group specific
                     * data. The pointer is not use as is but is cast to a reference
                     * to the appropriate type that is actually pointed to.
                     */
                    std::vector<void *> specific_vals;
                };
                std::array<per_scheduling_group, max_scheduling_groups()> per_scheduling_group_data;
                std::vector<scheduling_group_key_config> scheduling_group_key_configs;
            };

            inline scheduling_group_specific_thread_local_data **
                get_scheduling_group_specific_thread_local_data_ptr() noexcept {
                static thread_local scheduling_group_specific_thread_local_data *data;
                return &data;
            }
            inline scheduling_group_specific_thread_local_data &
                get_scheduling_group_specific_thread_local_data() noexcept {
                return **get_scheduling_group_specific_thread_local_data_ptr();
            }

            [[noreturn]] void no_such_scheduling_group(scheduling_group sg);

            /**
             * Returns a pointer to the given scheduling group specific data.
             * @param sg - The scheduling group which it's data needs to be accessed
             * @param key - The scheduling group key that for the data to access
             * @return A pointer of type T* to the data, if sg is valid initialized.
             *
             * @note The parameter T has to be given since there is no way to deduce it.
             */
            template<typename T>
            T *scheduling_group_get_specific_ptr(scheduling_group sg, scheduling_group_key key) noexcept {
                auto &data = detail::get_scheduling_group_specific_thread_local_data();
#ifdef ACTOR_DEBUG
                assert(std::type_index(typeid(T)) == data.scheduling_group_key_configs[key.id()].type_index);
#endif
                auto sg_id = detail::scheduling_group_index(sg);
                if (__builtin_expect(sg_id < data.per_scheduling_group_data.size() &&
                                         data.per_scheduling_group_data[sg_id].queue_is_initialized,
                                     true)) {
                    return reinterpret_cast<T *>(data.per_scheduling_group_data[sg_id].specific_vals[key.id()]);
                }
                return nullptr;
            }

        }    // namespace detail

        /**
         * Returns a reference to the given scheduling group specific data.
         * @param sg - The scheduling group which it's data needs to be accessed
         * @param key - The scheduling group key that for the data to access
         * @return A reference of type T& to the data.
         *
         * @note The parameter T has to be given since there is no way to deduce it.
         *       May throw std::invalid_argument if sg does not exist or is uninitialized.
         */
        template<typename T>
        T &scheduling_group_get_specific(scheduling_group sg, scheduling_group_key key) {
            T *p = detail::scheduling_group_get_specific_ptr<T>(sg, std::move(key));
            if (!p) {
                detail::no_such_scheduling_group(sg);
            }
            return *p;
        }

        /**
         * Returns a reference to the current specific data.
         * @param key - The scheduling group key that for the data to access
         * @return A reference of type T& to the data.
         *
         * @note The parameter T has to be given since there is no way to deduce it.
         */
        template<typename T>
        T &scheduling_group_get_specific(scheduling_group_key key) noexcept {
            // Unlike detail::scheduling_group_get_specific_ptr, this can
            // return a reference to an element whose queue_is_initialized is
            // false.
            auto &data = detail::get_scheduling_group_specific_thread_local_data();
            assert(std::type_index(typeid(T)) == data.scheduling_group_key_configs[key.id()].type_index);
            auto sg_id = detail::scheduling_group_index(current_scheduling_group());
            return *reinterpret_cast<T *>(data.per_scheduling_group_data[sg_id].specific_vals[key.id()]);
        }

        /**
         * A map reduce over all values of a specific scheduling group data.
         * @param mapper -  A functor SomeType(SpecificValType&) or SomeType(SpecificValType) that maps
         * the specific data to a value of any type.
         * @param reducer - A functor of of type ConvetibleToInitial(Initial, MapperReurnType) that reduces
         * a value of type Initial and of the mapper return type to a value of type convertible to Initial.
         * @param initial_val - the initial value to pass in the first call to the reducer.
         * @param key - the key to the specific data that the mapper should act upon.
         * @return A future that resolves when the result of the map reduce is ready.
         * @note The type of SpecificValType must be given because there is no way to deduce it in a *consistent*
         * manner.
         * @note Theoretically the parameter type of Mapper can be deduced to be the type
         * (function_traits<Mapper>::arg<0>) but then there is a danger when the Mapper accepts a parameter type T where
         * SpecificValType is convertible to SpecificValType.
         */
        template<typename SpecificValType, typename Mapper, typename Reducer, typename Initial>
#ifdef BOOST_HAS_CONCEPTS
        requires requires(SpecificValType specific_val, Mapper mapper, Reducer reducer, Initial initial) {
            { reducer(initial, mapper(specific_val)) } -> std::convertible_to<Initial>;
        }
#endif
        future<typename function_traits<Reducer>::return_type>
            map_reduce_scheduling_group_specific(Mapper mapper, Reducer reducer, Initial initial_val,
                                                 scheduling_group_key key) {
            using per_scheduling_group = detail::scheduling_group_specific_thread_local_data::per_scheduling_group;
            auto &data = detail::get_scheduling_group_specific_thread_local_data();
            auto wrapped_mapper = [key, mapper](per_scheduling_group &psg) {
                auto id = detail::scheduling_group_key_id(key);
                return make_ready_future<typename function_traits<Mapper>::return_type>(
                    mapper(*reinterpret_cast<SpecificValType *>(psg.specific_vals[id])));
            };

            return map_reduce(data.per_scheduling_group_data |
                                  boost::adaptors::filtered(std::mem_fn(&per_scheduling_group::queue_is_initialized)),
                              wrapped_mapper, std::move(initial_val), reducer);
        }

        /**
         * A reduce over all values of a specific scheduling group data.
         * @param reducer - A functor of of type ConvetibleToInitial(Initial, SpecificValType) that reduces
         * a value of type Initial and of the sg specific data type to a value of type convertible to Initial.
         * @param initial_val - the initial value to pass in the first call to the reducer.
         * @param key - the key to the specific data that the mapper should act upon.
         * @return A future that resolves when the result of the reduce is ready.
         * * @note The type of SpecificValType must be given because there is no way to deduce it in a *consistent*
         * manner.
         * @note Theoretically the parameter type of Reducer can be deduced to be the type
         * (function_traits<Reducer>::arg<0>) but then there is a danger when the Reducer accepts a parameter type T
         * where SpecificValType is convertible to SpecificValType.
         */
        template<typename SpecificValType, typename Reducer, typename Initial>
#ifdef BOOST_HAS_CONCEPTS
        requires requires(SpecificValType specific_val, Reducer reducer, Initial initial) {
            { reducer(initial, specific_val) } -> std::convertible_to<Initial>;
        }
#endif
        future<typename function_traits<Reducer>::return_type>
            reduce_scheduling_group_specific(Reducer reducer, Initial initial_val, scheduling_group_key key) {
            using per_scheduling_group = detail::scheduling_group_specific_thread_local_data::per_scheduling_group;
            auto &data = detail::get_scheduling_group_specific_thread_local_data();

            auto mapper = [key](per_scheduling_group &psg) {
                auto id = detail::scheduling_group_key_id(key);
                return make_ready_future<SpecificValType>(*reinterpret_cast<SpecificValType *>(psg.specific_vals[id]));
            };

            return map_reduce(data.per_scheduling_group_data |
                                  boost::adaptors::filtered(std::mem_fn(&per_scheduling_group::queue_is_initialized)),
                              mapper, std::move(initial_val), reducer);
        }

    }    // namespace actor
}    // namespace nil
