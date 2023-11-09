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

#include <chrono>

namespace nil {
    namespace actor {

        /// Configuration structure for reactor
        ///
        /// This structure provides configuration items for the reactor. It is typically
        /// provided by \ref app_template, not the user.
        struct reactor_config {
            std::chrono::duration<double> task_quota {0.5e-3};    ///< default time between polls
            /// \brief Handle SIGINT/SIGTERM by calling reactor::stop()
            ///
            /// When true, =nil; Actor will set up signal handlers for SIGINT/SIGTERM that call
            /// reactor::stop(). The reactor will then execute callbacks installed by
            /// reactor::at_exit().
            ///
            /// When false, =nil; Actor will not set up signal handlers for SIGINT/SIGTERM
            /// automatically. The default behavior (terminate the program) will be kept.
            /// You can adjust the behavior of SIGINT/SIGTERM by installing signal handlers
            /// via reactor::handle_signal().
            bool auto_handle_sigint_sigterm = true;    ///< automatically terminate on SIGINT/SIGTERM
        };

    }    // namespace actor
}    // namespace nil
