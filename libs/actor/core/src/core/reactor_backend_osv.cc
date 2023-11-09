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

#include <nil/actor/core/detail/reactor_backend_osv.hh>
#include <nil/actor/core/detail/thread_pool.hh>
#include <nil/actor/core/detail/syscall_result.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/detail/buffer_allocator.hh>

#include <nil/actor/detail/defer.hh>
#include <nil/actor/detail/read_first_line.hh>

#include <chrono>
#include <sys/poll.h>
#include <sys/syscall.h>

namespace nil {
    namespace actor {

        using namespace std::chrono_literals;
        using namespace detail;
        using namespace detail::linux_abi;

#ifdef HAVE_OSV
        reactor_backend_osv::reactor_backend_osv() {
        }

        bool reactor_backend_osv::reap_kernel_completions() {
            _poller.process();
            // osv::poller::process runs pollable's callbacks, but does not currently
            // have a timer expiration callback - instead if gives us an expired()
            // function we need to check:
            if (_poller.expired()) {
                _timer_promise.set_value();
                _timer_promise = promise<>();
            }
            return true;
        }

        reactor_backend_osv::kernel_submit_work() {
        }

        void reactor_backend_osv::wait_and_process_events(const sigset_t *sigset) {
            return process_events_nowait();
        }

        future<> reactor_backend_osv::readable(pollable_fd_state &fd) {
            std::cerr
                << "reactor_backend_osv does not support file descriptors - readable() shouldn't have been called!\n";
            abort();
        }

        future<> reactor_backend_osv::writeable(pollable_fd_state &fd) {
            std::cerr
                << "reactor_backend_osv does not support file descriptors - writeable() shouldn't have been called!\n";
            abort();
        }

        void reactor_backend_osv::forget(pollable_fd_state &fd) noexcept {
            std::cerr
                << "reactor_backend_osv does not support file descriptors - forget() shouldn't have been called!\n";
            abort();
        }

        future<std::tuple<pollable_fd, socket_address>> reactor_backend_osv::accept(pollable_fd_state &listenfd) {
            return engine().do_accept(listenfd);
        }

        future<> reactor_backend_osv::connect(pollable_fd_state &fd, socket_address &sa) {
            return engine().do_connect(fd, sa);
        }

        void reactor_backend_osv::shutdown(pollable_fd_state &fd, int how) {
            fd.fd.shutdown(how);
        }

        future<size_t> reactor_backend_osv::read_some(pollable_fd_state &fd, void *buffer, size_t len) {
            return engine().do_read_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_osv::read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) {
            return engine().do_read_some(fd, iov);
        }

        future<temporary_buffer<char>> reactor_backend_osv::read_some(pollable_fd_state &fd,
                                                                      detail::buffer_allocator *ba) {
            return engine().do_read_some(fd, ba);
        }

        future<size_t> reactor_backend_osv::write_some(pollable_fd_state &fd, const void *buffer, size_t len) {
            return engine().do_write_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_osv::write_some(pollable_fd_state &fd, net::packet &p) {
            return engine().do_write_some(fd, p);
        }

        void reactor_backend_osv::enable_timer(steady_clock_type::time_point when) {
            _poller.set_timer(when);
        }

        pollable_fd_state_ptr reactor_backend_osv::make_pollable_fd_state(file_desc fd,
                                                                          pollable_fd::speculation speculate) {
            std::cerr
                << "reactor_backend_osv does not support file descriptors - make_pollable_fd_state() shouldn't have "
                   "been called!\n";
            abort();
        }
#endif

    }    // namespace actor
}    // namespace nil
