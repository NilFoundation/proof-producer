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

#include <nil/actor/core/systemwide_memory_barrier.hh>
#include <nil/actor/core/cacheline.hh>
#include <nil/actor/detail/log.hh>

#include <sys/mman.h>
#include <unistd.h>
#include <cassert>
#include <atomic>
#include <mutex>

#if ACTOR_HAS_MEMBARRIER
#include <linux/membarrier.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace nil {
    namespace actor {

#ifdef ACTOR_HAS_MEMBARRIER

        static bool has_native_membarrier = [] {
            auto r = syscall(SYS_membarrier, MEMBARRIER_CMD_QUERY, 0);
            if (r == -1) {
                return false;
            }
            int needed = MEMBARRIER_CMD_PRIVATE_EXPEDITED | MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED;
            if ((r & needed) != needed) {
                return false;
            }
            syscall(SYS_membarrier, MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0);
            return true;
        }();

        static bool try_native_membarrier() {
            if (has_native_membarrier) {
                syscall(SYS_membarrier, MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0);
                return true;
            }
            return false;
        }

#else

        static bool try_native_membarrier() {
            return false;
        }

#endif

        // cause all threads to invoke a full memory barrier
        void systemwide_memory_barrier() {
            if (try_native_membarrier()) {
                return;
            }

            // FIXME: use sys_membarrier() when available
            static thread_local char *mem = [] {
                void *mem = mmap(nullptr, getpagesize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                assert(mem != MAP_FAILED);

                // If the user specified --lock-memory, then madvise() below will fail
                // with EINVAL, so we unlock here:
                auto r = munlock(mem, getpagesize());
                // munlock may fail on old kernels if we don't have permission. That's not
                // a problem, since if we don't have permission to unlock, we didn't have
                // permissions to lock.
                assert(r == 0 || errno == EPERM);

                return reinterpret_cast<char *>(mem);
            }();
            // Force page into memory to make madvise() have real work to do
            *mem = 3;
            // Evict page to force kernel to send IPI to all threads, with
            // a side effect of executing a memory barrier on those threads
            // FIXME: does this work on ARM?
            int r2 = madvise(mem, getpagesize(), MADV_DONTNEED);
            assert(r2 == 0);
        }

        bool try_systemwide_memory_barrier() {
            if (try_native_membarrier()) {
                return true;
            }

#ifdef __aarch64__

            // Some (not all) ARM processors can broadcast TLB invalidations using the
            // TLBI instruction. On those, the mprotect trick won't work.
            static std::once_flag warn_once;
            extern logger actor_logger;
            std::call_once(warn_once, [] {
                actor_logger.warn(
                    "membarrier(MEMBARRIER_CMD_PRIVATE_EXPEDITED) is not available, reactor will not sleep when idle. "
                    "Upgrade to Linux 4.14 or later");
            });

            return false;

#endif

            systemwide_memory_barrier();
            return true;
        }

    }    // namespace actor
}    // namespace nil
