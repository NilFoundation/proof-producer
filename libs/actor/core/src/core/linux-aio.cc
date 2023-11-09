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

#include <nil/actor/core/linux-aio.hh>
#include <nil/actor/core/print.hh>

#include <unistd.h>
#include <sys/syscall.h>

#include <atomic>
#include <algorithm>
#include <cerrno>
#include <cstring>

#ifdef ACTOR_HAS_VALGRIND
#include <valgrind/valgrind.h>
#endif

namespace nil {
    namespace actor {

        namespace detail {

            namespace linux_abi {

                struct linux_aio_ring {
                    uint32_t id;
                    uint32_t nr;
                    std::atomic<uint32_t> head;
                    std::atomic<uint32_t> tail;
                    uint32_t magic;
                    uint32_t compat_features;
                    uint32_t incompat_features;
                    uint32_t header_length;
                };

            }    // namespace linux_abi

            using namespace linux_abi;

            static linux_aio_ring *to_ring(aio_context_t io_context) {
                return reinterpret_cast<linux_aio_ring *>(uintptr_t(io_context));
            }

            static bool usable(const linux_aio_ring *ring) {
#ifdef ACTOR_HAS_VALGRIND
                return ring->magic == 0xa10a10a1 && ring->incompat_features == 0 && !RUNNING_ON_VALGRIND;
#else
                return ring->magic == 0xa10a10a1 && ring->incompat_features == 0;
#endif
            }

            int io_setup(int nr_events, aio_context_t *io_context) {
                return ::syscall(SYS_io_setup, nr_events, io_context);
            }

            int io_destroy(aio_context_t io_context) {
                return ::syscall(SYS_io_destroy, io_context);
            }

            int io_submit(aio_context_t io_context, long nr, iocb **iocbs) {
                return ::syscall(SYS_io_submit, io_context, nr, iocbs);
            }

            int io_cancel(aio_context_t io_context, iocb *iocb, io_event *result) {
                return ::syscall(SYS_io_cancel, io_context, iocb, result);
            }

            static int try_reap_events(aio_context_t io_context, long min_nr, long nr, io_event *events,
                                       const ::timespec *timeout, bool force_syscall) {
                auto ring = to_ring(io_context);
                if (usable(ring) && !force_syscall) {
                    // Try to complete in userspace, if enough available events,
                    // or if the timeout is zero

                    // We're the only writer to ->head, so we can load with memory_order_relaxed (assuming
                    // only a single thread calls io_getevents()).
                    auto head = ring->head.load(std::memory_order_relaxed);
                    // The kernel will write to the ring from an interrupt and then release with a write
                    // to ring->tail, so we must memory_order_acquire here.
                    auto tail = ring->tail.load(std::memory_order_acquire);    // kernel writes from interrupts
                    auto available = tail - head;
                    if (tail < head) {
                        available += ring->nr;
                    }
                    if (available >= uint32_t(min_nr) || (timeout && timeout->tv_sec == 0 && timeout->tv_nsec == 0)) {
                        if (!available) {
                            return 0;
                        }
                        auto ring_events =
                            reinterpret_cast<const io_event *>(uintptr_t(io_context) + ring->header_length);
                        auto now = std::min<uint32_t>(nr, available);
                        auto start = ring_events + head;
                        head += now;
                        if (head < ring->nr) {
                            std::copy(start, start + now, events);
                        } else {
                            head -= ring->nr;
                            auto p = std::copy(start, ring_events + ring->nr, events);
                            std::copy(ring_events, ring_events + head, p);
                        }
                        // The kernel will read ring->head and update its view of how many entries
                        // in the ring are available, so memory_order_release to make sure any ring
                        // accesses are completed before the update to ring->head is visible.
                        ring->head.store(head, std::memory_order_release);
                        return now;
                    }
                }
                return -1;
            }

            int io_getevents(aio_context_t io_context, long min_nr, long nr, io_event *events,
                             const ::timespec *timeout, bool force_syscall) {
                auto r = try_reap_events(io_context, min_nr, nr, events, timeout, force_syscall);
                if (r >= 0) {
                    return r;
                }
                return ::syscall(SYS_io_getevents, io_context, min_nr, nr, events, timeout);
            }

#ifndef __NR_io_pgetevents

#if BOOST_ARCH_X86_64
#define __NR_io_pgetevents 333
#elif BOOST_ARCH_X86_32
#define __NR_io_pgetevents 385
#endif

#endif

            int io_pgetevents(aio_context_t io_context, long min_nr, long nr, io_event *events,
                              const ::timespec *timeout, const sigset_t *sigmask, bool force_syscall) {
#ifdef __NR_io_pgetevents
                auto r = try_reap_events(io_context, min_nr, nr, events, timeout, force_syscall);
                if (r >= 0) {
                    return r;
                }
                aio_sigset as;
                as.sigmask = sigmask;
                as.sigsetsize = 8;    // Can't use sizeof(*sigmask) because user and kernel sigset_t are inconsistent
                return ::syscall(__NR_io_pgetevents, io_context, min_nr, nr, events, timeout, &as);
#else
                errno = ENOSYS;
                return -1;
#endif
            }

            void setup_aio_context(size_t nr, linux_abi::aio_context_t *io_context) {
                auto r = io_setup(nr, io_context);
                if (r < 0) {
                    char buf[1024];
                    char *msg = strerror_r(errno, buf, sizeof(buf));
                    throw std::runtime_error(fmt::format(
                        "Could not setup Async I/O: {}. The most common cause is not enough request capacity "
                        "in /proc/sys/fs/aio-max-nr. Try increasing that number or reducing the amount of "
                        "logical CPUs available for your application",
                        msg));
                }
            }

        }    // namespace detail

    }    // namespace actor
}    // namespace nil
