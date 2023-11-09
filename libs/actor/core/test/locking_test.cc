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

#include <chrono>

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/core/sleep.hh>
#include <nil/actor/core/rwlock.hh>
#include <nil/actor/core/shared_mutex.hh>
#include <nil/actor/detail/alloc_failure_injector.hh>

#include <boost/range/irange.hpp>

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_THREAD_TEST_CASE(test_rwlock) {
    rwlock l;

    l.for_write().lock().get();
    BOOST_REQUIRE(!l.try_write_lock());
    BOOST_REQUIRE(!l.try_read_lock());
    l.for_write().unlock();

    l.for_read().lock().get();
    BOOST_REQUIRE(!l.try_write_lock());
    BOOST_REQUIRE(l.try_read_lock());
    l.for_read().lock().get();
    l.for_read().unlock();
    l.for_read().unlock();
    l.for_read().unlock();

    BOOST_REQUIRE(l.try_write_lock());
    l.for_write().unlock();
}

ACTOR_TEST_CASE(test_with_lock_mutable) {
    return do_with(rwlock(),
                   [](rwlock &l) { return with_lock(l.for_read(), [p = std::make_unique<int>(42)]() mutable {}); });
}

ACTOR_TEST_CASE(test_rwlock_exclusive) {
    return do_with(rwlock(), unsigned(0), [](rwlock &l, unsigned &counter) {
        return parallel_for_each(boost::irange(0, 10), [&l, &counter](int idx) {
            return with_lock(l.for_write(), [&counter] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                ++counter;
                return sleep(1ms).then([&counter] {
                    --counter;
                    BOOST_REQUIRE_EQUAL(counter, 0u);
                });
            });
        });
    });
}

ACTOR_TEST_CASE(test_rwlock_shared) {
    return do_with(rwlock(), unsigned(0), unsigned(0), [](rwlock &l, unsigned &counter, unsigned &max) {
        return parallel_for_each(boost::irange(0, 10),
                                 [&l, &counter, &max](int idx) {
                                     return with_lock(l.for_read(), [&counter, &max] {
                                         ++counter;
                                         max = std::max(max, counter);
                                         return sleep(1ms).then([&counter] { --counter; });
                                     });
                                 })
            .finally([&counter, &max] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                BOOST_REQUIRE_NE(max, 0u);
            });
    });
}

ACTOR_THREAD_TEST_CASE(test_rwlock_failed_func) {
    rwlock l;

    // verify that the rwlock is unlocked when func fails
    future<> fut = with_lock(l.for_read(), [] { throw std::runtime_error("injected"); });
    BOOST_REQUIRE_THROW(fut.get(), std::runtime_error);

    fut = with_lock(l.for_write(), [] { throw std::runtime_error("injected"); });
    BOOST_REQUIRE_THROW(fut.get(), std::runtime_error);

    BOOST_REQUIRE(l.try_write_lock());
    l.for_write().unlock();
}

ACTOR_THREAD_TEST_CASE(test_failed_with_lock) {
    struct test_lock {
        future<> lock() noexcept {
            return make_exception_future<>(std::runtime_error("injected"));
        }
        void unlock() noexcept {
            BOOST_REQUIRE(false);
        }
    };

    test_lock l;

    // if l.lock() fails neither the function nor l.unlock()
    // should be called.
    BOOST_REQUIRE_THROW(with_lock(l, [] { BOOST_REQUIRE(false); }).get(), std::runtime_error);
}

ACTOR_THREAD_TEST_CASE(test_shared_mutex) {
    shared_mutex sm;

    sm.lock().get();
    BOOST_REQUIRE(!sm.try_lock());
    BOOST_REQUIRE(!sm.try_lock_shared());
    sm.unlock();

    sm.lock_shared().get();
    BOOST_REQUIRE(!sm.try_lock());
    BOOST_REQUIRE(sm.try_lock_shared());
    sm.lock_shared().get();
    sm.unlock_shared();
    sm.unlock_shared();
    sm.unlock_shared();

    BOOST_REQUIRE(sm.try_lock());
    sm.unlock();
}

ACTOR_TEST_CASE(test_shared_mutex_exclusive) {
    return do_with(shared_mutex(), unsigned(0), [](shared_mutex &sm, unsigned &counter) {
        return parallel_for_each(boost::irange(0, 10), [&sm, &counter](int idx) {
            return with_lock(sm, [&counter] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                ++counter;
                return sleep(1ms).then([&counter] {
                    --counter;
                    BOOST_REQUIRE_EQUAL(counter, 0u);
                });
            });
        });
    });
}

ACTOR_TEST_CASE(test_shared_mutex_shared) {
    return do_with(shared_mutex(), unsigned(0), unsigned(0), [](shared_mutex &sm, unsigned &counter, unsigned &max) {
        return parallel_for_each(boost::irange(0, 10),
                                 [&sm, &counter, &max](int idx) {
                                     return with_shared(sm, [&counter, &max] {
                                         ++counter;
                                         max = std::max(max, counter);
                                         return sleep(1ms).then([&counter] { --counter; });
                                     });
                                 })
            .finally([&counter, &max] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                BOOST_REQUIRE_NE(max, 0u);
            });
    });
}

ACTOR_THREAD_TEST_CASE(test_shared_mutex_failed_func) {
    shared_mutex sm;

    // verify that the shared_mutex is unlocked when func fails
    future<> fut = with_shared(sm, [] { throw std::runtime_error("injected"); });
    BOOST_REQUIRE_THROW(fut.get(), std::runtime_error);

    fut = with_lock(sm, [] { throw std::runtime_error("injected"); });
    BOOST_REQUIRE_THROW(fut.get(), std::runtime_error);

    BOOST_REQUIRE(sm.try_lock());
    sm.unlock();
}

ACTOR_THREAD_TEST_CASE(test_shared_mutex_failed_lock) {
#ifdef ACTOR_ENABLE_ALLOC_FAILURE_INJECTION
    shared_mutex sm;

    // if l.lock() fails neither the function nor l.unlock()
    // should be called.
    sm.lock().get();
    nil::actor::memory::local_failure_injector().fail_after(0);
    BOOST_REQUIRE_THROW(with_shared(sm, [] { BOOST_REQUIRE(false); }).get(), std::bad_alloc);

    nil::actor::memory::local_failure_injector().fail_after(0);
    BOOST_REQUIRE_THROW(with_lock(sm, [] { BOOST_REQUIRE(false); }).get(), std::bad_alloc);
    sm.unlock();

    nil::actor::memory::local_failure_injector().cancel();
#endif    // ACTOR_ENABLE_ALLOC_FAILURE_INJECTION
}
