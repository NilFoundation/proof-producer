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

#include <nil/actor/core/detail/reactor_backend_aio.hh>
#include <nil/actor/core/detail/buffer_allocator.hh>
#include <nil/actor/core/detail/thread_pool.hh>
#include <nil/actor/core/detail/syscall_result.hh>

#include <nil/actor/core/print.hh>
#include <nil/actor/core/reactor.hh>

#include <nil/actor/detail/defer.hh>
#include <nil/actor/detail/read_first_line.hh>

#include <chrono>

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/syscall.h>

namespace nil {
    namespace actor {

        using namespace std::chrono_literals;
        using namespace detail;
        using namespace detail::linux_abi;

        void prepare_iocb(io_request &req, io_completion *desc, iocb &iocb) {
            switch (req.opcode()) {
                case io_request::operation::fdatasync:
                    iocb = make_fdsync_iocb(req.fd());
                    break;
                case io_request::operation::write:
                    iocb = make_write_iocb(req.fd(), req.pos(), req.address(), req.size());
                    set_nowait(iocb, true);
                    break;
                case io_request::operation::writev:
                    iocb = make_writev_iocb(req.fd(), req.pos(), req.iov(), req.size());
                    set_nowait(iocb, true);
                    break;
                case io_request::operation::read:
                    iocb = make_read_iocb(req.fd(), req.pos(), req.address(), req.size());
                    set_nowait(iocb, true);
                    break;
                case io_request::operation::readv:
                    iocb = make_readv_iocb(req.fd(), req.pos(), req.iov(), req.size());
                    set_nowait(iocb, true);
                    break;
                default:
                    actor_logger.error("Invalid operation for iocb: {}", req.opname());
                    std::abort();
            }
            set_user_data(iocb, desc);
        }

        aio_storage_context::iocb_pool::iocb_pool() {
            for (unsigned i = 0; i != max_aio; ++i) {
                _free_iocbs.push(&_iocb_pool[i]);
            }
        }

        aio_storage_context::aio_storage_context(reactor &r) : _r(r), _io_context(0) {
            static_assert(max_aio >= reactor::max_queues * reactor::max_queues,
                          "Mismatch between maximum allowed io and what the IO queues can produce");
            detail::setup_aio_context(max_aio, &_io_context);
        }

        aio_storage_context::~aio_storage_context() {
            detail::io_destroy(_io_context);
        }

        inline detail::linux_abi::iocb &aio_storage_context::iocb_pool::get_one() {
            auto io = _free_iocbs.top();
            _free_iocbs.pop();
            return *io;
        }

        inline void aio_storage_context::iocb_pool::put_one(detail::linux_abi::iocb *io) {
            _free_iocbs.push(io);
        }

        inline unsigned aio_storage_context::iocb_pool::outstanding() const {
            return max_aio - _free_iocbs.size();
        }

        inline bool aio_storage_context::iocb_pool::has_capacity() const {
            return !_free_iocbs.empty();
        }

        // Returns: number of iocbs consumed (0 or 1)
        size_t aio_storage_context::handle_aio_error(linux_abi::iocb *iocb, int ec) {
            switch (ec) {
                case EAGAIN:
                    return 0;
                case EBADF: {
                    auto desc = reinterpret_cast<kernel_completion *>(get_user_data(*iocb));
                    _iocb_pool.put_one(iocb);
                    desc->complete_with(-EBADF);
                    // if EBADF, it means that the first request has a bad fd, so
                    // we will only remove it from _pending_io and try again.
                    return 1;
                }
                default:
                    ++_r._io_stats.aio_errors;
                    throw_system_error_on(true, "io_submit");
                    abort();
            }
        }

        extern bool aio_nowait_supported;

        bool aio_storage_context::submit_work() {
            bool did_work = false;

            _submission_queue.resize(0);
            size_t to_submit = _r._io_sink.drain([this](detail::io_request &req, io_completion *desc) -> bool {
                if (!_iocb_pool.has_capacity()) {
                    return false;
                }

                auto &io = _iocb_pool.get_one();
                prepare_iocb(req, desc, io);

                if (_r._aio_eventfd) {
                    set_eventfd_notification(io, _r._aio_eventfd->get_fd());
                }
                _submission_queue.push_back(&io);
                return true;
            });

            size_t submitted = 0;
            while (to_submit > submitted) {
                auto nr = to_submit - submitted;
                auto iocbs = _submission_queue.data() + submitted;
                auto r = io_submit(_io_context, nr, iocbs);
                size_t nr_consumed;
                if (r == -1) {
                    nr_consumed = handle_aio_error(iocbs[0], errno);
                } else {
                    nr_consumed = size_t(r);
                }
                did_work = true;
                submitted += nr_consumed;
            }

            if (!_pending_aio_retry.empty()) {
                schedule_retry();
                did_work = true;
            }

            return did_work;
        }

