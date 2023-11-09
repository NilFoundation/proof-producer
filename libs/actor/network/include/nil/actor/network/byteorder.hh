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

#include <arpa/inet.h>    // for ntohs() and friends

#include <iosfwd>
#include <utility>

#include <nil/actor/core/unaligned.hh>

namespace nil {
    namespace actor {

        inline uint64_t ntohq(uint64_t v) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            // big endian, nothing to do
            return v;
#else
            // little endian, reverse bytes
            return __builtin_bswap64(v);
#endif
        }

        inline uint64_t htonq(uint64_t v) {
            // htonq and ntohq have identical implementations
            return ntohq(v);
        }

        namespace net {

            inline void ntoh() {
            }
            inline void hton() {
            }

            inline uint8_t ntoh(uint8_t x) {
                return x;
            }
            inline uint8_t hton(uint8_t x) {
                return x;
            }
            inline uint16_t ntoh(uint16_t x) {
                return ntohs(x);
            }
            inline uint16_t hton(uint16_t x) {
                return htons(x);
            }
            inline uint32_t ntoh(uint32_t x) {
                return ntohl(x);
            }
            inline uint32_t hton(uint32_t x) {
                return htonl(x);
            }
            inline uint64_t ntoh(uint64_t x) {
                return ntohq(x);
            }
            inline uint64_t hton(uint64_t x) {
                return htonq(x);
            }

            inline int8_t ntoh(int8_t x) {
                return x;
            }
            inline int8_t hton(int8_t x) {
                return x;
            }
            inline int16_t ntoh(int16_t x) {
                return ntohs(x);
            }
            inline int16_t hton(int16_t x) {
                return htons(x);
            }
            inline int32_t ntoh(int32_t x) {
                return ntohl(x);
            }
            inline int32_t hton(int32_t x) {
                return htonl(x);
            }
            inline int64_t ntoh(int64_t x) {
                return ntohq(x);
            }
            inline int64_t hton(int64_t x) {
                return htonq(x);
            }

            // Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
            // TODO: get rid of this alias.
            template<typename T>
            using packed = unaligned<T>;

            template<typename T>
            inline T ntoh(const packed<T> &x) {
                T v = x;
                return ntoh(v);
            }

            template<typename T>
            inline T hton(const packed<T> &x) {
                T v = x;
                return hton(v);
            }

            template<typename T>
            inline std::ostream &operator<<(std::ostream &os, const packed<T> &v) {
                auto x = v.raw;
                return os << x;
            }

            inline void ntoh_inplace() {
            }
            inline void hton_inplace() {};

            template<typename First, typename... Rest>
            inline void ntoh_inplace(First &first, Rest &...rest) {
                first = ntoh(first);
                ntoh_inplace(std::forward<Rest &>(rest)...);
            }

            template<typename First, typename... Rest>
            inline void hton_inplace(First &first, Rest &...rest) {
                first = hton(first);
                hton_inplace(std::forward<Rest &>(rest)...);
            }

            template<class T>
            inline T ntoh(const T &x) {
                T tmp = x;
                tmp.adjust_endianness([](auto &&...what) { ntoh_inplace(std::forward<decltype(what) &>(what)...); });
                return tmp;
            }

            template<class T>
            inline T hton(const T &x) {
                T tmp = x;
                tmp.adjust_endianness([](auto &&...what) { hton_inplace(std::forward<decltype(what) &>(what)...); });
                return tmp;
            }
        }    // namespace net
    }    // namespace actor
}    // namespace nil
