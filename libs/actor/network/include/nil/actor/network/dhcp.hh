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

#include <nil/actor/network/ip.hh>
#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        namespace net {

            /*
             * Simplistic DHCP query class.
             * Due to the nature of the native stack,
             * it operates on an "ipv4" object instead of,
             * for example, an interface.
             */
            class dhcp {
            public:
                dhcp(ipv4 &);
                dhcp(dhcp &&) noexcept;
                ~dhcp();

                static const steady_clock_type::duration default_timeout;

                struct lease {
                    ipv4_address ip;
                    ipv4_address netmask;
                    ipv4_address broadcast;

                    ipv4_address gateway;
                    ipv4_address dhcp_server;

                    std::vector<ipv4_address> name_servers;

                    std::chrono::seconds lease_time;
                    std::chrono::seconds renew_time;
                    std::chrono::seconds rebind_time;

                    uint16_t mtu = 0;
                };

                typedef future<boost::optional<lease>> result_type;

                /**
                 * Runs a discover/request sequence on the ipv4 "stack".
                 * During this execution the ipv4 will be "hijacked"
                 * more or less (through packet filter), and while not
                 * inoperable, most likely quite less efficient.
                 *
                 * Please note that this does _not_ modify the ipv4 object bound.
                 * It only makes queries and records replys for the related NIC.
                 * It is up to caller to use the returned information as he se fit.
                 */
                result_type discover(const steady_clock_type::duration & = default_timeout);
                result_type renew(const lease &, const steady_clock_type::duration & = default_timeout);
                ip_packet_filter *get_ipv4_filter();

            private:
                class impl;
                std::unique_ptr<impl> _impl;
            };
        }    // namespace net
    }        // namespace actor
}    // namespace nil
