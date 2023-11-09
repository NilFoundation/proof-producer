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

#include <nil/actor/core/condition_variable.hh>

namespace nil {
    namespace actor {

        const char *broken_condition_variable::what() const noexcept {
            return "Condition variable is broken";
        }

        const char *condition_variable_timed_out::what() const noexcept {
            return "Condition variable timed out";
        }

        condition_variable_timed_out condition_variable::condition_variable_exception_factory::timeout() noexcept {
            static_assert(std::is_nothrow_default_constructible_v<condition_variable_timed_out>);
            return condition_variable_timed_out();
        }

        broken_condition_variable condition_variable::condition_variable_exception_factory::broken() noexcept {
            static_assert(std::is_nothrow_default_constructible_v<broken_condition_variable>);
            return broken_condition_variable();
        }

    }    // namespace actor
}    // namespace nil
