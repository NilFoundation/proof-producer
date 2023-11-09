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

#include <nil/actor/core/print.hh>
#include <nil/actor/network/ethernet.hh>

#include <boost/algorithm/string.hpp>
#include <string>

namespace nil {
    namespace actor {

        namespace net {

            std::ostream &operator<<(std::ostream &os, ethernet_address ea) {
                auto &m = ea.mac;
                using u = uint32_t;
                return fmt_print(os, "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", u(m[0]), u(m[1]), u(m[2]), u(m[3]),
                                 u(m[4]), u(m[5]));
            }

            ethernet_address parse_ethernet_address(std::string addr) {
                std::vector<std::string> v;
                boost::split(v, addr, boost::algorithm::is_any_of(":"));

                if (v.size() != 6) {
                    throw std::runtime_error("invalid mac address\n");
                }

                ethernet_address a;
                unsigned i = 0;
                for (auto &x : v) {
                    a.mac[i++] = std::stoi(x, nullptr, 16);
                }
                return a;
            }
        }    // namespace net

    }    // namespace actor
}    // namespace nil