        void aio_storage_context::schedule_retry() {
            // FIXME: future is discarded
            (void)do_with(std::exchange(_pending_aio_retry, {}), [this](pending_aio_retry_t &retries) {
                return _r._thread_pool
                    ->submit<syscall_result<int>>([this, &retries]() mutable {
                        auto r = io_submit(_io_context, retries.size(), retries.data());
                        return wrap_syscall<int>(r);
                    })
                    .then([this, &retries](syscall_result<int> result) {
                        auto iocbs = retries.data();
                        size_t nr_consumed = 0;
                        if (result.result == -1) {
                            nr_consumed = handle_aio_error(iocbs[0], result.error);
                        } else {
                            nr_consumed = result.result;
                        }
                        std::copy(retries.begin() + nr_consumed, retries.end(), std::back_inserter(_pending_aio_retry));
                    });
            });
        }

        bool aio_storage_context::reap_completions() {
            struct timespec timeout = {0, 0};
            auto n = io_getevents(_io_context, 1, max_aio, _ev_buffer, &timeout, _r._force_io_getevents_syscall);
            if (n == -1 && errno == EINTR) {
                n = 0;
            }
            assert(n >= 0);
            for (size_t i = 0; i < size_t(n); ++i) {
                auto iocb = get_iocb(_ev_buffer[i]);
                if (_ev_buffer[i].res == -EAGAIN) {
                    set_nowait(*iocb, false);
                    _pending_aio_retry.push_back(iocb);
                    continue;
                }
                _iocb_pool.put_one(iocb);
                auto desc = reinterpret_cast<kernel_completion *>(_ev_buffer[i].data);
                desc->complete_with(_ev_buffer[i].res);
            }
            return n;
        }

        bool aio_storage_context::can_sleep() const {
            // Because aio depends on polling, it cannot generate events to wake us up, Therefore, sleep
            // is only possible if there are no in-flight aios. If there are, we need to keep polling.
            //
            // Alternatively, if we enabled _aio_eventfd, we can always enter
            unsigned executing = _iocb_pool.outstanding();
            return executing == 0 || _r._aio_eventfd;
        }

        aio_general_context::aio_general_context(size_t nr) : iocbs(new iocb *[nr]) {
            setup_aio_context(nr, &io_context);
        }

        aio_general_context::~aio_general_context() {
            io_destroy(io_context);
        }

        void aio_general_context::queue(linux_abi::iocb *iocb) {
            *last++ = iocb;
        }

        size_t aio_general_context::flush() {
            if (last != iocbs.get()) {
                auto nr = last - iocbs.get();
                last = iocbs.get();
                auto r = io_submit(io_context, nr, iocbs.get());
                assert(r >= 0);
                return nr;
            }
            return 0;
        }

        completion_with_iocb::completion_with_iocb(int fd, int events, void *user_data) :
            _iocb(make_poll_iocb(fd, events)) {
            set_user_data(_iocb, user_data);
        }

        void completion_with_iocb::maybe_queue(aio_general_context &context) {
            if (!_in_context) {
                _in_context = true;
                context.queue(&_iocb);
            }
        }

        hrtimer_aio_completion::hrtimer_aio_completion(reactor &r, file_desc &fd) :
            fd_kernel_completion(r, fd), completion_with_iocb(fd.get(), POLLIN, this) {
        }

        task_quota_aio_completion::task_quota_aio_completion(reactor &r, file_desc &fd) :
            fd_kernel_completion(r, fd), completion_with_iocb(fd.get(), POLLIN, this) {
        }

        smp_wakeup_aio_completion::smp_wakeup_aio_completion(reactor &r, file_desc &fd) :
            fd_kernel_completion(r, fd), completion_with_iocb(fd.get(), POLLIN, this) {
        }

        void hrtimer_aio_completion::complete_with(ssize_t ret) {
            uint64_t expirations = 0;
            (void)_fd.read(&expirations, 8);
            if (expirations) {
                _r.service_highres_timer();
            }
            completion_with_iocb::completed();
        }

        void task_quota_aio_completion::complete_with(ssize_t ret) {
            uint64_t v;
            (void)_fd.read(&v, 8);
            completion_with_iocb::completed();
        }

