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

#ifndef ACTOR_COROUTINES_ENABLED
#error Coroutines support disabled.
#endif

#include <nil/actor/core/std-coroutine.hh>

namespace nil {
    namespace actor {

        namespace detail {

            template<typename T = void>
            class coroutine_traits_base {
            public:
                class promise_type final : public nil::actor::task {
                    nil::actor::promise<T> _promise;

                public:
                    promise_type() = default;
                    promise_type(promise_type &&) = delete;
                    promise_type(const promise_type &) = delete;

                    template<typename... U>
                    void return_value(U &&...value) {
                        _promise.set_value(std::forward<U>(value)...);
                    }

                    [[deprecated(
                        "Forwarding coroutine returns are deprecated as too dangerous. Use 'co_return co_await ...' "
                        "until "
                        "explicit syntax is available.")]] void
                        return_value(future<T> &&fut) noexcept {
                        fut.forward_to(std::move(_promise));
                    }

                    void unhandled_exception() noexcept {
                        _promise.set_exception(std::current_exception());
                    }

                    nil::actor::future<T> get_return_object() noexcept {
                        return _promise.get_future();
                    }

                    ACTOR_INTERNAL_COROUTINE_NAMESPACE::suspend_never initial_suspend() noexcept {
                        return {};
                    }
                    ACTOR_INTERNAL_COROUTINE_NAMESPACE::suspend_never final_suspend() noexcept {
                        return {};
                    }

                    virtual void run_and_dispose() noexcept override {
                        auto handle =
                            ACTOR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<promise_type>::from_promise(*this);
                        handle.resume();
                    }

                    task *waiting_task() noexcept override {
                        return _promise.waiting_task();
                    }
                };
            };

            template<>
            class coroutine_traits_base<> {
            public:
                class promise_type final : public nil::actor::task {
                    nil::actor::promise<> _promise;

                public:
                    promise_type() = default;
                    promise_type(promise_type &&) = delete;
                    promise_type(const promise_type &) = delete;

                    void return_void() noexcept {
                        _promise.set_value();
                    }

// Only accepted in gcc 10; the standard says it is illegal
#if !defined(__clang__) && __GNUC__ < 11
                    [[deprecated(
                        "forwarding a future<> is not possible in standard C++. 'Use co_return co_wait ...' "
                        "instead.")]] void
                        return_value(future<> &&fut) noexcept {
                        fut.forward_to(std::move(_promise));
                    }
#endif

                    void unhandled_exception() noexcept {
                        _promise.set_exception(std::current_exception());
                    }

                    nil::actor::future<> get_return_object() noexcept {
                        return _promise.get_future();
                    }

                    ACTOR_INTERNAL_COROUTINE_NAMESPACE::suspend_never initial_suspend() noexcept {
                        return {};
                    }
                    ACTOR_INTERNAL_COROUTINE_NAMESPACE::suspend_never final_suspend() noexcept {
                        return {};
                    }

                    virtual void run_and_dispose() noexcept override {
                        auto handle =
                            ACTOR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<promise_type>::from_promise(*this);
                        handle.resume();
                    }

                    task *waiting_task() noexcept override {
                        return _promise.waiting_task();
                    }
                };
            };

            template<typename... T>
            struct awaiter {
                nil::actor::future<T...> _future;

            public:
                explicit awaiter(nil::actor::future<T...> &&f) noexcept : _future(std::move(f)) {
                }

                awaiter(const awaiter &) = delete;
                awaiter(awaiter &&) = delete;

                bool await_ready() const noexcept {
                    return _future.available() && !need_preempt();
                }

                template<typename U>
                void await_suspend(ACTOR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
                    if (!_future.available()) {
                        _future.set_coroutine(hndl.promise());
                    } else {
                        schedule(&hndl.promise());
                    }
                }

                std::tuple<T...> await_resume() {
                    return _future.get();
                }
            };

            template<typename T>
            struct awaiter<T> {
                nil::actor::future<T> _future;

            public:
                explicit awaiter(nil::actor::future<T> &&f) noexcept : _future(std::move(f)) {
                }

                awaiter(const awaiter &) = delete;
                awaiter(awaiter &&) = delete;

                bool await_ready() const noexcept {
                    return _future.available() && !need_preempt();
                }

                template<typename U>
                void await_suspend(ACTOR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
                    if (!_future.available()) {
                        _future.set_coroutine(hndl.promise());
                    } else {
                        schedule(&hndl.promise());
                    }
                }

                T await_resume() {
                    return _future.get0();
                }
            };

            template<>
            struct awaiter<> {
                nil::actor::future<> _future;

            public:
                explicit awaiter(nil::actor::future<> &&f) noexcept : _future(std::move(f)) {
                }

                awaiter(const awaiter &) = delete;
                awaiter(awaiter &&) = delete;

                bool await_ready() const noexcept {
                    return _future.available() && !need_preempt();
                }

                template<typename U>
                void await_suspend(ACTOR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
                    if (!_future.available()) {
                        _future.set_coroutine(hndl.promise());
                    } else {
                        schedule(&hndl.promise());
                    }
                }

                void await_resume() {
                    _future.get();
                }
            };

        }    // namespace detail

        template<typename... T>
        auto operator co_await(future<T...> f) noexcept {
            return detail::awaiter<T...>(std::move(f));
        }

    }    // namespace actor
}    // namespace nil

namespace ACTOR_INTERNAL_COROUTINE_NAMESPACE {

    template<typename... T, typename... Args>
    class coroutine_traits<nil::actor::future<T...>, Args...> : public nil::actor::detail::coroutine_traits_base<T...> {
    };

}    // namespace ACTOR_INTERNAL_COROUTINE_NAMESPACE
