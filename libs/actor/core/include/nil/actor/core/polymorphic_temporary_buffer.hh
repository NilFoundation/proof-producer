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

#include <boost/container/pmr/polymorphic_allocator.hpp>

#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        /// Creates a `temporary_buffer` allocated by a custom allocator
        ///
        /// \param allocator allocator to use when allocating the temporary_buffer
        /// \param size      size of the temporary buffer
        template<typename CharType>
        temporary_buffer<CharType>
            make_temporary_buffer(boost::container::pmr::polymorphic_allocator<CharType> *allocator, std::size_t size) {
            if (allocator == memory::malloc_allocator) {
                return temporary_buffer<CharType>(size);
            }
            CharType *buffer = allocator->allocate(size);
            return temporary_buffer<CharType>(
                buffer, size,
                make_deleter(deleter(), [allocator, buffer, size]() mutable { allocator->deallocate(buffer, size); }));
        }

    }    // namespace actor
}    // namespace nil
