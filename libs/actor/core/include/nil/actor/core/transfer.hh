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

// Helper functions for copying or moving multiple objects in an exception
// safe manner, then destroying the sources.
//
// To transfer, call transfer_pass1(allocator, &from, &to) on all object pairs,
// (this copies the object from @from to @to).  If no exceptions are encountered,
// call transfer_pass2(allocator, &from, &to).  This destroys the object at the
// origin.  If exceptions were encountered, simply destroy all copied objects.
//
// As an optimization, if the objects are moveable without throwing (noexcept)
// transfer_pass1() simply moves the objects and destroys the source, and
// transfer_pass2() does nothing.

#include <memory>
#include <type_traits>
#include <utility>

namespace nil {
    namespace actor {

        template<typename T, typename Alloc>
        inline void
            transfer_pass1(Alloc &a, T *from, T *to,
                           typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
            std::allocator_traits<Alloc>::construct(a, to, std::move(*from));
            std::allocator_traits<Alloc>::destroy(a, from);
        }

        template<typename T, typename Alloc>
        inline void
            transfer_pass2(Alloc &a, T *from, T *to,
                           typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
        }

        template<typename T, typename Alloc>
        inline void
            transfer_pass1(Alloc &a, T *from, T *to,
                           typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
            std::allocator_traits<Alloc>::construct(a, to, *from);
        }

        template<typename T, typename Alloc>
        inline void
            transfer_pass2(Alloc &a, T *from, T *to,
                           typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type * = nullptr) {
            std::allocator_traits<Alloc>::destroy(a, from);
        }

    }    // namespace actor
}    // namespace nil
