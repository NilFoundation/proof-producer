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

#include <nil/actor/core/thread.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/semaphore.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/core/sleep.hh>
#include <sys/mman.h>
#include <sys/signal.h>

#ifdef ACTOR_HAS_VALGRIND
#include <valgrind/valgrind.h>
#endif

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_TEST_CASE(test_thread_1) {
    return do_with(sstring(), [](sstring &x) {
        auto t1 = new thread([&x] { x = "abc"; });
        return t1->join().then([&x, t1] {
            BOOST_REQUIRE_EQUAL(x, "abc");
            delete t1;
        });
    });
}

ACTOR_TEST_CASE(test_thread_2) {
    struct tmp {
        std::vector<thread> threads;
        semaphore sem1 {0};
        semaphore sem2 {0};
        int counter = 0;
        void thread_fn() {
            sem1.wait(1).get();
            ++counter;
            sem2.signal(1);
        }
    };
    return do_with(tmp(), [](tmp &x) {
        auto n = 10;
        for (int i = 0; i < n; ++i) {
            x.threads.emplace_back(std::bind(&tmp::thread_fn, &x));
        }
        BOOST_REQUIRE_EQUAL(x.counter, 0);
        x.sem1.signal(n);
        return x.sem2.wait(n).then([&x, n] {
            BOOST_REQUIRE_EQUAL(x.counter, n);
            return parallel_for_each(x.threads.begin(), x.threads.end(), std::mem_fn(&thread::join));
        });
    });
}

ACTOR_TEST_CASE(test_thread_async) {
    sstring x = "x";
    sstring y = "y";
    auto concat = [](sstring x, sstring y) {
        sleep(10ms).get();
        return x + y;
    };
    return async(concat, x, y).then([](sstring xy) { BOOST_REQUIRE_EQUAL(xy, "xy"); });
}

ACTOR_TEST_CASE(test_thread_async_immed) {
    return async([] { return 3; }).then([](int three) { BOOST_REQUIRE_EQUAL(three, 3); });
}

ACTOR_TEST_CASE(test_thread_async_nested) {
    return async([] { return async([] { return 3; }).get0(); }).then([](int three) { BOOST_REQUIRE_EQUAL(three, 3); });
}

void compute(float &result, bool &done, uint64_t &ctr) {
    while (!done) {
        for (int n = 0; n < 10000; ++n) {
            result += 1 / (result + 1);
            ++ctr;
        }
        thread::yield();
    }
}

#if defined(ACTOR_ASAN_ENABLED) && defined(ACTOR_HAVE_ASAN_FIBER_SUPPORT)
volatile int force_write;
volatile void *shut_up_gcc;

[[gnu::noinline]] void throw_exception() {
    volatile char buf[1024];
    shut_up_gcc = &buf;
    for (int i = 0; i < 1024; i++) {
        buf[i] = force_write;
    }
    throw 1;
}

[[gnu::noinline]] void use_stack() {
    volatile char buf[2 * 1024];
    shut_up_gcc = &buf;
    for (int i = 0; i < 2 * 1024; i++) {
        buf[i] = force_write;
    }
}

ACTOR_TEST_CASE(test_asan_false_positive) {
    return async([] {
        try {
            throw_exception();
        } catch (...) {
            use_stack();
        }
    });
}
#endif

ACTOR_THREAD_TEST_CASE_EXPECTED_FAILURES(abc, 2) {
    BOOST_TEST(false);
    BOOST_TEST(false);
}

ACTOR_TEST_CASE(test_thread_custom_stack_size) {
    sstring x = "x";
    sstring y = "y";
    auto concat = [](sstring x, sstring y) {
        sleep(10ms).get();
        return x + y;
    };
    thread_attributes attr;
    attr.stack_size = 16384;
    return async(attr, concat, x, y).then([](sstring xy) { BOOST_REQUIRE_EQUAL(xy, "xy"); });
}

