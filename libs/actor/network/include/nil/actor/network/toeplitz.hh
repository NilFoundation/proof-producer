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

namespace nil {
    namespace actor {

        using rss_key_type = std::basic_string_view<uint8_t>;

        // Mellanox Linux's driver key
        static constexpr uint8_t default_rsskey_40bytes_v[] = {
            0xd1, 0x81, 0xc6, 0x2c, 0xf7, 0xf4, 0xdb, 0x5b, 0x19, 0x83, 0xa2, 0xfc, 0x94, 0x3e,
            0x1a, 0xdb, 0xd9, 0x38, 0x9e, 0x6b, 0xd1, 0x03, 0x9c, 0x2c, 0xa7, 0x44, 0x99, 0xad,
            0x59, 0x3d, 0x56, 0xd9, 0xf3, 0x25, 0x3c, 0x06, 0x2a, 0xdc, 0x1f, 0xfc};

        static constexpr rss_key_type default_rsskey_40bytes {default_rsskey_40bytes_v,
                                                              sizeof(default_rsskey_40bytes_v)};

        // Intel's i40e PMD default RSS key
        static constexpr uint8_t default_rsskey_52bytes_v[] = {
            0x44, 0x39, 0x79, 0x6b, 0xb5, 0x4c, 0x50, 0x23, 0xb6, 0x75, 0xea, 0x5b, 0x12, 0x4f, 0x9f, 0x30, 0xb8, 0xa2,
            0xc0, 0x3d, 0xdf, 0xdc, 0x4d, 0x02, 0xa0, 0x8c, 0x9b, 0x33, 0x4a, 0xf6, 0x4a, 0x4c, 0x05, 0xc6, 0xfa, 0x34,
            0x39, 0x58, 0xd8, 0x55, 0x7d, 0x99, 0x58, 0x3a, 0xe1, 0x38, 0xc9, 0x2e, 0x81, 0x15, 0x03, 0x66};

        static constexpr rss_key_type default_rsskey_52bytes {default_rsskey_52bytes_v,
                                                              sizeof(default_rsskey_52bytes_v)};

        template<typename T>
        static inline uint32_t toeplitz_hash(rss_key_type key, const T &data) {
            uint32_t hash = 0, v;
            u_int i, b;

            /* XXXRW: Perhaps an assertion about key length vs. data length? */

            v = (key[0] << 24) + (key[1] << 16) + (key[2] << 8) + key[3];
            for (i = 0; i < data.size(); i++) {
                for (b = 0; b < 8; b++) {
                    if (data[i] & (1 << (7 - b)))
                        hash ^= v;
                    v <<= 1;
                    if ((i + 4) < key.size() && (key[i + 4] & (1 << (7 - b))))
                        v |= 1;
                }
            }
            return (hash);
        }

    }    // namespace actor
}    // namespace nil