        void smp_wakeup_aio_completion::complete_with(ssize_t ret) {
            uint64_t ignore = 0;
            (void)_fd.read(&ignore, 8);
            completion_with_iocb::completed();
        }

        preempt_io_context::preempt_io_context(reactor &r, file_desc &task_quota, file_desc &hrtimer) :
            _r(r), _task_quota_aio_completion(r, task_quota), _hrtimer_aio_completion(r, hrtimer) {
        }

        void preempt_io_context::start_tick() {
            // Preempt whenever an event (timer tick or signal) is available on the
            // _preempting_io ring
            g_need_preempt = reinterpret_cast<const preemption_monitor *>(_context.io_context + 8);
            // preempt_io_context::request_preemption() will write to reactor::_preemption_monitor, which is now ignored
        }

        void preempt_io_context::stop_tick() {
            g_need_preempt = &_r._preemption_monitor;
        }

        void preempt_io_context::request_preemption() {
            ::itimerspec expired = {};
            expired.it_value.tv_nsec = 1;
            // will trigger immediately, triggering the preemption monitor
            _hrtimer_aio_completion.fd().timerfd_settime(TFD_TIMER_ABSTIME, expired);

            // This might have been called from poll_once. If that is the case, we cannot assume that timerfd is being
            // monitored.
            _hrtimer_aio_completion.maybe_queue(_context);
            _context.flush();

            // The kernel is not obliged to deliver the completion immediately, so wait for it
            while (!need_preempt()) {
                std::atomic_signal_fence(std::memory_order_seq_cst);
            }
        }

        void preempt_io_context::reset_preemption_monitor() {
            service_preempting_io();
            _hrtimer_aio_completion.maybe_queue(_context);
            _task_quota_aio_completion.maybe_queue(_context);
            flush();
        }

        bool preempt_io_context::service_preempting_io() {
            linux_abi::io_event a[2];
            auto r = io_getevents(_context.io_context, 0, 2, a, 0);
            assert(r != -1);
            bool did_work = r > 0;
            for (unsigned i = 0; i != unsigned(r); ++i) {
                auto desc = reinterpret_cast<kernel_completion *>(a[i].data);
                desc->complete_with(a[i].res);
            }
            return did_work;
        }

        file_desc reactor_backend_aio::make_timerfd() {
            return file_desc::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        }

        bool reactor_backend_aio::await_events(int timeout, const sigset_t *active_sigmask) {
            ::timespec ts = {};
            ::timespec *tsp = [&]() -> ::timespec * {
                if (timeout == 0) {
                    return &ts;
                } else if (timeout == -1) {
                    return nullptr;
                } else {
                    ts = posix::to_timespec(timeout * 1ms);
                    return &ts;
                }
            }();
            constexpr size_t batch_size = 128;
            io_event batch[batch_size];
            bool did_work = false;
            int r;
            do {
                r = io_pgetevents(_polling_io.io_context, 1, batch_size, batch, tsp, active_sigmask);
                if (r == -1 && errno == EINTR) {
                    return true;
                }
                assert(r != -1);
                for (unsigned i = 0; i != unsigned(r); ++i) {
                    did_work = true;
                    auto &event = batch[i];
                    auto *desc = reinterpret_cast<kernel_completion *>(uintptr_t(event.data));
                    desc->complete_with(event.res);
                }
                // For the next iteration, don't use a timeout, since we may have waited already
                ts = {};
                tsp = &ts;
            } while (r == batch_size);
            return did_work;
        }

        void reactor_backend_aio::signal_received(int signo, siginfo_t *siginfo, void *ignore) {
            engine()._signals.action(signo, siginfo, ignore);
        }

        reactor_backend_aio::reactor_backend_aio(reactor &r) :
            _r(r), _hrtimer_timerfd(make_timerfd()), _storage_context(_r),
            _preempting_io(_r, _r._task_quota_timer, _hrtimer_timerfd), _hrtimer_poll_completion(_r, _hrtimer_timerfd),
            _smp_wakeup_aio_completion(_r, _r._notify_eventfd) {
            // Protect against spurious wakeups - if we get notified that the timer has
            // expired when it really hasn't, we don't want to block in read(tfd, ...).
            auto tfd = _r._task_quota_timer.get();
            ::fcntl(tfd, F_SETFL, ::fcntl(tfd, F_GETFL) | O_NONBLOCK);

            sigset_t mask = make_sigset_mask(hrtimer_signal());
            auto e = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
            assert(e == 0);
        }

        bool reactor_backend_aio::reap_kernel_completions() {
            bool did_work = await_events(0, nullptr);
            did_work |= _storage_context.reap_completions();
            return did_work;
        }

