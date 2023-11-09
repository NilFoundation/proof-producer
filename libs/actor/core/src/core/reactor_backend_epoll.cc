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

#include <nil/actor/core/detail/reactor_backend_epoll.hh>
#include <nil/actor/core/detail/buffer_allocator.hh>
#include <nil/actor/core/detail/thread_pool.hh>
#include <nil/actor/core/detail/syscall_result.hh>

#if BOOST_OS_LINUX
#include <nil/actor/core/detail/reactor_backend_aio.hh>
#endif

#include <nil/actor/core/print.hh>
#include <nil/actor/core/reactor.hh>

#include <nil/actor/detail/defer.hh>
#include <nil/actor/detail/read_first_line.hh>

#include <chrono>

namespace nil {
    namespace actor {

        using namespace std::chrono_literals;
        using namespace detail;
        using namespace detail::linux_abi;

        reactor_backend_epoll::reactor_backend_epoll(reactor &r) :
            _r(r),
            _steady_clock_timer_reactor_thread(file_desc::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
            _steady_clock_timer_timer_thread(file_desc::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
            _epollfd(file_desc::epoll_create(EPOLL_CLOEXEC))
#if BOOST_OS_LINUX
            ,
            _storage_context(_r)
#endif

        {
            _highres_timer_pending.store(false, std::memory_order_relaxed);

            ::epoll_event event;
            event.events = EPOLLIN;
            event.data.ptr = nullptr;
            auto ret = ::epoll_ctl(_epollfd.get(), EPOLL_CTL_ADD, _r._notify_eventfd.get(), &event);
            throw_system_error_on(ret == -1);

            event.events = EPOLLIN;
            event.data.ptr = &_steady_clock_timer_reactor_thread;
            ret = ::epoll_ctl(_epollfd.get(), EPOLL_CTL_ADD, _steady_clock_timer_reactor_thread.get(), &event);
            throw_system_error_on(ret == -1);
        }

        reactor_backend_epoll::~reactor_backend_epoll() = default;

        void reactor_backend_epoll::start_tick() {
            _task_quota_timer_thread = std::thread(&reactor_backend_epoll::task_quota_timer_thread_fn, this);

            ::sched_param sp;
            sp.sched_priority = 1;
            auto sched_ok = pthread_setschedparam(_task_quota_timer_thread.native_handle(), SCHED_FIFO, &sp);
            if (sched_ok != 0 && _r._id == 0) {
                actor_logger.warn(
                    "Unable to set SCHED_FIFO scheduling policy for timer thread; latency impact possible. Try adding "
                    "CAP_SYS_NICE");
            }
        }

        void reactor_backend_epoll::stop_tick() {
            _r._dying.store(true, std::memory_order_relaxed);
            _r._task_quota_timer.timerfd_settime(
                0, nil::actor::posix::to_relative_itimerspec(1ns, 1ms));    // Make the timer fire soon
            _task_quota_timer_thread.join();
        }

        void reactor_backend_epoll::arm_highres_timer(const ::itimerspec &its) {
            _steady_clock_timer_deadline = its;
            _steady_clock_timer_timer_thread.timerfd_settime(TFD_TIMER_ABSTIME, its);
        }

        void reactor_backend_epoll::switch_steady_clock_timers(file_desc &from, file_desc &to) {
            auto &deadline = _steady_clock_timer_deadline;
            if (deadline.it_value.tv_sec == 0 && deadline.it_value.tv_nsec == 0) {
                return;
            }
            // Enable-then-disable, so the hardware timer doesn't have to be reprogrammed. Probably pointless.
            to.timerfd_settime(TFD_TIMER_ABSTIME, _steady_clock_timer_deadline);
            from.timerfd_settime(TFD_TIMER_ABSTIME, {});
        }

        void reactor_backend_epoll::maybe_switch_steady_clock_timers(int timeout, file_desc &from, file_desc &to) {
            if (timeout != 0) {
                switch_steady_clock_timers(from, to);
            }
        }

        void reactor_backend_epoll::task_quota_timer_thread_fn() {
            auto thread_name = nil::actor::format("timer-{}", _r._id);
            detail::set_thread_name(pthread_setname_np, thread_name.c_str());

            sigset_t mask;
            sigfillset(&mask);
            for (auto sig : {SIGSEGV}) {
                sigdelset(&mask, sig);
            }
            auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
            if (r) {
                actor_logger.error("Thread {}: failed to block signals. Aborting.", thread_name.c_str());
                abort();
            }

            // We need to wait until task quota is set before we can calculate how many ticks are to
            // a minute. Technically task_quota is used from many threads, but since it is read-only here
            // and only used during initialization we will avoid complicating the code.
            {
                uint64_t events;
#if BOOST_OS_LINUX
                _r._task_quota_timer.read(&events, sizeof(uint64_t));
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                r = ::timerfd_read(_r._task_quota_timer.get(), &events, sizeof(uint64_t));
                throw_system_error_on(r == -1);
#endif
                request_preemption();
            }

            while (!_r._dying.load(std::memory_order_relaxed)) {
                // Wait for either the task quota timer, or the high resolution timer, or both,
                // to expire.
                struct pollfd pfds[2] = {};
                pfds[0].fd = _r._task_quota_timer.get();
                pfds[0].events = POLL_IN;
                pfds[1].fd = _steady_clock_timer_timer_thread.get();
                pfds[1].events = POLL_IN;
                int r = poll(pfds, 2, -1);
                assert(r != -1);

                uint64_t events;
                if (pfds[0].revents & POLL_IN) {
#if BOOST_OS_LINUX
                    _r._task_quota_timer.read(&events, sizeof(uint64_t));
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    r = ::timerfd_read(_r._task_quota_timer.get(), &events, sizeof(uint64_t));
                    throw_system_error_on(r == -1);
#endif
                }
                if (pfds[1].revents & POLL_IN) {
#if BOOST_OS_LINUX
                    _steady_clock_timer_timer_thread.read(&events, sizeof(uint64_t));
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    r = ::timerfd_read(_steady_clock_timer_timer_thread.get(), &events, sizeof(uint64_t));
                    throw_system_error_on(r == -1);
#endif
                    _highres_timer_pending.store(true, std::memory_order_relaxed);
                }
                request_preemption();

                // We're in a different thread, but guaranteed to be on the same core, so even
                // a signal fence is overdoing it
                std::atomic_signal_fence(std::memory_order_seq_cst);
            }
        }

        bool reactor_backend_epoll::wait_and_process(int timeout, const sigset_t *active_sigmask) {
            // If we plan to sleep, disable the timer thread steady clock timer (since it won't
            // wake us up from sleep, and timer thread wakeup will just waste CPU time) and enable
            // reactor thread steady clock timer.
            maybe_switch_steady_clock_timers(
                timeout, _steady_clock_timer_timer_thread, _steady_clock_timer_reactor_thread);
            auto undo_timer_switch = defer([&] {
                maybe_switch_steady_clock_timers(
                    timeout, _steady_clock_timer_reactor_thread, _steady_clock_timer_timer_thread);
            });

            std::array<epoll_event, 128> eevt {};
            int nr = ::epoll_pwait(_epollfd.get(), eevt.data(), eevt.size(), timeout, active_sigmask);
            if (nr == -1 && errno == EINTR) {
                return false;    // gdb can cause this
            }
            assert(nr != -1);
            for (int i = 0; i < nr; ++i) {
                auto &evt = eevt[i];
                auto pfd = reinterpret_cast<pollable_fd_state *>(evt.data.ptr);
                if (!pfd) {
                    char dummy[8];
#if BOOST_OS_LINUX
                    _r._notify_eventfd.read(dummy, 8);
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    ::eventfd_read(_r._notify_eventfd.get(), reinterpret_cast<uint64_t *>(&dummy));
#endif
                    continue;
                }
                if (evt.data.ptr == &_steady_clock_timer_reactor_thread) {
                    char dummy[8];
#if BOOST_OS_LINUX
                    _steady_clock_timer_reactor_thread.read(dummy, sizeof(uint64_t));
#else
                    int r = ::timerfd_read(_steady_clock_timer_timer_thread.get(), &dummy, sizeof(uint64_t));
                    throw_system_error_on(r == -1);
#endif
                    _highres_timer_pending.store(true, std::memory_order_relaxed);
                    _steady_clock_timer_deadline = {};
                    continue;
                }
                if (evt.events & (EPOLLHUP | EPOLLERR)) {
                    // treat the events as required events when error occurs, let
                    // send/recv/accept/connect handle the specific error.
                    evt.events = pfd->events_requested;
                }
                auto events = evt.events & (EPOLLIN | EPOLLOUT);
                auto events_to_remove = events & ~pfd->events_requested;
                if (pfd->events_rw) {
                    // accept() signals normal completions via EPOLLIN, but errors (due to shutdown())
                    // via EPOLLOUT|EPOLLHUP, so we have to wait for both EPOLLIN and EPOLLOUT with the
                    // same future
                    complete_epoll_event(*pfd, events, EPOLLIN | EPOLLOUT);
                } else {
                    // Normal processing where EPOLLIN and EPOLLOUT are waited for via different
                    // futures.
                    complete_epoll_event(*pfd, events, EPOLLIN);
                    complete_epoll_event(*pfd, events, EPOLLOUT);
                }
                if (events_to_remove) {
                    pfd->events_epoll &= ~events_to_remove;
                    evt.events = pfd->events_epoll;
                    auto op = evt.events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                    ::epoll_ctl(_epollfd.get(), op, pfd->fd.get(), &evt);
                }
            }
            return nr;
        }

        bool reactor_backend_epoll::reap_kernel_completions() {
#if BOOST_OS_LINUX
            // epoll does not have a separate submission stage, and just
            // calls epoll_ctl everytime it needs, so this method and
            // kernel_submit_work are essentially the same. Ordering also
            // doesn't matter much. wait_and_process is actually completing,
            // but we prefer to call it in kernel_submit_work because the
            // reactor register two pollers for completions and one for submission,
            // since completion is cheaper for other backends like aio. This avoids
            // calling epoll_wait twice.
            //
            // We will only reap the io completions
            return _storage_context.reap_completions();
#endif
            return true;
        }

        bool reactor_backend_epoll::kernel_submit_work() {
            bool result = false;
#if BOOST_OS_LINUX
            _storage_context.submit_work();
#endif
            if (_need_epoll_events) {
                result |= wait_and_process(0, nullptr);
            }

            result |= complete_hrtimer();

            return result;
        }

        bool reactor_backend_epoll::complete_hrtimer() {
            // This can be set from either the task quota timer thread, or
            // wait_and_process(), above.
            if (_highres_timer_pending.load(std::memory_order_relaxed)) {
                _highres_timer_pending.store(false, std::memory_order_relaxed);
                _r.service_highres_timer();
                return true;
            }
            return false;
        }

        bool reactor_backend_epoll::kernel_events_can_sleep() const {
#if BOOST_OS_LINUX
            return _storage_context.can_sleep();
#endif
            return true;
        }

        void reactor_backend_epoll::wait_and_process_events(const sigset_t *active_sigmask) {
            wait_and_process(-1, active_sigmask);
            complete_hrtimer();
        }

        void reactor_backend_epoll::complete_epoll_event(pollable_fd_state &pfd, int events, int event) {
            if (pfd.events_requested & events & event) {
                pfd.events_requested &= ~event;
                pfd.events_known &= ~event;
                auto *fd = static_cast<epoll_pollable_fd_state *>(&pfd);
                return fd->complete_with(event);
            }
        }

        void reactor_backend_epoll::signal_received(int signo, siginfo_t *siginfo, void *ignore) {
            if (engine_is_ready()) {
                engine()._signals.action(signo, siginfo, ignore);
            } else {
                reactor::signals::failed_to_handle(signo);
            }
        }

        future<> reactor_backend_epoll::get_epoll_future(pollable_fd_state &pfd, int event) {
            if (pfd.events_known & event) {
                pfd.events_known &= ~event;
                return make_ready_future();
            }
            pfd.events_rw = event == (EPOLLIN | EPOLLOUT);
            pfd.events_requested |= event;
            if ((pfd.events_epoll & event) != event) {
                auto ctl = pfd.events_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
                pfd.events_epoll |= event;
                ::epoll_event eevt;
                eevt.events = pfd.events_epoll;
                eevt.data.ptr = &pfd;
                int r = ::epoll_ctl(_epollfd.get(), ctl, pfd.fd.get(), &eevt);
                assert(r == 0);
                _need_epoll_events = true;
            }

            auto *fd = static_cast<epoll_pollable_fd_state *>(&pfd);
            return fd->get_completion_future(event);
        }

        future<> reactor_backend_epoll::readable(pollable_fd_state &fd) {
            return get_epoll_future(fd, EPOLLIN);
        }

        future<> reactor_backend_epoll::writeable(pollable_fd_state &fd) {
            return get_epoll_future(fd, EPOLLOUT);
        }

        future<> reactor_backend_epoll::readable_or_writeable(pollable_fd_state &fd) {
            return get_epoll_future(fd, EPOLLIN | EPOLLOUT);
        }

        void reactor_backend_epoll::forget(pollable_fd_state &fd) noexcept {
            if (fd.events_epoll) {
                ::epoll_ctl(_epollfd.get(), EPOLL_CTL_DEL, fd.fd.get(), nullptr);
            }
            auto *efd = static_cast<epoll_pollable_fd_state *>(&fd);
            delete efd;
        }

        future<std::tuple<pollable_fd, socket_address>> reactor_backend_epoll::accept(pollable_fd_state &listenfd) {
            return engine().do_accept(listenfd);
        }

        future<> reactor_backend_epoll::connect(pollable_fd_state &fd, socket_address &sa) {
            return engine().do_connect(fd, sa);
        }

        void reactor_backend_epoll::shutdown(pollable_fd_state &fd, int how) {
            fd.fd.shutdown(how);
        }

        future<size_t> reactor_backend_epoll::read_some(pollable_fd_state &fd, void *buffer, size_t len) {
            return engine().do_read_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_epoll::read_some(pollable_fd_state &fd, const std::vector<iovec> &iov) {
            return engine().do_read_some(fd, iov);
        }

        future<temporary_buffer<char>> reactor_backend_epoll::read_some(pollable_fd_state &fd,
                                                                        detail::buffer_allocator *ba) {
            return engine().do_read_some(fd, ba);
        }

        future<size_t> reactor_backend_epoll::write_some(pollable_fd_state &fd, const void *buffer, size_t len) {
            return engine().do_write_some(fd, buffer, len);
        }

        future<size_t> reactor_backend_epoll::write_some(pollable_fd_state &fd, net::packet &p) {
            return engine().do_write_some(fd, p);
        }

        void reactor_backend_epoll::request_preemption() {
            _r._preemption_monitor.head.store(1, std::memory_order_relaxed);
        }

        void reactor_backend_epoll::start_handling_signal() {
            // The epoll backend uses signals for the high resolution timer. That is used for thread_scheduling_group,
            // so we request preemption so when we receive a signal.
            request_preemption();
        }

        pollable_fd_state_ptr reactor_backend_epoll::make_pollable_fd_state(file_desc fd,
                                                                            pollable_fd::speculation speculate) {
            return pollable_fd_state_ptr(new epoll_pollable_fd_state(std::move(fd), std::move(speculate)));
        }

        void reactor_backend_epoll::reset_preemption_monitor() {
            _r._preemption_monitor.head.store(0, std::memory_order_relaxed);
        }
    }    // namespace actor
}    // namespace nil
