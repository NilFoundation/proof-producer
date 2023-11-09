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

#include <nil/actor/core/posix.hh>
#include <nil/actor/core/linux-aio.hh>
#include <nil/actor/core/fsqual.hh>

#include <nil/actor/detail/defer.hh>

#include <sys/time.h>
#include <sys/resource.h>

#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <type_traits>

namespace nil {
    namespace actor {

        using namespace nil::actor::detail;
        using namespace nil::actor::detail::linux_abi;

        // Runs func(), and also adds the number of context switches
        // that happened during func() to counter.
        template<typename Counter, typename Func>
        typename std::result_of<Func()>::type with_ctxsw_counting(Counter &counter, Func &&func) {
            struct count_guard {
                Counter &counter;
                count_guard(Counter &counter) : counter(counter) {
                    counter -= nvcsw();
                }
                ~count_guard() {
                    counter += nvcsw();
                }
                static Counter nvcsw() {
                    struct rusage usage;
#if BOOST_OS_LINUX
                    getrusage(RUSAGE_THREAD, &usage);
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    detail::getrusage_thread(&usage);
#endif
                    return usage.ru_nvcsw;
                }
            };
            count_guard g(counter);
            return func();
        }

        bool filesystem_has_good_aio_support(sstring directory, bool verbose) {
            aio_context_t ioctx = {};
            auto r = io_setup(1, &ioctx);
            throw_system_error_on(r == -1, "io_setup");
            auto cleanup = defer([&] { io_destroy(ioctx); });
            auto fname = directory + "/fsqual.tmp";
#if BOOST_OS_LINUX || BOOST_OS_BSD
            auto fd = file_desc::open(fname, O_CREAT | O_EXCL | O_RDWR | O_DIRECT, 0600);
#elif BOOST_OS_MACOS || BOOST_OS_IOS
            auto fd = file_desc::open(fname, O_CREAT | O_EXCL | O_RDWR, 0600);
            // Not an ultimate solution for Darwin-based systems accroding
            // to https://github.com/libuv/libuv/issues/1600
            fcntl(fd.get(), F_NOCACHE, 1);
#endif
            unlink(fname.c_str());
            auto nr = 1000;
            fd.truncate(nr * 4096);
            auto bufsize = 4096;
            auto ctxsw = 0;
            auto buf = aligned_alloc(4096, 4096);
            for (int i = 0; i < nr; ++i) {
                struct iocb cmd;
                cmd = make_write_iocb(fd.get(), bufsize * i, buf, bufsize);
                struct iocb *cmds[1] = {&cmd};
                with_ctxsw_counting(ctxsw, [&] {
                    auto r = io_submit(ioctx, 1, cmds);
                    throw_system_error_on(r == -1, "io_submit");
                    assert(r == 1);
                });
                struct io_event ioev;
                int n = -1;
                do {
                    n = io_getevents(ioctx, 1, 1, &ioev, nullptr);
                    throw_system_error_on((n == -1) && (errno != EINTR), "io_getevents");
                } while (n == -1);
                assert(n == 1);
                throw_kernel_error(long(ioev.res));
                assert(long(ioev.res) == bufsize);
            }
            auto rate = float(ctxsw) / nr;
            bool ok = rate < 0.1;
            if (verbose) {
                auto verdict = ok ? "GOOD" : "BAD";
                std::cout << "context switch per appending io: " << rate << " (" << verdict << ")\n";
            }
            return ok;
        }

    }    // namespace actor
}    // namespace nil
