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
#include <nil/actor/core/map_reduce.hh>
#include <nil/actor/core/sleep.hh>
#include <nil/actor/core/shared_mutex.hh>

#include <boost/range/irange.hpp>

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_TEST_CASE(test_semaphore_consume) {
    semaphore sem(0);
    sem.consume(1);
    BOOST_REQUIRE_EQUAL(sem.current(), 0u);
    BOOST_REQUIRE_EQUAL(sem.waiters(), 0u);

    BOOST_REQUIRE_EQUAL(sem.try_wait(0), false);
    auto fut = sem.wait(1);
    BOOST_REQUIRE_EQUAL(fut.available(), false);
    BOOST_REQUIRE_EQUAL(sem.waiters(), 1u);
    sem.signal(2);
    BOOST_REQUIRE_EQUAL(sem.waiters(), 0u);
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_semaphore_1) {
    return do_with(std::make_pair(semaphore(0), 0), [](std::pair<semaphore, int> &x) {
        (void)x.first.wait().then([&x] { x.second++; });
        x.first.signal();
        return sleep(10ms).then([&x] { BOOST_REQUIRE_EQUAL(x.second, 1); });
    });
}

ACTOR_THREAD_TEST_CASE(test_semaphore_2) {
    auto sem = boost::make_optional<semaphore>(0);
    int x = 0;
    auto fut = sem->wait().then([&x] { x++; });
    sleep(10ms).get();
    BOOST_REQUIRE_EQUAL(x, 0);
    sem = boost::none;
    BOOST_CHECK_THROW(fut.get(), broken_promise);
}

ACTOR_TEST_CASE(test_semaphore_timeout_1) {
    return do_with(std::make_pair(semaphore(0), 0), [](std::pair<semaphore, int> &x) {
        (void)x.first.wait(100ms).then([&x] { x.second++; });
        (void)sleep(3ms).then([&x] { x.first.signal(); });
        return sleep(200ms).then([&x] { BOOST_REQUIRE_EQUAL(x.second, 1); });
    });
}

ACTOR_THREAD_TEST_CASE(test_semaphore_timeout_2) {
    auto sem = semaphore(0);
    int x = 0;
    auto fut1 = sem.wait(3ms).then([&x] { x++; });
    bool signaled = false;
    auto fut2 = sleep(100ms).then([&sem, &signaled] {
        signaled = true;
        sem.signal();
    });
    sleep(200ms).get();
    fut2.get();
    BOOST_REQUIRE_EQUAL(signaled, true);
    BOOST_CHECK_THROW(fut1.get(), semaphore_timed_out);
    BOOST_REQUIRE_EQUAL(x, 0);
}

ACTOR_THREAD_TEST_CASE(test_semaphore_mix_1) {
    auto sem = semaphore(0);
    int x = 0;
    auto fut1 = sem.wait(30ms).then([&x] { x++; });
    auto fut2 = sem.wait().then([&x] { x += 10; });
    auto fut3 = sleep(100ms).then([&sem] { sem.signal(); });
    sleep(200ms).get();
    fut3.get();
    fut2.get();
    BOOST_CHECK_THROW(fut1.get(), semaphore_timed_out);
    BOOST_REQUIRE_EQUAL(x, 10);
}

ACTOR_TEST_CASE(test_broken_semaphore) {
    auto sem = make_lw_shared<semaphore>(0);
    struct oops { };
    auto check_result = [sem](future<> f) {
        try {
            f.get();
            BOOST_FAIL("expecting exception");
        } catch (oops &x) {
            // ok
            return make_ready_future<>();
        } catch (...) {
            BOOST_FAIL("wrong exception seen");
        }
        BOOST_FAIL("unreachable");
        return make_ready_future<>();
    };
    auto ret = sem->wait().then_wrapped(check_result);
    sem->broken(oops());
    return sem->wait().then_wrapped(check_result).then([ret = std::move(ret)]() mutable { return std::move(ret); });
}

ACTOR_TEST_CASE(test_shared_mutex_exclusive) {
    return do_with(shared_mutex(), unsigned(0), [](shared_mutex &sm, unsigned &counter) {
        return parallel_for_each(boost::irange(0, 10), [&sm, &counter](int idx) {
            return with_lock(sm, [&counter] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                ++counter;
                return sleep(10ms).then([&counter] {
                    --counter;
                    BOOST_REQUIRE_EQUAL(counter, 0u);
                });
            });
        });
    });
}

ACTOR_TEST_CASE(test_shared_mutex_shared) {
    return do_with(shared_mutex(), unsigned(0), [](shared_mutex &sm, unsigned &counter) {
        auto running_in_parallel = [&sm, &counter](int instance) {
            return with_shared(sm, [&counter] {
                ++counter;
                return sleep(10ms).then([&counter] {
                    bool was_parallel = counter != 0;
                    --counter;
                    return was_parallel;
                });
            });
        };
        return map_reduce(boost::irange(0, 100), running_in_parallel, false, std::bit_or<bool>())
            .then([&counter](bool result) {
                BOOST_REQUIRE_EQUAL(result, true);
                BOOST_REQUIRE_EQUAL(counter, 0u);
            });
    });
}

