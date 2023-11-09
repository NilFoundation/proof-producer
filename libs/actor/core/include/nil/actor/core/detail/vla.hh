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

#include <nil/actor/core/aligned_buffer.hh>

#include <memory>
#include <new>
#include <cassert>
#include <type_traits>

namespace nil {
    namespace actor {

        // Some C APIs have a structure with a variable length array at the end.
        // This is a helper function to help allocate it.
        //
        // for a structure
        //
        //   struct xx { int a; float b[0]; };
        //
        // use
        //
        //   make_struct_with_vla(&xx::b, number_of_bs);
        //
        // to allocate it.
        //
        template<class S, typename E>
        inline std::unique_ptr<S, free_deleter> make_struct_with_vla(E S::*last, size_t nr) {
            auto fake = reinterpret_cast<S *>(0);
            size_t offset = reinterpret_cast<uintptr_t>(&(fake->*last));
            size_t element_size = sizeof((fake->*last)[0]);
            assert(offset == sizeof(S));
            auto p =
                std::unique_ptr<char, free_deleter>(reinterpret_cast<char *>(::malloc(offset + element_size * nr)));
            auto s = std::unique_ptr<S, free_deleter>(new (p.get()) S());
            p.release();
            return s;
        }

    }    // namespace actor
}    // namespace nil
