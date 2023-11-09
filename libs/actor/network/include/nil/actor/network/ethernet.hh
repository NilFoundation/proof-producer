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

#include <array>
#include <cassert>
#include <algorithm>

#include <nil/actor/network/byteorder.hh>

namespace nil {
    namespace actor {

        namespace net {

            struct ethernet_address {
                ethernet_address() : mac {} {
                }

                ethernet_address(const uint8_t *eaddr) {
                    std::copy(eaddr, eaddr + 6, mac.begin());
                }

                ethernet_address(std::initializer_list<uint8_t> eaddr) {
                    assert(eaddr.size() == mac.size());
                    std::copy(eaddr.begin(), eaddr.end(), mac.begin());
                }

                std::array<uint8_t, 6> mac;

                template<typename Adjuster>
                void adjust_endianness(Adjuster a) {
                }

                static ethernet_address read(const char *p) {
                    ethernet_address ea;
                    std::copy_n(p, size(), reinterpret_cast<char *>(ea.mac.data()));
                    return ea;
                }
                static ethernet_address consume(const char *&p) {
                    auto ea = read(p);
                    p += size();
                    return ea;
                }
                void write(char *p) const {
                    std::copy_n(reinterpret_cast<const char *>(mac.data()), size(), p);
                }
                void produce(char *&p) const {
                    write(p);
                    p += size();
                }
                static constexpr size_t size() {
                    return 6;
                }
            } __attribute__((packed));

            std::ostream &operator<<(std::ostream &os, ethernet_address ea);

            struct ethernet {
                using address = ethernet_address;
                static address broadcast_address() {
                    return {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
                }
                static constexpr uint16_t arp_hardware_type() {
                    return 1;
                }
            };

            struct eth_hdr {
                ethernet_address dst_mac;
                ethernet_address src_mac;
                packed<uint16_t> eth_proto;
                template<typename Adjuster>
                auto adjust_endianness(Adjuster a) {
                    return a(eth_proto);
                }
            } __attribute__((packed));

            ethernet_address parse_ethernet_address(std::string addr);
        }    // namespace net

    }    // namespace actor
}    // namespace nil
