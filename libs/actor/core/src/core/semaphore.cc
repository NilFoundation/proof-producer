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

#include <fmt/format.h>

#include <nil/actor/core/semaphore.hh>

namespace nil {
    namespace actor {

        // Exception Factory for standard semaphore
        //
        // constructs standard semaphore exceptions
        // \see semaphore_timed_out and broken_semaphore

        static_assert(std::is_nothrow_default_constructible<semaphore_default_exception_factory>::value);
        static_assert(std::is_nothrow_move_constructible<semaphore_default_exception_factory>::value);

        static_assert(std::is_nothrow_constructible<semaphore, size_t>::value);
        static_assert(std::is_nothrow_constructible<semaphore, size_t, semaphore_default_exception_factory &&>::value);
        static_assert(std::is_nothrow_move_constructible<semaphore>::value);

        const char *broken_semaphore::what() const noexcept {
            return "Semaphore broken";
        }

        const char *semaphore_timed_out::what() const noexcept {
            return "Semaphore timedout";
        }

        semaphore_timed_out semaphore_default_exception_factory::timeout() noexcept {
            static_assert(std::is_nothrow_default_constructible_v<semaphore_timed_out>);
            return semaphore_timed_out();
        }

        broken_semaphore semaphore_default_exception_factory::broken() noexcept {
            static_assert(std::is_nothrow_default_constructible_v<broken_semaphore>);
            return broken_semaphore();
        }

        // A factory of semaphore exceptions that contain additional context: the semaphore name
        // auto sem = named_semaphore(0, named_semaphore_exception_factory{"file_opening_limit_semaphore"});

        static_assert(std::is_nothrow_default_constructible<named_semaphore_exception_factory>::value);
        static_assert(std::is_nothrow_move_constructible<named_semaphore_exception_factory>::value);

        static_assert(std::is_nothrow_constructible<named_semaphore, size_t>::value);
        static_assert(
            std::is_nothrow_constructible<named_semaphore, size_t, named_semaphore_exception_factory &&>::value);
        static_assert(std::is_nothrow_move_constructible<named_semaphore>::value);

        named_semaphore_timed_out::named_semaphore_timed_out(std::string_view msg) noexcept : _msg() {
            try {
                _msg = format("Semaphore timed out: {}", msg);
            } catch (...) {
                // ignore, empty _msg will generate a static message in what().
            }
        }

        broken_named_semaphore::broken_named_semaphore(std::string_view msg) noexcept : _msg() {
            try {
                _msg = format("Semaphore broken: {}", msg);
            } catch (...) {
                // ignore, empty _msg will generate a static message in what().
            }
        }

        const char *named_semaphore_timed_out::what() const noexcept {
            // return a static message if generating the dynamic message failed.
            return _msg.empty() ? "Named semaphore timed out" : _msg.c_str();
        }

        const char *broken_named_semaphore::what() const noexcept {
            // return a static message if generating the dynamic message failed.
            return _msg.empty() ? "Broken named semaphore" : _msg.c_str();
        }

        named_semaphore_timed_out named_semaphore_exception_factory::timeout() const noexcept {
            return named_semaphore_timed_out(name);
        }

        broken_named_semaphore named_semaphore_exception_factory::broken() const noexcept {
            return broken_named_semaphore(name);
        }

    }    // namespace actor
}    // namespace nil
