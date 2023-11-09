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

#include <nil/actor/core/on_internal_error.hh>
#include <nil/actor/detail/backtrace.hh>
#include <nil/actor/detail/log.hh>

#include <atomic>

static std::atomic<bool> abort_on_internal_error {false};

using namespace nil::actor;

void nil::actor::set_abort_on_internal_error(bool do_abort) {
    abort_on_internal_error.store(do_abort);
}

void nil::actor::on_internal_error(logger &logger, std::string_view msg) {
    if (abort_on_internal_error.load()) {
        logger.error("{}, at: {}", msg, current_backtrace());
        abort();
    } else {
        throw_with_backtrace<std::runtime_error>(std::string(msg));
    }
}

void nil::actor::on_internal_error(logger &logger, std::exception_ptr ex) {
    if (abort_on_internal_error.load()) {
        logger.error("{}", ex);
        abort();
    } else {
        std::rethrow_exception(std::move(ex));
    }
}

void nil::actor::on_internal_error_noexcept(logger &logger, std::string_view msg) noexcept {
    logger.error("{}, at: {}", msg, current_backtrace());
    if (abort_on_internal_error.load()) {
        abort();
    }
}
