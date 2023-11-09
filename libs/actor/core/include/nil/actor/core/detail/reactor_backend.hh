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
#include <nil/actor/core/posix.hh>
#include <nil/actor/core/linux-aio.hh>
#include <nil/actor/core/cacheline.hh>

#include <nil/actor/core/detail/pollable_fd.hh>
#include <nil/actor/core/detail/poll.hh>

#include <sys/time.h>

#include <csignal>
#include <thread>
#include <stack>

#include <boost/any.hpp>
#include <boost/predef.h>
#include <boost/program_options.hpp>
#include <boost/container/static_vector.hpp>

namespace nil {
    namespace actor {

        class reactor;

        class pollable_fd_state_completion : public kernel_completion {
            promise<> _pr;

        public:
            virtual void complete_with(ssize_t res) override {
                _pr.set_value();
            }
            future<> get_future() {
                return _pr.get_future();
            }
        };

        // The "reactor_backend" interface provides a method of waiting for various
        // basic events on one thread. We have one implementation based on epoll and
        // file-descriptors (reactor_backend_epoll) and one implementation based on
        // OSv-specific file-descriptor-less mechanisms (reactor_backend_osv).
        class reactor_backend {
        public:
            virtual ~reactor_backend() {};
            // The methods below are used to communicate with the kernel.
            // reap_kernel_completions() will complete any previous async
            // work that is ready to consume.
            // kernel_submit_work() submit new events that were produced.
            // Both of those methods are asynchronous and will never block.
            //
            // wait_and_process_events on the other hand may block, and is called when
            // we are about to go to sleep.
            virtual bool reap_kernel_completions() = 0;
            virtual bool kernel_submit_work() = 0;
            virtual bool kernel_events_can_sleep() const = 0;
            virtual void wait_and_process_events(const sigset_t *active_sigmask = nullptr) = 0;

            // Methods that allow polling on file descriptors. This will only work on
            // reactor_backend_epoll. Other reactor_backend will probably abort if
            // they are called (which is fine if no file descriptors are waited on):
            virtual future<> readable(pollable_fd_state &fd) = 0;
            virtual future<> writeable(pollable_fd_state &fd) = 0;
            virtual future<> readable_or_writeable(pollable_fd_state &fd) = 0;
            virtual void forget(pollable_fd_state &fd) noexcept = 0;

            virtual future<std::tuple<pollable_fd, socket_address>> accept(pollable_fd_state &listenfd) = 0;
            virtual future<> connect(pollable_fd_state &fd, socket_address &sa) = 0;
            virtual void shutdown(pollable_fd_state &fd, int how) = 0;
            virtual future<size_t> read_some(pollable_fd_state &fd, void *buffer, size_t len) = 0;
            virtual future<size_t> read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) = 0;
            virtual future<temporary_buffer<char>> read_some(pollable_fd_state &fd, detail::buffer_allocator *ba) = 0;
            virtual future<size_t> write_some(pollable_fd_state &fd, net::packet &p) = 0;
            virtual future<size_t> write_some(pollable_fd_state &fd, const void *buffer, size_t len) = 0;

            virtual void signal_received(int signo, siginfo_t *siginfo, void *ignore) = 0;
            virtual void start_tick() = 0;
            virtual void stop_tick() = 0;
            virtual void arm_highres_timer(const ::itimerspec &ts) = 0;
            virtual void reset_preemption_monitor() = 0;
            virtual void request_preemption() = 0;
            virtual void start_handling_signal() = 0;

            virtual pollable_fd_state_ptr make_pollable_fd_state(file_desc fd, pollable_fd::speculation speculate) = 0;
        };

    }    // namespace actor
}    // namespace nil
