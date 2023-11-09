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

#include <nil/actor/core/detail/reactor_backend.hh>

#ifdef HAVE_OSV
#include <osv/newpoll.hh>
#endif

namespace nil {
    namespace actor {
#ifdef HAVE_OSV
        // reactor_backend using OSv-specific features, without any file descriptors.
        // This implementation cannot currently wait on file descriptors, but unlike
        // reactor_backend_epoll it doesn't need file descriptors for waiting on a
        // timer, for example, so file descriptors are not necessary.
        class reactor_backend_osv : public reactor_backend {
        private:
            osv::newpoll::poller _poller;
            future<> get_poller_future(reactor_notifier_osv *n);
            promise<> _timer_promise;

        public:
            reactor_backend_osv();
            virtual ~reactor_backend_osv() override {
            }

            virtual bool reap_kernel_completions() override;
            virtual bool kernel_submit_work() override;
            virtual bool kernel_events_can_sleep() const override;
            virtual void wait_and_process_events(const sigset_t *active_sigmask) override;
            virtual future<> readable(pollable_fd_state &fd) override;
            virtual future<> writeable(pollable_fd_state &fd) override;
            virtual void forget(pollable_fd_state &fd) noexcept override;

            virtual future<std::tuple<pollable_fd, socket_address>> accept(pollable_fd_state &listenfd) override;
            virtual future<> connect(pollable_fd_state &fd, socket_address &sa) override;
            virtual void shutdown(pollable_fd_state &fd, int how) override;
            virtual future<size_t> read_some(pollable_fd_state &fd, void *buffer, size_t len) override;
            virtual future<size_t> read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) override;
            virtual future<temporary_buffer<char>> read_some(pollable_fd_state &fd,
                                                             detail::buffer_allocator *ba) override;
            virtual future<size_t> write_some(net::packet &p) override;
            virtual future<size_t> write_some(pollable_fd_state &fd, const void *buffer, size_t len) override;

            void enable_timer(steady_clock_type::time_point when);
            virtual pollable_fd_state_ptr make_pollable_fd_state(file_desc fd,
                                                                 pollable_fd::speculation speculate) override;
        };
#endif /* HAVE_OSV */

    }    // namespace actor
}    // namespace nil
