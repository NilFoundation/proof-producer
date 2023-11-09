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

#include <nil/actor/core/thread_impl.hh>
#include <nil/actor/core/future.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/timer.hh>
#include <nil/actor/core/scheduling.hh>
#include <memory>
#include <csetjmp>
#include <type_traits>
#include <chrono>
#include <nil/actor/detail/std-compat.hh>
#include <boost/intrusive/list.hpp>

/// \defgroup thread-module =nil; Actor threads
///
/// =nil; Actor threads provide an execution environment where blocking
/// is tolerated; you can issue I/O, and wait for it in the same function,
/// rather then establishing a callback to be called with \ref future<>::then().
///
/// =nil; Actor threads are not the same as operating system threads:
///   - =nil; Actor threads are cooperative; they are never preempted except
///     at blocking points (see below)
///   - =nil; Actor threads always run on the same core they were launched on
///
/// Like other actor code, actor threads may not issue blocking system calls.
///
/// A actor thread blocking point is any function that returns a \ref future.
/// you block by calling \ref future<>::get(); this waits for the future to become
/// available, and in the meanwhile, other actor threads and actor non-threaded
/// code may execute.
///
/// Example:
/// \code
///    nil::actor::thread th([] {
///       sleep(5s).get();  // blocking point
///    });
/// \endcode
///
/// An easy way to launch a thread and carry out some computation, and return a
/// result from this execution is by using the \ref nil::actor::async() function.
/// The result is returned as a future, so that non-threaded code can wait for
/// the thread to terminate and yield a result.

/// =nil; Actor API namespace
namespace nil {
    namespace actor {

        /// \addtogroup thread-module
        /// @{

        class thread;
        class thread_attributes;

        /// Class that holds attributes controling the behavior of a thread.
        class thread_attributes {
        public:
            boost::optional<nil::actor::scheduling_group> sched_group;
            // For stack_size 0, a default value will be used (128KiB when writing this comment)
            size_t stack_size = 0;
        };

        /// \cond internal
        extern thread_local jmp_buf_link g_unthreaded_context;

        // Internal class holding thread state.  We can't hold this in
        // \c thread itself because \c thread is movable, and we want pointers
        // to this state to be captured.
        class thread_context final : private task {
            struct stack_deleter {
                void operator()(char *ptr) const noexcept;
#ifdef ACTOR_HAS_VALGRIND
                int valgrind_id;
                stack_deleter(int valgrind_id);
#else
                stack_deleter(int);
#endif
            };
            using stack_holder = std::unique_ptr<char[], stack_deleter>;

            stack_holder _stack;
            noncopyable_function<void()> _func;
            jmp_buf_link _context;
            promise<> _done;
            bool _joined = false;

            boost::intrusive::list_member_hook<> _all_link;
            using all_thread_list =
                boost::intrusive::list<thread_context,
                                       boost::intrusive::member_hook<thread_context,
                                                                     boost::intrusive::list_member_hook<>,
                                                                     &thread_context::_all_link>,
                                       boost::intrusive::constant_time_size<false>>;

            static thread_local all_thread_list _all_threads;

        private:
            static void s_main(int lo, int hi);    // all parameters MUST be 'int' for makecontext
            void setup(size_t stack_size);
            void main();
            stack_holder make_stack(size_t stack_size);
            virtual void run_and_dispose() noexcept override;    // from task class
        public:
            thread_context(thread_attributes attr, noncopyable_function<void()> func);
            ~thread_context();
            void switch_in();
            void switch_out();
            bool should_yield() const;
            void reschedule();
            void yield();
            task *waiting_task() noexcept override {
                return _done.waiting_task();
            }
            friend class thread;
            friend void thread_impl::switch_in(thread_context *);
            friend void thread_impl::switch_out(thread_context *);
            friend scheduling_group thread_impl::sched_group(const thread_context *);
        };

        /// \endcond

        /// \brief thread - stateful thread of execution
        ///
        /// Threads allow using actor APIs in a blocking manner,
        /// by calling future::get() on a non-ready future.  When
        /// this happens, the thread is put to sleep until the future
        /// becomes ready.
        class thread {
            std::unique_ptr<thread_context> _context;
            static thread_local thread *_current;

