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

#include <nil/actor/detail/noncopyable_function.hh>

namespace nil {
    namespace actor {
        namespace memory {

            /// \brief The kind of allocation failures to dump diagnostics report for.
            ///
            /// Note that if the actor_memory logger is set to level debug, there will
            /// be a report dumped for any allocation failure, regardless of this
            /// configuration.
            enum class alloc_failure_kind {
                /// Dump diagnostic error report for none of the allocation failures.
                none,
                /// Dump diagnostic error report for critical allocation failures, see
                /// \ref scoped_critical_alloc_section.
                critical,
                /// Dump diagnostic error report for all the allocation failures.
                all,
            };

            /// \brief Configure when memory diagnostics are dumped.
            ///
            /// See \ref alloc_failure_kind on available options.
            /// Applies configuration on all shards.
            void set_dump_memory_diagnostics_on_alloc_failure_kind(alloc_failure_kind);

            /// \brief Configure when memory diagnostics are dumped.
            ///
            /// String version. See \ref alloc_failure_kind on available options.
            /// Applies configuration on all shards.
            void set_dump_memory_diagnostics_on_alloc_failure_kind(std::string_view);

            /// \brief A functor which writes its argument into the diagnostics report.
            using memory_diagnostics_writer = noncopyable_function<void(std::string_view)>;

            /// \brief Set a producer of additional diagnostic information.
            ///
            /// This allows the application running on top of actor to add its own part to
            /// the diagnostics dump. The application can supply higher level diagnostics
            /// information, that might help explain how the memory was consumed.
            ///
            /// The application specific part will be added just below the main stats
            /// (free/used/total memory).
            ///
            /// \param producer - the functor to produce the additional diagnostics, specific
            ///     to the application, to be added to the generated report. The producer is
            ///     passed a writer functor, which it can use to add its parts to the report.
            ///
            /// \note As the report is generated at a time when allocations are failing, the
            ///     producer should try as hard as possible to not allocate while producing
            ///     the output.
            void set_additional_diagnostics_producer(noncopyable_function<void(memory_diagnostics_writer)> producer);

            /// Manually generate a diagnostics report
            ///
            /// Note that contrary to the automated report generation (triggered by
            /// allocation failure), this method does allocate memory and can fail in
            /// low-memory conditions.
            sstring generate_memory_diagnostics_report();

        }    // namespace memory
    }        // namespace actor
}    // namespace nil
