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

namespace nil {
    namespace actor {

        namespace net {

            enum class ip_protocol_num : uint8_t { icmp = 1, tcp = 6, udp = 17, unused = 255 };

            enum class eth_protocol_num : uint16_t { ipv4 = 0x0800, arp = 0x0806, ipv6 = 0x86dd };

            const uint8_t eth_hdr_len = 14;
            const uint8_t tcp_hdr_len_min = 20;
            const uint8_t ipv4_hdr_len_min = 20;
            const uint8_t ipv6_hdr_len_min = 40;
            const uint16_t ip_packet_len_max = 65535;

        }    // namespace net

    }    // namespace actor
}    // namespace nil
