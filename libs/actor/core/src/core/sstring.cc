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

#include <nil/actor/core/sstring.hh>

using namespace nil::actor;

[[noreturn]] void detail::throw_bad_alloc() {
    throw std::bad_alloc();
}

[[noreturn]] void detail::throw_sstring_overflow() {
    throw std::overflow_error("sstring overflow");
}

[[noreturn]] void detail::throw_sstring_out_of_range() {
    throw std::out_of_range("sstring out of range");
}