// The test case uses x86_64 specific signal handler info. The test
// fails with detect_stack_use_after_return=1. We could put it behind
// a command line option and fork/exec to run it after removing
// detect_stack_use_after_return=1 from the environment.
#if defined(ACTOR_THREAD_STACK_GUARDS) && defined(__x86_64__) && !defined(ACTOR_ASAN_ENABLED)
struct test_thread_custom_stack_size_failure : public nil::actor::testing::actor_test {
    const char *get_test_file() override {
        return __FILE__;
    }
    const char *get_name() override {
        return "test_thread_custom_stack_size_failure";
    }
    int get_expected_failures() override {
        return 0;
    }
    nil::actor::future<> run_test_case() override;
};

static test_thread_custom_stack_size_failure test_thread_custom_stack_size_failure_instance;
static thread_local volatile bool stack_guard_bypassed = false;

static int get_mprotect_flags(void *ctx) {
    int flags;
    ucontext_t *context = reinterpret_cast<ucontext_t *>(ctx);
    if (context->uc_mcontext.gregs[REG_ERR] & 0x2) {
        flags = PROT_READ | PROT_WRITE;
    } else {
        flags = PROT_READ;
    }
    return flags;
}

static void *pagealign(void *ptr, size_t page_size) {
    static const int pageshift = ffs(page_size) - 1;
    return reinterpret_cast<void *>(((reinterpret_cast<intptr_t>((ptr)) >> pageshift) << pageshift));
}

static thread_local struct sigaction default_old_sigsegv_handler;

static void bypass_stack_guard(int sig, siginfo_t *si, void *ctx) {
    assert(sig == SIGSEGV);
    int flags = get_mprotect_flags(ctx);
    stack_guard_bypassed = (flags & PROT_WRITE);
    if (!stack_guard_bypassed) {
        return;
    }
    size_t page_size = getpagesize();
    auto mp_result = mprotect(pagealign(si->si_addr, page_size), page_size, PROT_READ | PROT_WRITE);
    assert(mp_result == 0);
}

// This test will fail with a regular stack size, because we only probe
// around 10KiB of data, and the stack guard resides after 128'th KiB.
nil::actor::future<> test_thread_custom_stack_size_failure::run_test_case() {
    if (RUNNING_ON_VALGRIND) {
        return make_ready_future<>();
    }

    sstring x = "x";
    sstring y = "y";

    // Catch segmentation fault once:
    struct sigaction sa { };
    sa.sa_sigaction = &bypass_stack_guard;
    sa.sa_flags = SA_SIGINFO;
    auto ret = sigaction(SIGSEGV, &sa, &default_old_sigsegv_handler);
    if (ret) {
        throw std::system_error(ret, std::system_category());
    }

    auto concat = [](sstring x, sstring y) {
        sleep(10ms).get();
        // Probe the stack by writing to it in intervals of 1024,
        // until we hit a write fault. In order not to ruin anything,
        // the "write" uses data it just read from the address.
        volatile char *mem = reinterpret_cast<volatile char *>(&x);
        for (int i = 0; i < 20; ++i) {
            mem[i * -1024] = char(mem[i * -1024]);
            if (stack_guard_bypassed) {
                break;
            }
        }
        return x + y;
    };
    thread_attributes attr;
    attr.stack_size = 16384;
    return async(attr, concat, x, y)
        .then([](sstring xy) {
            BOOST_REQUIRE_EQUAL(xy, "xy");
            BOOST_REQUIRE(stack_guard_bypassed);
            auto ret = sigaction(SIGSEGV, &default_old_sigsegv_handler, nullptr);
            if (ret) {
                throw std::system_error(ret, std::system_category());
            }
        })
        .then([concat, x, y] {
            // The same function with a default stack will not trigger
            // a segfault, because its stack is much bigger than 10KiB
            return async(concat, x, y).then([](sstring xy) { BOOST_REQUIRE_EQUAL(xy, "xy"); });
        });
}
#endif    // ACTOR_THREAD_STACK_GUARDS && __x86_64__
