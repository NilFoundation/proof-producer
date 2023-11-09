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

#include <nil/actor/core/detail/reactor_backend_selector.hh>
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

#if BOOST_OS_LINUX
        static bool detect_aio_poll() {
            auto fd = file_desc::eventfd(0, 0);
            detail::linux_abi::aio_context_t ioc {};
            detail::setup_aio_context(1, &ioc);
            auto cleanup = defer([&] { detail::io_destroy(ioc); });
            detail::linux_abi::iocb iocb = detail::make_poll_iocb(fd.get(), POLLIN | POLLOUT);
            detail::linux_abi::iocb *a[1] = {&iocb};
            auto r = detail::io_submit(ioc, 1, a);
            if (r != 1) {
                return false;
            }
            uint64_t one = 1;
            fd.write(&one, 8);
            detail::linux_abi::io_event ev[1];
            // We set force_syscall = true (the last parameter) to ensure
            // the system call exists and is usable. If IOCB_CMD_POLL exists then
            // io_pgetevents() will also exist, but some versions of docker
            // have a syscall whitelist that does not include io_pgetevents(),
            // which causes it to fail with -EPERM. See
            // https://github.com/moby/moby/issues/38894.
            r = detail::io_pgetevents(ioc, 1, 1, ev, nullptr, nullptr, true);
            return r == 1;
        }
#endif

        bool reactor_backend_selector::has_enough_aio_nr() {
#if BOOST_OS_LINUX
            auto aio_max_nr = read_first_line_as<unsigned>("/proc/sys/fs/aio-max-nr");
            auto aio_nr = read_first_line_as<unsigned>("/proc/sys/fs/aio-nr");
            /* reactor_backend_selector::available() will be execute in early stage,
             * it's before io_setup() issued, and not per-cpu basis.
             * So this method calculates:
             *  Available AIO on the system - (request AIO per-cpu * ncpus)
             */
            if (aio_max_nr - aio_nr < reactor::max_aio * smp::count) {
                return false;
            }
#endif
            return true;
        }

        std::unique_ptr<reactor_backend> reactor_backend_selector::create(reactor &r) {
#if BOOST_OS_LINUX
            if (_name == "linux-aio") {
                return std::make_unique<reactor_backend_aio>(r);
            } else if (_name == "epoll") {
                return std::make_unique<reactor_backend_epoll>(r);
            }
#else
            if (_name == "epoll") {
                return std::make_unique<reactor_backend_epoll>(r);
            }
#endif
            throw std::logic_error("bad reactor backend");
        }

        reactor_backend_selector reactor_backend_selector::default_backend() {
            return available()[0];
        }

        std::vector<reactor_backend_selector> reactor_backend_selector::available() {
            std::vector<reactor_backend_selector> ret;
#if BOOST_OS_LINUX
            if (detect_aio_poll() && has_enough_aio_nr()) {
                ret.push_back(reactor_backend_selector("linux-aio"));
            }
#endif
            ret.push_back(reactor_backend_selector("epoll"));
            return ret;
        }

    }    // namespace actor
}    // namespace nil
