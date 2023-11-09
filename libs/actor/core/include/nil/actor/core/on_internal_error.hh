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

#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        class logger;

        /// Controls whether on_internal_error() aborts or throws. The default
        /// is to throw.
        void set_abort_on_internal_error(bool do_abort);

        /// Report an internal error
        ///
        /// Depending on the value passed to set_abort_on_internal_error, this
        /// will either log to \p logger and abort or throw a std::runtime_error.
        [[noreturn]] void on_internal_error(logger &logger, std::string_view reason);

        /// Report an internal error
        ///
        /// Depending on the value passed to set_abort_on_internal_error, this
        /// will either log to \p logger and abort or throw the passed-in
        /// \p ex.
        /// This overload cannot attach a backtrace to the exception, so if the
        /// caller wishes to have one attached they have to do it themselves.
        [[noreturn]] void on_internal_error(logger &logger, std::exception_ptr ex);

        /// Report an internal error in a noexcept context
        ///
        /// The error will be logged to \logger and if set_abort_on_internal_error,
        /// was set to true, the program will be aborted. This overload can be used
        /// in noexcept contexts like destructors or noexcept functions.
        void on_internal_error_noexcept(logger &logger, std::string_view reason) noexcept;

    }    // namespace actor
}    // namespace nil
