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

#include <poll.h>

#include <nil/actor/core/detail/reactor_backend.hh>

namespace nil {
    namespace actor {
        class aio_pollable_fd_state : public pollable_fd_state {
            detail::linux_abi::iocb _iocb_pollin;
            pollable_fd_state_completion _completion_pollin;

            detail::linux_abi::iocb _iocb_pollout;
            pollable_fd_state_completion _completion_pollout;

        public:
            pollable_fd_state_completion *get_desc(int events) {
                if (events & POLLIN) {
                    return &_completion_pollin;
                }
                return &_completion_pollout;
            }
            detail::linux_abi::iocb *get_iocb(int events) {
                if (events & POLLIN) {
                    return &_iocb_pollin;
                }
                return &_iocb_pollout;
            }
            explicit aio_pollable_fd_state(file_desc fd, speculation speculate) :
                pollable_fd_state(std::move(fd), std::move(speculate)) {
            }
            future<> get_completion_future(int events) {
                return get_desc(events)->get_future();
            }
        };

        class aio_storage_context {
            static constexpr unsigned max_aio = 1024;

            class iocb_pool {
                alignas(cache_line_size) std::array<detail::linux_abi::iocb, max_aio> _iocb_pool;
                std::stack<detail::linux_abi::iocb *,
                           boost::container::static_vector<detail::linux_abi::iocb *, max_aio>>
                    _free_iocbs;

            public:
                iocb_pool();
                detail::linux_abi::iocb &get_one();
                void put_one(detail::linux_abi::iocb *io);
                unsigned outstanding() const;
                bool has_capacity() const;
            };

            reactor &_r;
            detail::linux_abi::aio_context_t _io_context;
            boost::container::static_vector<detail::linux_abi::iocb *, max_aio> _submission_queue;
            iocb_pool _iocb_pool;
            size_t handle_aio_error(detail::linux_abi::iocb *iocb, int ec);
            using pending_aio_retry_t = boost::container::static_vector<detail::linux_abi::iocb *, max_aio>;
            pending_aio_retry_t _pending_aio_retry;
            detail::linux_abi::io_event _ev_buffer[max_aio];

        public:
            explicit aio_storage_context(reactor &r);
            ~aio_storage_context();

            bool reap_completions();
            void schedule_retry();
            bool submit_work();
            bool can_sleep() const;
        };

        // FIXME: merge it with storage context below. At this point the
        // main thing to do is unify the iocb list
        struct aio_general_context {
            explicit aio_general_context(size_t nr);
            ~aio_general_context();
            detail::linux_abi::aio_context_t io_context {};
            std::unique_ptr<detail::linux_abi::iocb *[]> iocbs;
            detail::linux_abi::iocb **last = iocbs.get();
            void queue(detail::linux_abi::iocb *iocb);
            size_t flush();
        };

        class completion_with_iocb {
            bool _in_context = false;
            detail::linux_abi::iocb _iocb;

        protected:
            completion_with_iocb(int fd, int events, void *user_data);
            void completed() {
                _in_context = false;
            }

        public:
            void maybe_queue(aio_general_context &context);
        };

        class fd_kernel_completion : public kernel_completion {
        protected:
            reactor &_r;
            file_desc &_fd;
            fd_kernel_completion(reactor &r, file_desc &fd) : _r(r), _fd(fd) {
            }

        public:
            file_desc &fd() {
                return _fd;
            }
        };

        struct hrtimer_aio_completion : public fd_kernel_completion, public completion_with_iocb {
            hrtimer_aio_completion(reactor &r, file_desc &fd);
            virtual void complete_with(ssize_t value) override;
        };

        struct task_quota_aio_completion : public fd_kernel_completion, public completion_with_iocb {
            task_quota_aio_completion(reactor &r, file_desc &fd);
            virtual void complete_with(ssize_t value) override;
        };

        struct smp_wakeup_aio_completion : public fd_kernel_completion, public completion_with_iocb {
            smp_wakeup_aio_completion(reactor &r, file_desc &fd);
            virtual void complete_with(ssize_t value) override;
        };

        // Common aio-based Implementation of the task quota and hrtimer.
        class preempt_io_context {
            reactor &_r;
            aio_general_context _context {2};

            task_quota_aio_completion _task_quota_aio_completion;
            hrtimer_aio_completion _hrtimer_aio_completion;

        public:
            preempt_io_context(reactor &r, file_desc &task_quota, file_desc &hrtimer);
            bool service_preempting_io();

            size_t flush() {
                return _context.flush();
            }

            void reset_preemption_monitor();
            void request_preemption();
            void start_tick();
            void stop_tick();
        };

        class reactor_backend_aio : public reactor_backend {
            static constexpr size_t max_polls = 10000;
            reactor &_r;
            file_desc _hrtimer_timerfd;
            aio_storage_context _storage_context;
            // We use two aio contexts, one for preempting events (the timer tick and
            // signals), the other for non-preempting events (fd poll).
            preempt_io_context _preempting_io;              // Used for the timer tick and the high resolution timer
            aio_general_context _polling_io {max_polls};    // FIXME: unify with disk aio_context
            hrtimer_aio_completion _hrtimer_poll_completion;
            smp_wakeup_aio_completion _smp_wakeup_aio_completion;
            static file_desc make_timerfd();
            bool await_events(int timeout, const sigset_t *active_sigmask);

        public:
            explicit reactor_backend_aio(reactor &r);

            virtual bool reap_kernel_completions() override;
            virtual bool kernel_submit_work() override;
            virtual bool kernel_events_can_sleep() const override;
            virtual void wait_and_process_events(const sigset_t *active_sigmask) override;
            future<> poll(pollable_fd_state &fd, int events);
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
