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

#include <nil/actor/network/ip_checksum.hh>
#include <nil/actor/network/net.hh>

#include <arpa/inet.h>

namespace nil {
    namespace actor {
        namespace net {
            void checksummer::sum(const char *data, size_t len) {
                auto orig_len = len;
                if (odd) {
                    csum += uint8_t(*data++);
                    --len;
                }
                auto p64 = reinterpret_cast<const packed<uint64_t> *>(data);
                while (len >= 8) {
                    csum += ntoh(*p64++);
                    len -= 8;
                }
                auto p16 = reinterpret_cast<const packed<uint16_t> *>(p64);
                while (len >= 2) {
                    csum += ntoh(*p16++);
                    len -= 2;
                }
                auto p8 = reinterpret_cast<const uint8_t *>(p16);
                if (len) {
                    csum += *p8++ << 8;
                    len -= 1;
                }
                odd ^= orig_len & 1;
            }

            uint16_t checksummer::get() const {
                __int128 csum1 = (csum & 0xffff'ffff'ffff'ffff) + (csum >> 64);
                uint64_t csum = (csum1 & 0xffff'ffff'ffff'ffff) + (csum1 >> 64);
                csum = (csum & 0xffff) + ((csum >> 16) & 0xffff) + ((csum >> 32) & 0xffff) + (csum >> 48);
                csum = (csum & 0xffff) + (csum >> 16);
                csum = (csum & 0xffff) + (csum >> 16);
                return hton(~csum);
            }

            void checksummer::sum(const packet &p) {
                for (auto &&f : p.fragments()) {
                    sum(f.base, f.size);
                }
            }

            uint16_t ip_checksum(const void *data, size_t len) {
                checksummer cksum;
                cksum.sum(reinterpret_cast<const char *>(data), len);
                return cksum.get();
            }
        }    // namespace net
    }        // namespace actor
}    // namespace nil
