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

#include <cstdio>

namespace nil {
    namespace actor {

        //
        // Collection of async-signal safe printing functions.
        //

        // Outputs string to stderr.
        // Async-signal safe.
        inline void print_safe(const char *str, size_t len) noexcept {
            while (len) {
                auto result = write(STDERR_FILENO, str, len);
                if (result > 0) {
                    len -= result;
                    str += result;
                } else if (result == 0) {
                    break;
                } else {
                    if (errno == EINTR) {
                        // retry
                    } else {
                        break;    // what can we do?
                    }
                }
            }
        }

        // Outputs string to stderr.
        // Async-signal safe.
        inline void print_safe(const char *str) noexcept {
            print_safe(str, strlen(str));
        }

        // Fills a buffer with a zero-padded hexadecimal representation of an integer.
        // For example, convert_zero_padded_hex_safe(buf, 4, uint16_t(12)) fills the buffer with "000c".
        template<typename Integral>
        void convert_zero_padded_hex_safe(char *buf, size_t bufsz, Integral n) noexcept {
            const char *digits = "0123456789abcdef";
            memset(buf, '0', bufsz);
            unsigned i = bufsz;
            while (n) {
                buf[--i] = digits[n & 0xf];
                n >>= 4;
            }
        }

        // Prints zero-padded hexadecimal representation of an integer to stderr.
        // For example, print_zero_padded_hex_safe(uint16_t(12)) prints "000c".
        // Async-signal safe.
        template<typename Integral>
        void print_zero_padded_hex_safe(Integral n) noexcept {
            static_assert(std::is_integral<Integral>::value && !std::is_signed<Integral>::value,
                          "Requires unsigned integrals");

            char buf[sizeof(n) * 2];
            convert_zero_padded_hex_safe(buf, sizeof(buf), n);
            print_safe(buf, sizeof(buf));
        }

        // Fills a buffer with a decimal representation of an integer.
        // The argument bufsz is the maximum size of the buffer.
        // For example, print_decimal_safe(buf, 16, 12) prints "12".
        template<typename Integral>
        size_t convert_decimal_safe(char *buf, size_t bufsz, Integral n) noexcept {
            static_assert(std::is_integral<Integral>::value && !std::is_signed<Integral>::value,
                          "Requires unsigned integrals");

            char tmp[sizeof(n) * 3];
            unsigned i = bufsz;
            do {
                tmp[--i] = '0' + n % 10;
                n /= 10;
            } while (n);
            memcpy(buf, tmp + i, sizeof(tmp) - i);
            return sizeof(tmp) - i;
        }

        // Prints decimal representation of an integer to stderr.
        // For example, print_decimal_safe(12) prints "12".
        // Async-signal safe.
        template<typename Integral>
        void print_decimal_safe(Integral n) noexcept {
            char buf[sizeof(n) * 3];
            unsigned i = sizeof(buf);
            auto len = convert_decimal_safe(buf, i, n);
            print_safe(buf, len);
        }

    }    // namespace actor
}    // namespace nil
