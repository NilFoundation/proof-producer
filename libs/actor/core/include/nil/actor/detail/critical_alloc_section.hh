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

namespace nil {
    namespace actor {
        namespace memory {

            /// \cond internal
            namespace detail {

// This variable is used in hot paths so we want to avoid the compiler
// generating TLS init guards for it. In C++20 we have constinit to tell the
// compiler that it can be initialized compile time (although gcc still doesn't
// completely drops the init guards - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97848).
// In < c++20 we use `__thread` which results in no TLS init guards generated.
#ifdef __cpp_constinit
                extern thread_local constinit int critical_alloc_section;
#else
                extern __thread int critical_alloc_section;
#endif

            }    // namespace detail
            /// \endcond

            /// \brief Marks scopes that contain critical allocations.
            ///
            /// Critical allocations are those, whose failure the application cannot
            /// tolerate. In a perfect world, there should be no such allocation, but we
            /// don't live in a perfect world.
            /// This information is used by other parts of the memory subsystem:
            /// * \ref alloc_failure_injector will not inject errors into these scopes.
            /// * A memory diagnostics report will be dumped if an allocation fails in these
            ///   scopes when the memory diagnostics subsystem is configured to dump reports
            ///   for \ref alloc_failure_kind \ref alloc_failure_kind::critical or above.
            ///   See \ref set_dump_memory_diagnostics_on_alloc_failure_kind().
            class scoped_critical_alloc_section {
            public:
                scoped_critical_alloc_section() {
                    ++detail::critical_alloc_section;
                }
                ~scoped_critical_alloc_section() {
                    --detail::critical_alloc_section;
                }
            };

            /// \brief Is the current context inside a critical alloc section?
            ///
            /// Will return true if there is at least one \ref scoped_critical_alloc_section
            /// alive in the current scope or the scope of any of the caller functions.
            inline bool is_critical_alloc_section() {
                return bool(detail::critical_alloc_section);
            }

        }    // namespace memory
    }        // namespace actor
}    // namespace nil