ACTOR_TEST_CASE(test_shared_mutex_mixed) {
    return do_with(shared_mutex(), unsigned(0), [](shared_mutex &sm, unsigned &counter) {
        auto running_in_parallel = [&sm, &counter](int instance) {
            return with_shared(sm, [&counter] {
                ++counter;
                return sleep(10ms).then([&counter] {
                    bool was_parallel = counter != 0;
                    --counter;
                    return was_parallel;
                });
            });
        };
        auto running_alone = [&sm, &counter](int instance) {
            return with_lock(sm, [&counter] {
                BOOST_REQUIRE_EQUAL(counter, 0u);
                ++counter;
                return sleep(10ms).then([&counter] {
                    --counter;
                    BOOST_REQUIRE_EQUAL(counter, 0u);
                    return true;
                });
            });
        };
        auto run = [running_in_parallel, running_alone](int instance) {
            if (instance % 9 == 0) {
                return running_alone(instance);
            } else {
                return running_in_parallel(instance);
            }
        };
        return map_reduce(boost::irange(0, 100), run, false, std::bit_or<bool>()).then([&counter](bool result) {
            BOOST_REQUIRE_EQUAL(result, true);
            BOOST_REQUIRE_EQUAL(counter, 0u);
        });
    });
}

ACTOR_TEST_CASE(test_with_semaphore) {
    return do_with(semaphore(1), 0, [](semaphore &sem, int &counter) {
        return with_semaphore(sem, 1, [&counter] { ++counter; }).then([&counter, &sem]() {
            return with_semaphore(sem, 1,
                                  [&counter] {
                                      ++counter;
                                      throw 123;
                                  })
                .then_wrapped([&counter](auto &&fut) {
                    BOOST_REQUIRE_EQUAL(counter, 2);
                    BOOST_REQUIRE(fut.failed());
                    fut.ignore_ready_future();
                });
        });
    });
}

ACTOR_THREAD_TEST_CASE(test_semaphore_units_splitting) {
    auto sm = semaphore(2);
    auto units = get_units(sm, 2, 1min).get0();
    {
        BOOST_REQUIRE_EQUAL(units.count(), 2);
        BOOST_REQUIRE_EQUAL(sm.available_units(), 0);
        auto split = units.split(1);
        BOOST_REQUIRE_EQUAL(sm.available_units(), 0);
    }
    BOOST_REQUIRE_EQUAL(sm.available_units(), 1);
    units.~semaphore_units();
    units = get_units(sm, 2, 1min).get0();
    BOOST_REQUIRE_EQUAL(sm.available_units(), 0);
    BOOST_REQUIRE_THROW(units.split(10), std::invalid_argument);
    BOOST_REQUIRE_EQUAL(sm.available_units(), 0);
}

ACTOR_THREAD_TEST_CASE(test_semaphore_units_return) {
    auto sm = semaphore(3);
    auto units = get_units(sm, 3, 1min).get0();
    BOOST_REQUIRE_EQUAL(units.count(), 3);
    BOOST_REQUIRE_EQUAL(sm.available_units(), 0);
    BOOST_REQUIRE_EQUAL(units.return_units(1), 2);
    BOOST_REQUIRE_EQUAL(units.count(), 2);
    BOOST_REQUIRE_EQUAL(sm.available_units(), 1);
    units.~semaphore_units();
    BOOST_REQUIRE_EQUAL(sm.available_units(), 3);

    units = get_units(sm, 2, 1min).get0();
    BOOST_REQUIRE_EQUAL(sm.available_units(), 1);
    BOOST_REQUIRE_THROW(units.return_units(10), std::invalid_argument);
    BOOST_REQUIRE_EQUAL(sm.available_units(), 1);
    units.return_all();
    BOOST_REQUIRE_EQUAL(units.count(), 0);
    BOOST_REQUIRE_EQUAL(sm.available_units(), 3);
}

ACTOR_THREAD_TEST_CASE(test_named_semaphore_error) {
    auto sem = make_lw_shared<named_semaphore>(0, named_semaphore_exception_factory {"name_of_the_semaphore"});
    auto check_result = [sem](future<> f) {
        try {
            f.get();
            BOOST_FAIL("Expecting an exception");
        } catch (broken_named_semaphore &ex) {
            BOOST_REQUIRE_NE(std::string(ex.what()).find("name_of_the_semaphore"), std::string::npos);
        } catch (...) {
            BOOST_FAIL("Expected an instance of broken_named_semaphore with proper semaphore name");
        }
        return make_ready_future<>();
    };
    auto ret = sem->wait().then_wrapped(check_result);
    sem->broken();
    sem->wait().then_wrapped(check_result).then([ret = std::move(ret)]() mutable { return std::move(ret); }).get();
}

ACTOR_THREAD_TEST_CASE(test_named_semaphore_timeout) {
    auto sem = make_lw_shared<named_semaphore>(0, named_semaphore_exception_factory {"name_of_the_semaphore"});

    auto f = sem->wait(named_semaphore::clock::now() + 1ms, 1);
    try {
        f.get();
        BOOST_FAIL("Expecting an exception");
    } catch (named_semaphore_timed_out &ex) {
        BOOST_REQUIRE_NE(std::string(ex.what()).find("name_of_the_semaphore"), std::string::npos);
    } catch (...) {
        BOOST_FAIL("Expected an instance of named_semaphore_timed_out with proper semaphore name");
    }
}
