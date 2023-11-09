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

#include <boost/predef.h>

#if BOOST_OS_LINUX
#include <nil/actor/core/detail/reactor_backend_aio.hh>
#else
#include <nil/actor/core/detail/reactor_backend.hh>
#endif

#include <sys/epoll.h>
#include <sys/syscall.h>

namespace nil {
    namespace actor {

        class epoll_pollable_fd_state : public pollable_fd_state {
            pollable_fd_state_completion _pollin;
            pollable_fd_state_completion _pollout;

            pollable_fd_state_completion *get_desc(int events) {
                if (events & EPOLLIN) {
                    return &_pollin;
                }
                return &_pollout;
            }

        public:
            explicit epoll_pollable_fd_state(file_desc fd, speculation speculate) :
                pollable_fd_state(std::move(fd), std::move(speculate)) {
            }
            future<> get_completion_future(int event) {
                auto desc = get_desc(event);
                *desc = pollable_fd_state_completion {};
                return desc->get_future();
            }

            void complete_with(int event) {
                get_desc(event)->complete_with(event);
            }
        };

        // reactor backend using file-descriptor & epoll, suitable for running on
        // Linux. Can wait on multiple file descriptors, and converts other events
        // (such as timers, signals, inter-thread notifications) into file descriptors
        // using mechanisms like timerfd, signalfd and eventfd respectively.
        class reactor_backend_epoll : public reactor_backend {
            reactor &_r;
            std::atomic<bool> _highres_timer_pending;
            std::thread _task_quota_timer_thread;
            ::itimerspec _steady_clock_timer_deadline = {};
            // These two timers are used for high resolution timer<>s, one for
            // the reactor thread (when sleeping) and one for the timer thread
            // (when awake). We can't use one timer because of races between the
            // timer thread and reactor thread.
            //
            // Only one of the two is active at any time.
            file_desc _steady_clock_timer_reactor_thread;
            file_desc _steady_clock_timer_timer_thread;

        private:
            file_desc _epollfd;
            void task_quota_timer_thread_fn();
            future<> get_epoll_future(pollable_fd_state &fd, int event);
            void complete_epoll_event(pollable_fd_state &fd, int events, int event);
#if BOOST_OS_LINUX
            aio_storage_context _storage_context;
#endif
            void switch_steady_clock_timers(file_desc &from, file_desc &to);
            void maybe_switch_steady_clock_timers(int timeout, file_desc &from, file_desc &to);
            bool wait_and_process(int timeout, const sigset_t *active_sigmask);
            bool complete_hrtimer();
            bool _need_epoll_events = false;

        public:
            explicit reactor_backend_epoll(reactor &r);
            virtual ~reactor_backend_epoll() override;

            virtual bool reap_kernel_completions() override;
            virtual bool kernel_submit_work() override;
            virtual bool kernel_events_can_sleep() const override;
            virtual void wait_and_process_events(const sigset_t *active_sigmask) override;
            virtual future<> readable(pollable_fd_state &fd) override;
            virtual future<> writeable(pollable_fd_state &fd) override;
            virtual future<> readable_or_writeable(pollable_fd_state &fd) override;
            virtual void forget(pollable_fd_state &fd) noexcept override;

            virtual future<std::tuple<pollable_fd, socket_address>> accept(pollable_fd_state &listenfd) override;
            virtual future<> connect(pollable_fd_state &fd, socket_address &sa) override;
            virtual void shutdown(pollable_fd_state &fd, int how) override;
            virtual future<size_t> read_some(pollable_fd_state &fd, void *buffer, size_t len) override;
            virtual future<size_t> read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) override;
            virtual future<temporary_buffer<char>> read_some(pollable_fd_state &fd,
                                                             detail::buffer_allocator *ba) override;
            virtual future<size_t> write_some(pollable_fd_state &fd, net::packet &p) override;
            virtual future<size_t> write_some(pollable_fd_state &fd, const void *buffer, size_t len) override;

            virtual void signal_received(int signo, siginfo_t *siginfo, void *ignore) override;
            virtual void start_tick() override;
            virtual void stop_tick() override;
            virtual void arm_highres_timer(const ::itimerspec &ts) override;
            virtual void reset_preemption_monitor() override;
            virtual void request_preemption() override;
            virtual void start_handling_signal() override;

            virtual pollable_fd_state_ptr make_pollable_fd_state(file_desc fd,
                                                                 pollable_fd::speculation speculate) override;
        };
    }    // namespace actor
}    // namespace nil
