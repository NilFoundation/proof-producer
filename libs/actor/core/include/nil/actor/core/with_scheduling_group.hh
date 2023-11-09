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

#include <nil/actor/core/future.hh>
#include <nil/actor/core/make_task.hh>

#include <type_traits>

namespace nil {
    namespace actor {

        /// \addtogroup future-util
        /// @{

        namespace detail {

            template<typename Func>
#ifdef BOOST_HAS_CONCEPTS
            requires std::is_nothrow_move_constructible<Func>::value
#endif
                auto
                schedule_in_group(scheduling_group sg, Func func) noexcept {
                static_assert(std::is_nothrow_move_constructible<Func>::value);
                auto tsk = make_task(sg, std::move(func));
                schedule(tsk);
                return tsk->get_future();
            }
        }    // namespace detail

        /// \brief run a callable (with some arbitrary arguments) in a scheduling group
        ///
        /// If the conditions are suitable (see scheduling_group::may_run_immediately()),
        /// then the function is run immediately. Otherwise, the function is queued to run
        /// when its scheduling group next runs.
        ///
        /// \param sg  scheduling group that controls execution time for the function
        /// \param func function to run; must be movable or copyable
        /// \param args arguments to the function; may be copied or moved, so use \c std::ref()
        ///             to force passing references
        template<typename Func, typename... Args>
#ifdef BOOST_HAS_CONCEPTS
        requires std::is_nothrow_move_constructible<Func>::value
#endif
            inline auto
            with_scheduling_group(scheduling_group sg, Func func, Args &&...args) noexcept {
            static_assert(std::is_nothrow_move_constructible<Func>::value);
            using return_type = decltype(func(std::forward<Args>(args)...));
            using futurator = futurize<return_type>;
            if (sg.active()) {
                return futurator::invoke(func, std::forward<Args>(args)...);
            } else {
                return detail::schedule_in_group(
                    sg, [func = std::move(func), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                        return futurator::apply(func, std::move(args));
                    });
            }
        }

        /// @}

    }    // namespace actor
}    // namespace nil
