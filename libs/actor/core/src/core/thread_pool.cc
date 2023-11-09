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

#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/detail/thread_pool.hh>

namespace nil {
    namespace actor {

/* not yet implemented for OSv. TODO: do the notification like we do class smp. */
#ifndef HAVE_OSV
        thread_pool::thread_pool(reactor *r, const sstring &name) :
            _reactor(r), _worker_thread([this, name] { work(name); }) {
        }

        void thread_pool::work(const sstring &name) {
            detail::set_thread_name(pthread_setname_np, name.c_str());
            sigset_t mask;
            sigfillset(&mask);
            auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
            throw_pthread_error(r);
            std::array<syscall_work_queue::work_item *, syscall_work_queue::queue_length> tmp_buf;
            while (true) {
                uint64_t count;
#if BOOST_OS_LINUX
                r = ::read(inter_thread_wq._start_eventfd.get_read_fd(), &count, sizeof(count));
#else
                r = epoll_shim_read(inter_thread_wq._start_eventfd.get_read_fd(), &count, sizeof(count));
#endif
                assert(r == sizeof(count));
                if (_stopped.load(std::memory_order_relaxed)) {
                    break;
                }
                auto end = tmp_buf.data();
                inter_thread_wq._pending.consume_all([&](syscall_work_queue::work_item *wi) { *end++ = wi; });
                for (auto p = tmp_buf.data(); p != end; ++p) {
                    auto wi = *p;
                    wi->process();
                    inter_thread_wq._completed.push(wi);
                }
                if (_main_thread_idle.load(std::memory_order_seq_cst)) {
                    uint64_t one = 1;
#if BOOST_OS_LINUX
                    ::write(_reactor->_notify_eventfd.get(), &one, 8);
#else
                    epoll_shim_write(_reactor->_notify_eventfd.get(), &one, 8);
#endif
                }
            }
        }

        thread_pool::~thread_pool() {
            _stopped.store(true, std::memory_order_relaxed);
            inter_thread_wq._start_eventfd.signal(1);
            _worker_thread.join();
        }
#endif

    }    // namespace actor
}    // namespace nil
