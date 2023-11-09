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

#include <nil/actor/network/packet.hh>
#include <cstdint>
#include <cstddef>
#include <arpa/inet.h>

namespace nil {
    namespace actor {
        namespace net {

            uint16_t ip_checksum(const void *data, size_t len);

            struct checksummer {
                __int128 csum = 0;
                bool odd = false;
                void sum(const char *data, size_t len);
                void sum(const packet &p);
                void sum(uint8_t data) {
                    if (!odd) {
                        csum += data << 8;
                    } else {
                        csum += data;
                    }
                    odd = !odd;
                }
                void sum(uint16_t data) {
                    if (odd) {
                        sum(uint8_t(data >> 8));
                        sum(uint8_t(data));
                    } else {
                        csum += data;
                    }
                }
                void sum(uint32_t data) {
                    if (odd) {
                        sum(uint16_t(data));
                        sum(uint16_t(data >> 16));
                    } else {
                        csum += data;
                    }
                }
                void sum_many() {
                }
                template<typename T0, typename... T>
                void sum_many(T0 data, T... rest) {
                    sum(data);
                    sum_many(rest...);
                }
                uint16_t get() const;
            };
        }    // namespace net
    }    // namespace actor
}    // namespace nil
