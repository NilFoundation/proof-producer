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

#include <algorithm>
#include <vector>
#include <chrono>

#include <nil/actor/core/thread.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/testing/test_runner.hh>
#include <nil/actor/core/execution_stage.hh>
#include <nil/actor/core/sleep.hh>

using namespace std::chrono_literals;

using namespace nil::actor;

ACTOR_TEST_CASE(test_create_stage_from_lvalue_function_object) {
    return nil::actor::async([] {
        auto dont_move = [obj = make_shared<int>(53)] { return *obj; };
        auto stage = nil::actor::make_execution_stage("test", dont_move);
        BOOST_REQUIRE_EQUAL(stage().get0(), 53);
        BOOST_REQUIRE_EQUAL(dont_move(), 53);
    });
}

ACTOR_TEST_CASE(test_create_stage_from_rvalue_function_object) {
    return nil::actor::async([] {
        auto dont_copy = [obj = std::make_unique<int>(42)] { return *obj; };
        auto stage = nil::actor::make_execution_stage("test", std::move(dont_copy));
        BOOST_REQUIRE_EQUAL(stage().get0(), 42);
    });
}

int func() {
    return 64;
}

ACTOR_TEST_CASE(test_create_stage_from_function) {
    return nil::actor::async([] {
        auto stage = nil::actor::make_execution_stage("test", func);
        BOOST_REQUIRE_EQUAL(stage().get0(), 64);
    });
}

