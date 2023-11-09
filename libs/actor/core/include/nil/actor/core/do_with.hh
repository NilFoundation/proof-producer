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

#include <utility>
#include <memory>
#include <tuple>

namespace nil {
    namespace actor {

        /// \cond internal

        namespace detail {

            template<typename HeldState, typename Future>
            class do_with_state final : public continuation_base_from_future<Future>::type {
                HeldState _held;
                typename Future::promise_type _pr;

            public:
                template<typename... T>
                explicit do_with_state(T &&...args) : _held(std::forward<T>(args)...) {
                }
                virtual void run_and_dispose() noexcept override {
                    _pr.set_urgent_state(std::move(this->_state));
                    delete this;
                }
                task *waiting_task() noexcept override {
                    return _pr.waiting_task();
                }
                HeldState &data() {
                    return _held;
                }
                Future get_future() {
                    return _pr.get_future();
                }
            };

        }    // namespace detail
        /// \endcond

        namespace detail {
            template<typename Tuple, size_t... Idx>
            inline auto cherry_pick_tuple(std::index_sequence<Idx...>, Tuple &&tuple) {
                return std::forward_as_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
            }

            template<typename Tuple, typename Seq>
            struct subtuple;

            template<typename Tuple, size_t... Idx>
            struct subtuple<Tuple, std::index_sequence<Idx...>> {
                using type = std::tuple<std::decay_t<std::tuple_element_t<Idx, Tuple>>...>;
            };

            template<typename T1, typename T2, typename... More>
            inline auto do_with_impl(T1 &&rv1, T2 &&rv2, More &&...more) {
                auto all =
                    std::forward_as_tuple(std::forward<T1>(rv1), std::forward<T2>(rv2), std::forward<More>(more)...);
                constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
                using idx = std::make_index_sequence<nr>;
                auto &&just_values = cherry_pick_tuple(idx(), std::move(all));
                auto &&just_func = std::move(std::get<nr>(std::move(all)));
                using value_tuple = typename subtuple<decltype(all), idx>::type;
                using ret_type = decltype(std::apply(just_func, std::declval<value_tuple &>()));
                auto task = std::apply(
                    [](auto &&...x) {
                        return std::make_unique<detail::do_with_state<value_tuple, ret_type>>(
                            std::forward<decltype(x)>(x)...);
                    },
                    std::move(just_values));
                auto fut = std::apply(just_func, task->data());
                if (fut.available()) {
                    return fut;
                }
                auto ret = task->get_future();
                detail::set_callback(fut, task.release());
                return ret;
            }
        }    // namespace detail

        /// \addtogroup future-util
        /// @{

        /// do_with() holds a objects alive until a future completes, and
        /// allow the code involved in making the future complete to have easy
        /// access to this object.
        ///
        /// do_with() takes multiple arguments: The last is a function
        /// returning a future. The other are temporary objects (rvalue). The
        /// function is given (a moved copy of) these temporary object, by
        /// reference, and it is ensured that the objects will not be
        /// destructed until the completion of the future returned by the
        /// function.
        ///
        /// do_with() returns a future which resolves to whatever value the given future
        /// (returned by the given function) resolves to. This returned value must not
        /// contain references to the temporary object, as at that point the temporary
        /// is destructed.
        ///
        /// \return whatever the function returns
        template<typename T1, typename T2, typename... More>
        inline auto do_with(T1 &&rv1, T2 &&rv2, More &&...more) noexcept {
            auto func = detail::do_with_impl<T1, T2, More...>;
            return futurize_invoke(func, std::forward<T1>(rv1), std::forward<T2>(rv2), std::forward<More>(more)...);
        }

        /// Executes the function \c func making sure the lock \c lock is taken,
        /// and later on properly released.
        ///
        /// \param lock the lock, which is any object having providing a lock() / unlock() semantics.
        ///        Caller must make sure that it outlives \c func.
        /// \param func function to be executed
        /// \returns whatever \c func returns
        template<typename Lock, typename Func>
        inline auto with_lock(Lock &lock, Func &&func) {
            return lock.lock().then([&lock, func = std::forward<Func>(func)]() mutable {
                return futurize_invoke(func).finally([&lock] { lock.unlock(); });
            });
        }

        /// @}

    }    // namespace actor
}    // namespace nil