        public:
            /// \brief Constructs a \c thread object that does not represent a thread
            /// of execution.
            thread() = default;
            /// \brief Constructs a \c thread object that represents a thread of execution
            ///
            /// \param func Callable object to execute in thread.  The callable is
            ///             called immediately.
            template<typename Func>
            thread(Func func);
            /// \brief Constructs a \c thread object that represents a thread of execution
            ///
            /// \param attr Attributes describing the new thread.
            /// \param func Callable object to execute in thread.  The callable is
            ///             called immediately.
            template<typename Func>
            thread(thread_attributes attr, Func func);
            /// \brief Moves a thread object.
            thread(thread &&x) noexcept = default;
            /// \brief Move-assigns a thread object.
            thread &operator=(thread &&x) noexcept = default;
            /// \brief Destroys a \c thread object.
            ///
            /// The thread must not represent a running thread of execution (see join()).
            ~thread() {
                assert(!_context || _context->_joined);
            }
            /// \brief Waits for thread execution to terminate.
            ///
            /// Waits for thread execution to terminate, and marks the thread object as not
            /// representing a running thread of execution.
            future<> join();
            /// \brief Voluntarily defer execution of current thread.
            ///
            /// Gives other threads/fibers a chance to run on current CPU.
            /// The current thread will resume execution promptly.
            static void yield();
            /// \brief Checks whether this thread ought to call yield() now.
            ///
            /// Useful where we cannot call yield() immediately because we
            /// Need to take some cleanup action first.
            static bool should_yield();

            /// \brief Yield if this thread ought to call yield() now.
            ///
            /// Useful where a code does long running computation and does
            /// not want to hog cpu for more then its share
            static void maybe_yield();

            static bool running_in_thread() {
                return thread_impl::get() != nullptr;
            }
        };

        template<typename Func>
        inline thread::thread(thread_attributes attr, Func func) :
            _context(std::make_unique<thread_context>(std::move(attr), std::move(func))) {
        }

        template<typename Func>
        inline thread::thread(Func func) : thread(thread_attributes(), std::move(func)) {
        }

        inline future<> thread::join() {
            _context->_joined = true;
            return _context->_done.get_future();
        }

        /// Executes a callable in a actor thread.
        ///
        /// Runs a block of code in a threaded context,
        /// which allows it to block (using \ref future::get()).  The
        /// result of the callable is returned as a future.
        ///
        /// \param attr a \ref thread_attributes instance
        /// \param func a callable to be executed in a thread
        /// \param args a parameter pack to be forwarded to \c func.
        /// \return whatever \c func returns, as a future.
        ///
        /// Example:
        /// \code
        ///    future<int> compute_sum(int a, int b) {
        ///        thread_attributes attr = {};
        ///        attr.sched_group = some_scheduling_group_ptr;
        ///        return nil::actor::async(attr, [a, b] {
        ///            // some blocking code:
        ///            sleep(1s).get();
        ///            return a + b;
        ///        });
        ///    }
        /// \endcode
        template<typename Func, typename... Args>
        inline futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
            async(thread_attributes attr, Func &&func, Args &&...args) noexcept {
            using return_type = std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>;
            struct work {
                thread_attributes attr;
                Func func;
                std::tuple<Args...> args;
                promise<return_type> pr;
                thread th;
            };

            try {
                auto wp = std::make_unique<work>(work {std::move(attr), std::forward<Func>(func),
                                                       std::forward_as_tuple(std::forward<Args>(args)...)});
                auto &w = *wp;
                auto ret = w.pr.get_future();
                w.th = thread(std::move(w.attr), [&w] {
                    futurize<return_type>::apply(std::move(w.func), std::move(w.args)).forward_to(std::move(w.pr));
                });
                return w.th.join()
                    .then([ret = std::move(ret)]() mutable { return std::move(ret); })
                    .finally([wp = std::move(wp)] {});
            } catch (...) {
                return futurize<return_type>::make_exception_future(std::current_exception());
            }
        }

        /// Executes a callable in a actor thread.
        ///
        /// Runs a block of code in a threaded context,
        /// which allows it to block (using \ref future::get()).  The
        /// result of the callable is returned as a future.
        ///
        /// \param func a callable to be executed in a thread
        /// \param args a parameter pack to be forwarded to \c func.
        /// \return whatever \c func returns, as a future.
        template<typename Func, typename... Args>
        inline futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>> async(Func &&func,
                                                                                             Args &&...args) noexcept {
            return async(thread_attributes {}, std::forward<Func>(func), std::forward<Args>(args)...);
        }
        /// @}

    }    // namespace actor
}    // namespace nil