template<typename Function, typename Verify>
void test_simple_execution_stage(Function &&func, Verify &&verify) {
    auto stage = nil::actor::make_execution_stage("test", std::forward<Function>(func));

    std::vector<int> vs;
    std::default_random_engine &gen = testing::local_random_engine;
    std::uniform_int_distribution<> dist(0, 100'000);
    std::generate_n(std::back_inserter(vs), 1'000, [&] { return dist(gen); });

    std::vector<future<int>> fs;
    for (auto v : vs) {
        fs.emplace_back(stage(v));
    }

    for (auto i = 0u; i < fs.size(); i++) {
        verify(vs[i], std::move(fs[i]));
    }
}

ACTOR_TEST_CASE(test_simple_stage_returning_int) {
    return nil::actor::async([] {
        test_simple_execution_stage(
            [](int x) {
                if (x % 2) {
                    return x * 2;
                } else {
                    throw x;
                }
            },
            [](int original, future<int> result) {
                if (original % 2) {
                    BOOST_REQUIRE_EQUAL(original * 2, result.get0());
                } else {
                    BOOST_REQUIRE_EXCEPTION(result.get0(), int, [&](int v) { return original == v; });
                }
            });
    });
}

ACTOR_TEST_CASE(test_simple_stage_returning_future_int) {
    return nil::actor::async([] {
        test_simple_execution_stage(
            [](int x) {
                if (x % 2) {
                    return make_ready_future<int>(x * 2);
                } else {
                    return make_exception_future<int>(x);
                }
            },
            [](int original, future<int> result) {
                if (original % 2) {
                    BOOST_REQUIRE_EQUAL(original * 2, result.get0());
                } else {
                    BOOST_REQUIRE_EXCEPTION(result.get0(), int, [&](int v) { return original == v; });
                }
            });
    });
}

template<typename T>
void test_execution_stage_avoids_copy() {
    auto stage = nil::actor::make_execution_stage("test", [](T obj) { return std::move(obj); });

    auto f = stage(T());
    T obj = f.get0();
    (void)obj;
}

ACTOR_TEST_CASE(test_stage_moves_when_cannot_copy) {
    return nil::actor::async([] {
        struct noncopyable_but_movable {
            noncopyable_but_movable() = default;
            noncopyable_but_movable(const noncopyable_but_movable &) = delete;
            noncopyable_but_movable(noncopyable_but_movable &&) = default;
        };

        test_execution_stage_avoids_copy<noncopyable_but_movable>();
    });
}

ACTOR_TEST_CASE(test_stage_prefers_move_to_copy) {
    return nil::actor::async([] {
        struct copyable_and_movable {
            copyable_and_movable() = default;
            copyable_and_movable(const copyable_and_movable &) {
                BOOST_FAIL("should not copy");
            }
            copyable_and_movable(copyable_and_movable &&) = default;
        };

        test_execution_stage_avoids_copy<copyable_and_movable>();
    });
}

ACTOR_TEST_CASE(test_rref_decays_to_value) {
    return nil::actor::async([] {
        auto stage = nil::actor::make_execution_stage("test", [](std::vector<int> &&vec) { return vec.size(); });

        std::vector<int> tmp;
        std::vector<future<size_t>> fs;
        for (auto i = 0; i < 100; i++) {
            tmp.resize(i);
            fs.emplace_back(stage(std::move(tmp)));
            tmp = std::vector<int>();
        }

        for (size_t i = 0; i < 100; i++) {
            BOOST_REQUIRE_EQUAL(fs[i].get0(), i);
        }
    });
}

ACTOR_TEST_CASE(test_lref_does_not_decay) {
    return nil::actor::async([] {
        auto stage = nil::actor::make_execution_stage("test", [](int &v) { v++; });

        int value = 0;
        std::vector<future<>> fs;
        for (auto i = 0; i < 100; i++) {
            // fs.emplace_back(stage(value)); // should fail to compile
            fs.emplace_back(stage(nil::actor::ref(value)));
        }

        for (auto &&f : fs) {
            f.get();
        }
        BOOST_REQUIRE_EQUAL(value, 100);
    });
}

ACTOR_TEST_CASE(test_explicit_reference_wrapper_is_not_unwrapped) {
    return nil::actor::async([] {
        auto stage = nil::actor::make_execution_stage("test", [](nil::actor::reference_wrapper<int> v) { v.get()++; });

        int value = 0;
        std::vector<future<>> fs;
        for (auto i = 0; i < 100; i++) {
            // fs.emplace_back(stage(value)); // should fail to compile
            fs.emplace_back(stage(nil::actor::ref(value)));
        }

        for (auto &&f : fs) {
            f.get();
        }
        BOOST_REQUIRE_EQUAL(value, 100);
    });
}

ACTOR_TEST_CASE(test_function_is_class_member) {
    return nil::actor::async([] {
        struct foo {
            int value = -1;
            int member(int x) {
                return std::exchange(value, x);
            }
        };

        auto stage = nil::actor::make_execution_stage("test", &foo::member);

        foo object;
        std::vector<future<int>> fs;
        for (auto i = 0; i < 100; i++) {
            fs.emplace_back(stage(&object, i));
        }

        for (auto i = 0; i < 100; i++) {
            BOOST_REQUIRE_EQUAL(fs[i].get0(), i - 1);
        }
        BOOST_REQUIRE_EQUAL(object.value, 99);
    });
}

ACTOR_TEST_CASE(test_function_is_const_class_member) {
    return nil::actor::async([] {
        struct foo {
            int value = 999;
            int member() const {
                return value;
            }
        };
        auto stage = nil::actor::make_execution_stage("test", &foo::member);

        const foo object;
        BOOST_REQUIRE_EQUAL(stage(&object).get0(), 999);
    });
}

ACTOR_TEST_CASE(test_stage_stats) {
    return nil::actor::async([] {
        auto stage = nil::actor::make_execution_stage("test", [] {});

        BOOST_REQUIRE_EQUAL(stage.get_stats().function_calls_enqueued, 0u);
        BOOST_REQUIRE_EQUAL(stage.get_stats().function_calls_executed, 0u);

        auto fs = std::vector<future<>>();
        static constexpr auto call_count = 53u;
        for (auto i = 0u; i < call_count; i++) {
            fs.emplace_back(stage());
        }

        BOOST_REQUIRE_EQUAL(stage.get_stats().function_calls_enqueued, call_count);

        for (auto i = 0u; i < call_count; i++) {
            fs[i].get();
            BOOST_REQUIRE_GE(stage.get_stats().tasks_scheduled, 1u);
            BOOST_REQUIRE_GE(stage.get_stats().function_calls_executed, i);
        }
        BOOST_REQUIRE_EQUAL(stage.get_stats().function_calls_executed, call_count);
    });
}

ACTOR_TEST_CASE(test_unique_stage_names_are_enforced) {
    return nil::actor::async([] {
        {
            auto stage = nil::actor::make_execution_stage("test", [] {});
            BOOST_REQUIRE_THROW(nil::actor::make_execution_stage("test", [] {}), std::invalid_argument);
            stage().get();
        }

        auto stage = nil::actor::make_execution_stage("test", [] {});
        stage().get();
    });
}

ACTOR_THREAD_TEST_CASE(test_inheriting_concrete_execution_stage) {
    auto sg1 = nil::actor::create_scheduling_group("sg1", 300).get0();
    auto ksg1 = nil::actor::defer([&] { nil::actor::destroy_scheduling_group(sg1).get(); });
    auto sg2 = nil::actor::create_scheduling_group("sg2", 100).get0();
    auto ksg2 = nil::actor::defer([&] { nil::actor::destroy_scheduling_group(sg2).get(); });
    auto check_sg = [](nil::actor::scheduling_group sg) {
        BOOST_REQUIRE(nil::actor::current_scheduling_group() == sg);
    };
    auto es = nil::actor::inheriting_concrete_execution_stage<void, nil::actor::scheduling_group>("stage", check_sg);
    auto make_attr = [](scheduling_group sg) {
        nil::actor::thread_attributes a;
        a.sched_group = sg;
        return a;
    };
    bool done = false;
    auto make_test_thread = [&](scheduling_group sg) {
        return nil::actor::thread(make_attr(sg), [&, sg] {
            while (!done) {
                es(sg).get();    // will check if executed with same sg
            };
        });
    };
    auto th1 = make_test_thread(sg1);
    auto th2 = make_test_thread(sg2);
    nil::actor::sleep(10ms).get();
    done = true;
    th1.join().get();
    th2.join().get();
}

struct a_struct { };

ACTOR_THREAD_TEST_CASE(test_inheriting_concrete_execution_stage_reference_parameters) {
    // mostly a compile test, but take the opportunity to test that passing
    // by reference preserves the address
    auto check_ref = [](a_struct &ref, a_struct *ptr) { BOOST_REQUIRE_EQUAL(&ref, ptr); };
    auto es = nil::actor::inheriting_concrete_execution_stage<void, a_struct &, a_struct *>("stage", check_ref);
    a_struct obj;
    es(nil::actor::ref(obj), &obj).get();
}
