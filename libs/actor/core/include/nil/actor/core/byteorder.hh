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

#include <algorithm>

#include <boost/endian/conversion.hpp>

#include <nil/actor/core/unaligned.hh>

namespace nil {
    namespace actor {
        template<typename T>
        inline T read_le(const char *p) noexcept {
            T datum;
            std::copy_n(p, sizeof(T), reinterpret_cast<char *>(&datum));
            return boost::endian::little_to_native(datum);
        }

        template<typename T>
        inline void write_le(char *p, T datum) noexcept {
            datum = boost::endian::native_to_little(datum);
            std::copy_n(reinterpret_cast<const char *>(&datum), sizeof(T), p);
        }

        template<typename T>
        inline T read_be(const char *p) noexcept {
            T datum;
            std::copy_n(p, sizeof(T), reinterpret_cast<char *>(&datum));
            return boost::endian::big_to_native(datum);
        }

        template<typename T>
        inline void write_be(char *p, T datum) noexcept {
            datum = boost::endian::big_to_native(datum);
            std::copy_n(reinterpret_cast<const char *>(&datum), sizeof(T), p);
        }

        template<typename T>
        inline T consume_be(const char *&p) noexcept {
            auto ret = read_be<T>(p);
            p += sizeof(T);
            return ret;
        }

        template<typename T>
        inline void produce_be(char *&p, T datum) noexcept {
            write_be<T>(p, datum);
            p += sizeof(T);
        }
    }    // namespace actor
}    // namespace nil
