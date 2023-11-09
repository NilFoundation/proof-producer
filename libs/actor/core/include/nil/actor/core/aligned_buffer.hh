//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#pragma once
#include <cstdlib>
#include <memory>
#include <stdexcept>

namespace nil {
    namespace actor {

        namespace detail {
            void *allocate_aligned_buffer_impl(size_t size, size_t align);
        }

        struct free_deleter {
            void operator()(void *p) {
                ::free(p);
            }
        };

        template<typename CharType>
        inline std::unique_ptr<CharType[], free_deleter> allocate_aligned_buffer(size_t size, size_t align) {
            static_assert(sizeof(CharType) == 1, "must allocate byte type");
            void *ret = detail::allocate_aligned_buffer_impl(size, align);
            return std::unique_ptr<CharType[], free_deleter>(reinterpret_cast<CharType *>(ret));
        }

    }    // namespace actor
}    // namespace nil
