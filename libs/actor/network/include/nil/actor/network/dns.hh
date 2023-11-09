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

#include <vector>
#include <unordered_map>
#include <memory>

#include <nil/actor/detail/std-compat.hh>

#include <nil/actor/core/future.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/network/inet_address.hh>

namespace nil {
    namespace actor {

        struct ipv4_addr;

        class socket_address;
        class network_stack;

        /**
         * C-ares based dns query support.
         * Handles name- and ip-based resolution.
         *
         */

        namespace net {

            /**
             * A c++-esque version of a hostent
             */
            struct hostent {
                // Primary name is always first
                std::vector<sstring> names;
                // Primary address is also always first.
                std::vector<inet_address> addr_list;
            };

            typedef boost::optional<inet_address::family> opt_family;

            struct srv_record {
                unsigned short priority;
                unsigned short weight;
                unsigned short port;
                sstring target;
            };

            /**
             * A DNS resolver object.
             * Wraps the query logic & networking.
             * Can be instantiated with options and your network
             * stack of choice, though for "normal" non-test
             * querying, you are probably better of with the
             * global calls further down.
             */
            class dns_resolver {
            public:
                struct options {
                    boost::optional<bool> use_tcp_query;
                    boost::optional<std::vector<inet_address>> servers;
                    boost::optional<std::chrono::milliseconds> timeout;
                    boost::optional<uint16_t> tcp_port, udp_port;
                    boost::optional<std::vector<sstring>> domains;
                };

                enum class srv_proto { tcp, udp };
                using srv_records = std::vector<srv_record>;

                dns_resolver();
                dns_resolver(dns_resolver &&) noexcept;
                explicit dns_resolver(const options &);
                explicit dns_resolver(network_stack &, const options & = {});
                ~dns_resolver();

                dns_resolver &operator=(dns_resolver &&) noexcept;

                /**
                 * Resolves a hostname to one or more addresses and aliases
                 */
                future<hostent> get_host_by_name(const sstring &, opt_family = {});
                /**
                 * Resolves an address to one or more addresses and aliases
                 */
                future<hostent> get_host_by_addr(const inet_address &);

                /**
                 * Resolves a hostname to one (primary) address
                 */
                future<inet_address> resolve_name(const sstring &, opt_family = {});
                /**
                 * Resolves an address to one (primary) name
                 */
                future<sstring> resolve_addr(const inet_address &);

                /**
                 * Resolve a service in given domain to one or more SRV records
                 */
                future<srv_records> get_srv_records(srv_proto proto, const sstring &service, const sstring &domain);

                /**
                 * Shuts the object down. Great for tests.
                 */
                future<> close();

            private:
                class impl;
                shared_ptr<impl> _impl;
            };

            namespace dns {

                // See above. These functions simply queries using a shard-local
                // default-stack, default-opts resolver
                future<hostent> get_host_by_name(const sstring &, opt_family = {});
                future<hostent> get_host_by_addr(const inet_address &);

                future<inet_address> resolve_name(const sstring &, opt_family = {});
                future<sstring> resolve_addr(const inet_address &);

                future<std::vector<srv_record>> get_srv_records(dns_resolver::srv_proto proto, const sstring &service,
                                                                const sstring &domain);

            }    // namespace dns

        }    // namespace net

    }    // namespace actor
}    // namespace nil
