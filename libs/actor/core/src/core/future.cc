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

#include <nil/actor/core/future.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/report_exception.hh>
#include <nil/actor/detail/backtrace.hh>

namespace nil {
    namespace actor {

        // We can't test future_state_base directly because its private
        // destructor is protected.
        static_assert(std::is_nothrow_move_constructible<future_state<std::tuple<int>>>::value,
                      "future_state's move constructor must not throw");

        static_assert(sizeof(future_state<std::tuple<>>) <= 8, "future_state<std::tuple<>> is too large");
        static_assert(sizeof(future_state<std::tuple<long>>) <= 16, "future_state<std::tuple<long>> is too large");
        static_assert(future_state<std::tuple<>>::has_trivial_move_and_destroy,
                      "future_state<std::tuple<>> not trivial");
#if ACTOR_API_LEVEL < 5
        static_assert(future_state<std::tuple<long>>::has_trivial_move_and_destroy,
                      "future_state<std::tuple<long>> not trivial");
#else
        static_assert(future_state<long>::has_trivial_move_and_destroy, "future_state<long> not trivial");
#endif

        // We need to be able to move and copy std::exception_ptr in and out
        // of future/promise/continuations without that producing a new
        // exception.
        static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                      "std::exception_ptr's copy constructor must not throw");
        static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                      "std::exception_ptr's move constructor must not throw");

        namespace detail {

            static_assert(std::is_empty<uninitialized_wrapper<std::tuple<>>>::value, "This should still be empty");

            void promise_base::move_it(promise_base &&x) noexcept {
                // Don't use std::exchange to make sure x's values are nulled even
                // if &x == this.
                _task = x._task;
                x._task = nullptr;
                _state = x._state;
                x._state = nullptr;
                _future = x._future;
                if (auto *fut = _future) {
                    fut->detach_promise();
                    fut->_promise = this;
                }
            }

            static void set_to_broken_promise(future_state_base &state) noexcept {
                try {
                    // Constructing broken_promise may throw (std::logic_error ctor is not noexcept).
                    state.set_exception(std::make_exception_ptr(broken_promise {}));
                } catch (...) {
                    state.set_exception(std::current_exception());
                }
            }

            promise_base::promise_base(promise_base &&x) noexcept {
                move_it(std::move(x));
            }

            void promise_base::clear() noexcept {
                if (__builtin_expect(bool(_task), false)) {
                    assert(_state && !_state->available());
                    set_to_broken_promise(*_state);
                    ::nil::actor::schedule(std::exchange(_task, nullptr));
                }
                if (_future) {
                    assert(_state);
                    if (!_state->available()) {
                        set_to_broken_promise(*_state);
                    }
                    _future->detach_promise();
                }
            }

            promise_base &promise_base::operator=(promise_base &&x) noexcept {
                clear();
                move_it(std::move(x));
                return *this;
            }

            void promise_base::set_to_current_exception() noexcept {
                set_exception(std::current_exception());
            }

            template<promise_base::urgent Urgent>
            void promise_base::make_ready() noexcept {
                if (_task) {
                    if (Urgent == urgent::yes) {
                        ::nil::actor::schedule_urgent(std::exchange(_task, nullptr));
                    } else {
                        ::nil::actor::schedule(std::exchange(_task, nullptr));
                    }
                }
            }

            template void promise_base::make_ready<promise_base::urgent::no>() noexcept;
            template void promise_base::make_ready<promise_base::urgent::yes>() noexcept;
        }    // namespace detail

        template future<> current_exception_as_future() noexcept;

        /**
         * engine_exit() exits the reactor. It should be given a pointer to the
         * exception which prompted this exit - or a null pointer if the exit
         * request was not caused by any exception.
         */
        void engine_exit(std::exception_ptr eptr) {
            if (!eptr) {
                engine().exit(0);
                return;
            }
            report_exception("Exiting on unhandled exception", eptr);
            engine().exit(1);
        }

        broken_promise::broken_promise() : logic_error("broken promise") {
        }

        future_state_base::future_state_base(current_exception_future_marker) noexcept :
            future_state_base(std::current_exception()) {
        }

        void future_state_base::ignore() noexcept {
            switch (_u.st) {
                case state::invalid:
                case state::future:
                case state::result_unavailable:
                    assert(0 && "invalid state for ignore");
                case state::result:
                    _u.st = state::result_unavailable;
                    break;
                default:
                    // Ignore the exception
                    _u.take_exception();
            }
        }

        nested_exception::nested_exception(std::exception_ptr inner, std::exception_ptr outer) noexcept :
            inner(std::move(inner)), outer(std::move(outer)) {
        }

        nested_exception::nested_exception(nested_exception &&) noexcept = default;

        nested_exception::nested_exception(const nested_exception &) noexcept = default;

        const char *nested_exception::what() const noexcept {
            return "nil::actor::nested_exception";
        }

        [[noreturn]] void nested_exception::rethrow_nested() const {
            std::rethrow_exception(outer);
        }

        static std::exception_ptr make_nested(std::exception_ptr &&inner, future_state_base &&old) noexcept {
            std::exception_ptr outer = std::move(old).get_exception();
            nested_exception nested {std::move(inner), std::move(outer)};
            return std::make_exception_ptr<nested_exception>(std::move(nested));
        }

        future_state_base::future_state_base(nested_exception_marker,
                                             future_state_base &&n,
                                             future_state_base &&old) noexcept {
            std::exception_ptr inner = std::move(n).get_exception();
            if (!old.failed()) {
                new (this) future_state_base(std::move(inner));
            } else {
                new (this) future_state_base(make_nested(std::move(inner), std::move(old)));
            }
        }

        future_state_base::future_state_base(nested_exception_marker, future_state_base &&old) noexcept {
            if (!old.failed()) {
                new (this) future_state_base(current_exception_future_marker());
                return;
            } else {
                new (this) future_state_base(make_nested(std::current_exception(), std::move(old)));
            }
        }

        void future_state_base::rethrow_exception() && {
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(*this).get_exception());
        }

        void future_state_base::rethrow_exception() const & {
            std::rethrow_exception(_u.ex);
        }

        void report_failed_future(const std::exception_ptr &eptr) noexcept {
            ++engine()._abandoned_failed_futures;
            actor_logger.warn("Exceptional future ignored: {}, backtrace: {}", eptr, current_backtrace());
        }

        void report_failed_future(const future_state_base &state) noexcept {
            report_failed_future(state._u.ex);
        }

        void report_failed_future(future_state_base::any &&state) noexcept {
            report_failed_future(std::move(state).take_exception());
        }

        void with_allow_abandoned_failed_futures(unsigned count, noncopyable_function<void()> func) {
            auto before = engine()._abandoned_failed_futures;
            func();
            auto after = engine()._abandoned_failed_futures;
            assert(after - before == count);
            engine()._abandoned_failed_futures = before;
        }

        namespace {
            class thread_wake_task final : public task {
                thread_context *_thread;

            public:
                thread_wake_task(thread_context *thread) noexcept : _thread(thread) {
                }
                virtual void run_and_dispose() noexcept override {
                    thread_impl::switch_in(_thread);
                    // no need to delete, since this is always allocated on
                    // _thread's stack.
                }
                /// Returns the task which is waiting for this thread to be done, or nullptr.
                virtual task *waiting_task() noexcept override {
                    return _thread->waiting_task();
                }
            };
        }    // namespace

        void detail::future_base::do_wait() noexcept {
            auto thread = thread_impl::get();
            assert(thread);
            thread_wake_task wake_task {thread};
            wake_task.make_backtrace();
            _promise->_task = &wake_task;
            thread_impl::switch_out(thread);
        }

#ifdef ACTOR_COROUTINES_ENABLED
        void detail::future_base::set_coroutine(task &coroutine) noexcept {
            assert(_promise);
            _promise->_task = &coroutine;
        }
#endif

    }    // namespace actor
}    // namespace nil