        bool reactor_backend_aio::kernel_submit_work() {
            _hrtimer_poll_completion.maybe_queue(_polling_io);
            bool did_work = _polling_io.flush();
            did_work |= _storage_context.submit_work();
            return did_work;
        }

        bool reactor_backend_aio::kernel_events_can_sleep() const {
            return _storage_context.can_sleep();
        }

        void reactor_backend_aio::wait_and_process_events(const sigset_t *active_sigmask) {
            int timeout = -1;
            bool did_work = _preempting_io.service_preempting_io();
            if (did_work) {
                timeout = 0;
            }

            _hrtimer_poll_completion.maybe_queue(_polling_io);
            _smp_wakeup_aio_completion.maybe_queue(_polling_io);
            _polling_io.flush();
            await_events(timeout, active_sigmask);
            _preempting_io.service_preempting_io();    // clear task quota timer
        }

        future<> reactor_backend_aio::poll(pollable_fd_state &fd, int events) {
            try {
                if (events & fd.events_known) {
                    fd.events_known &= ~events;
                    return make_ready_future<>();
                }

                fd.events_rw = events == (POLLIN | POLLOUT);

                auto *pfd = static_cast<aio_pollable_fd_state *>(&fd);
                auto *iocb = pfd->get_iocb(events);
                auto *desc = pfd->get_desc(events);
                *iocb = make_poll_iocb(fd.fd.get(), events);
                *desc = pollable_fd_state_completion {};
                set_user_data(*iocb, desc);
                _polling_io.queue(iocb);
                return pfd->get_completion_future(events);
            } catch (...) {
                return make_exception_future<>(std::current_exception());
            }
        }

        future<> reactor_backend_aio::readable(pollable_fd_state &fd) {
            return poll(fd, POLLIN);
        }

        future<> reactor_backend_aio::writeable(pollable_fd_state &fd) {
            return poll(fd, POLLOUT);
        }

        future<> reactor_backend_aio::readable_or_writeable(pollable_fd_state &fd) {
            return poll(fd, POLLIN | POLLOUT);
        }

        void reactor_backend_aio::forget(pollable_fd_state &fd) noexcept {
            auto *pfd = static_cast<aio_pollable_fd_state *>(&fd);
            delete pfd;
            // ?
        }

        future<std::tuple<pollable_fd, socket_address>> reactor_backend_aio::accept(pollable_fd_state &listenfd) {
            return engine().do_accept(listenfd);
        }

        future<> reactor_backend_aio::connect(pollable_fd_state &fd, socket_address &sa) {
            return engine().do_connect(fd, sa);
        }

        void reactor_backend_aio::shutdown(pollable_fd_state &fd, int how) {
            fd.fd.shutdown(how);
        }

        future<size_t> reactor_backend_aio::read_some(pollable_fd_state &fd, void *buffer, size_t len) {
            return engine().do_read_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_aio::read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) {
            return engine().do_read_some(fd, iov);
        }

        future<temporary_buffer<char>> reactor_backend_aio::read_some(pollable_fd_state &fd,
                                                                      detail::buffer_allocator *ba) {
            return engine().do_read_some(fd, ba);
        }

        future<size_t> reactor_backend_aio::write_some(pollable_fd_state &fd, const void *buffer, size_t len) {
            return engine().do_write_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_aio::write_some(pollable_fd_state &fd, net::packet &p) {
            return engine().do_write_some(fd, p);
        }

        void reactor_backend_aio::start_tick() {
            _preempting_io.start_tick();
        }

        void reactor_backend_aio::stop_tick() {
            _preempting_io.stop_tick();
        }

        void reactor_backend_aio::arm_highres_timer(const ::itimerspec &its) {
            _hrtimer_timerfd.timerfd_settime(TFD_TIMER_ABSTIME, its);
        }

        void reactor_backend_aio::reset_preemption_monitor() {
            _preempting_io.reset_preemption_monitor();
        }

        void reactor_backend_aio::request_preemption() {
            _preempting_io.request_preemption();
        }

        void reactor_backend_aio::start_handling_signal() {
            // The aio backend only uses SIGHUP/SIGTERM/SIGINT. We don't need to handle them right away and our
            // implementation of request_preemption is not signal safe, so do nothing.
        }

        pollable_fd_state_ptr reactor_backend_aio::make_pollable_fd_state(file_desc fd,
                                                                          pollable_fd::speculation speculate) {
            return pollable_fd_state_ptr(new aio_pollable_fd_state(std::move(fd), std::move(speculate)));
        }
    }    // namespace actor
}    // namespace nil
