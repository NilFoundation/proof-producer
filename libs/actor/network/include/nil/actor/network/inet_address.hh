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

#include <iosfwd>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdexcept>
#include <vector>

#include <boost/optional.hpp>

#include <nil/actor/core/future.hh>
#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {
        namespace net {

            struct ipv4_address;
            struct ipv6_address;

            class unknown_host : public std::invalid_argument {
            public:
                using invalid_argument::invalid_argument;
            };

            class inet_address {
            public:
                enum class family : sa_family_t { INET = AF_INET, INET6 = AF_INET6 };

            private:
                family _in_family;

                union {
                    ::in_addr _in;
                    ::in6_addr _in6;
                };

                uint32_t _scope = invalid_scope;

            public:
                static constexpr uint32_t invalid_scope = std::numeric_limits<uint32_t>::max();

                inet_address() noexcept;
                inet_address(family) noexcept;
                inet_address(::in_addr i) noexcept;
                inet_address(::in6_addr i, uint32_t scope = invalid_scope) noexcept;
                // NOTE: does _not_ resolve the address. Only parses
                // ipv4/ipv6 numerical address
                // throws std::invalid_argument if sstring is invalid
                inet_address(const sstring &);
                inet_address(inet_address &&) noexcept = default;
                inet_address(const inet_address &) noexcept = default;

                inet_address(const ipv4_address &) noexcept;
                inet_address(const ipv6_address &, uint32_t scope = invalid_scope) noexcept;

                // throws iff ipv6
                ipv4_address as_ipv4_address() const;
                ipv6_address as_ipv6_address() const noexcept;

                inet_address &operator=(const inet_address &) noexcept = default;
                bool operator==(const inet_address &) const noexcept;

                family in_family() const noexcept {
                    return _in_family;
                }

                bool is_ipv6() const noexcept {
                    return _in_family == family::INET6;
                }
                bool is_ipv4() const noexcept {
                    return _in_family == family::INET;
                }

                size_t size() const noexcept;
                const void *data() const noexcept;

                uint32_t scope() const noexcept {
                    return _scope;
                }

                // throws iff ipv6
                operator ::in_addr() const;
                operator ::in6_addr() const noexcept;

                operator ipv6_address() const noexcept;

                future<sstring> hostname() const;
                future<std::vector<sstring>> aliases() const;

                static future<inet_address> find(const sstring &);
                static future<inet_address> find(const sstring &, family);
                static future<std::vector<inet_address>> find_all(const sstring &);
                static future<std::vector<inet_address>> find_all(const sstring &, family);

                static boost::optional<inet_address> parse_numerical(const sstring &);
            };

            std::ostream &operator<<(std::ostream &, const inet_address &);
            std::ostream &operator<<(std::ostream &, const inet_address::family &);

        }    // namespace net
    }        // namespace actor
}    // namespace nil

namespace std {
    template<>
    struct hash<nil::actor::net::inet_address> {
        size_t operator()(const nil::actor::net::inet_address &) const;
    };
}    // namespace std
