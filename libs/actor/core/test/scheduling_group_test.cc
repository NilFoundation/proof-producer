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
#include <nil/actor/core/print.hh>
#include <nil/actor/core/scheduling_specific.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/with_scheduling_group.hh>
#include <nil/actor/detail/later.hh>

using namespace std::chrono_literals;

using namespace nil::actor;

/**
 *  Test setting primitive and object as a value after all groups are created
 */
ACTOR_THREAD_TEST_CASE(sg_specific_values_define_after_sg_create) {
    using ivec = std::vector<int>;
    const int num_scheduling_groups = 4;
    std::vector<scheduling_group> sgs;
    for (int i = 0; i < num_scheduling_groups; i++) {
        sgs.push_back(create_scheduling_group(format("sg{}", i).c_str(), 100).get0());
    }

    const auto destroy_scheduling_groups = defer([&sgs]() {
        for (scheduling_group sg : sgs) {
            destroy_scheduling_group(sg).get();
        }
    });
    scheduling_group_key_config key1_conf = make_scheduling_group_key_config<int>();
    scheduling_group_key key1 = scheduling_group_key_create(key1_conf).get0();

    scheduling_group_key_config key2_conf = make_scheduling_group_key_config<ivec>();
    scheduling_group_key key2 = scheduling_group_key_create(key2_conf).get0();

    smp::invoke_on_all([key1, key2, &sgs]() {
        int factor = this_shard_id() + 1;
        for (int i = 0; i < num_scheduling_groups; i++) {
            sgs[i].get_specific<int>(key1) = (i + 1) * factor;
            sgs[i].get_specific<ivec>(key2).push_back((i + 1) * factor);
        }

        for (int i = 0; i < num_scheduling_groups; i++) {
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<int>(key1) = (i + 1) * factor, (i + 1) * factor);
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<ivec>(key2)[0], (i + 1) * factor);
        }
    }).get();

    smp::invoke_on_all([key1, key2]() {
        return reduce_scheduling_group_specific<int>(std::plus<int>(), int(0), key1)
            .then([](int sum) {
                int factor = this_shard_id() + 1;
                int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                BOOST_REQUIRE_EQUAL(expected_sum, sum);
            })
            .then([key2] {
                auto ivec_to_int = [](ivec &v) { return v.size() ? v[0] : 0; };

                return map_reduce_scheduling_group_specific<ivec>(ivec_to_int, std::plus<int>(), int(0), key2)
                    .then([](int sum) {
                        int factor = this_shard_id() + 1;
                        int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                        BOOST_REQUIRE_EQUAL(expected_sum, sum);
                    });
            });
    }).get();
}

/**
 *  Test setting primitive and object as a value before all groups are created
 */
ACTOR_THREAD_TEST_CASE(sg_specific_values_define_before_sg_create) {
    using ivec = std::vector<int>;
    const int num_scheduling_groups = 4;
    std::vector<scheduling_group> sgs;
    const auto destroy_scheduling_groups = defer([&sgs]() {
        for (scheduling_group sg : sgs) {
            destroy_scheduling_group(sg).get();
        }
    });
    scheduling_group_key_config key1_conf = make_scheduling_group_key_config<int>();
    scheduling_group_key key1 = scheduling_group_key_create(key1_conf).get0();

    scheduling_group_key_config key2_conf = make_scheduling_group_key_config<ivec>();
    scheduling_group_key key2 = scheduling_group_key_create(key2_conf).get0();

    for (int i = 0; i < num_scheduling_groups; i++) {
        sgs.push_back(create_scheduling_group(format("sg{}", i).c_str(), 100).get0());
    }

    smp::invoke_on_all([key1, key2, &sgs]() {
        int factor = this_shard_id() + 1;
        for (int i = 0; i < num_scheduling_groups; i++) {
            sgs[i].get_specific<int>(key1) = (i + 1) * factor;
            sgs[i].get_specific<ivec>(key2).push_back((i + 1) * factor);
        }

        for (int i = 0; i < num_scheduling_groups; i++) {
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<int>(key1) = (i + 1) * factor, (i + 1) * factor);
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<ivec>(key2)[0], (i + 1) * factor);
        }
    }).get();

    smp::invoke_on_all([key1, key2]() {
        return reduce_scheduling_group_specific<int>(std::plus<int>(), int(0), key1)
            .then([](int sum) {
                int factor = this_shard_id() + 1;
                int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                BOOST_REQUIRE_EQUAL(expected_sum, sum);
            })
            .then([key2] {
                auto ivec_to_int = [](ivec &v) { return v.size() ? v[0] : 0; };

                return map_reduce_scheduling_group_specific<ivec>(ivec_to_int, std::plus<int>(), int(0), key2)
                    .then([](int sum) {
                        int factor = this_shard_id() + 1;
                        int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                        BOOST_REQUIRE_EQUAL(expected_sum, sum);
                    });
            });
    }).get();
}

/**
 *  Test setting primitive and an object as a value before some groups are created
 *  and after some of the groups are created.
 */
ACTOR_THREAD_TEST_CASE(sg_specific_values_define_before_and_after_sg_create) {
    using ivec = std::vector<int>;
    const int num_scheduling_groups = 4;
    std::vector<scheduling_group> sgs;
    const auto destroy_scheduling_groups = defer([&sgs]() {
        for (scheduling_group sg : sgs) {
            destroy_scheduling_group(sg).get();
        }
    });

    for (int i = 0; i < num_scheduling_groups / 2; i++) {
        sgs.push_back(create_scheduling_group(format("sg{}", i).c_str(), 100).get0());
    }
    scheduling_group_key_config key1_conf = make_scheduling_group_key_config<int>();
    scheduling_group_key key1 = scheduling_group_key_create(key1_conf).get0();

    scheduling_group_key_config key2_conf = make_scheduling_group_key_config<ivec>();
    scheduling_group_key key2 = scheduling_group_key_create(key2_conf).get0();

    for (int i = num_scheduling_groups / 2; i < num_scheduling_groups; i++) {
        sgs.push_back(create_scheduling_group(format("sg{}", i).c_str(), 100).get0());
    }

    smp::invoke_on_all([key1, key2, &sgs]() {
        int factor = this_shard_id() + 1;
        for (int i = 0; i < num_scheduling_groups; i++) {
            sgs[i].get_specific<int>(key1) = (i + 1) * factor;
            sgs[i].get_specific<ivec>(key2).push_back((i + 1) * factor);
        }

        for (int i = 0; i < num_scheduling_groups; i++) {
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<int>(key1) = (i + 1) * factor, (i + 1) * factor);
            BOOST_REQUIRE_EQUAL(sgs[i].get_specific<ivec>(key2)[0], (i + 1) * factor);
        }
    }).get();

    smp::invoke_on_all([key1, key2]() {
        return reduce_scheduling_group_specific<int>(std::plus<int>(), int(0), key1)
            .then([](int sum) {
                int factor = this_shard_id() + 1;
                int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                BOOST_REQUIRE_EQUAL(expected_sum, sum);
            })
            .then([key2] {
                auto ivec_to_int = [](ivec &v) { return v.size() ? v[0] : 0; };

                return map_reduce_scheduling_group_specific<ivec>(ivec_to_int, std::plus<int>(), int(0), key2)
                    .then([](int sum) {
                        int factor = this_shard_id() + 1;
                        int expected_sum = ((1 + num_scheduling_groups) * num_scheduling_groups) * factor / 2;
                        BOOST_REQUIRE_EQUAL(expected_sum, sum);
                    });
            });
    }).get();
}

/*
 * Test that current scheduling group is inherited by nil::actor::async()
 */
ACTOR_THREAD_TEST_CASE(sg_scheduling_group_inheritance_in_actor_async_test) {
    scheduling_group sg = create_scheduling_group("sg0", 100).get0();
    thread_attributes attr = {};
    attr.sched_group = sg;
    nil::actor::async(attr, [attr] {
        BOOST_REQUIRE_EQUAL(detail::scheduling_group_index(current_scheduling_group()),
                            detail::scheduling_group_index(*(attr.sched_group)));

        nil::actor::async([attr] {
            BOOST_REQUIRE_EQUAL(detail::scheduling_group_index(current_scheduling_group()),
                                detail::scheduling_group_index(*(attr.sched_group)));

            smp::invoke_on_all([sched_group_idx = detail::scheduling_group_index(*(attr.sched_group))]() {
                BOOST_REQUIRE_EQUAL(detail::scheduling_group_index(current_scheduling_group()), sched_group_idx);
            }).get();
        }).get();
    }).get();
}

ACTOR_THREAD_TEST_CASE(later_preserves_sg) {
    scheduling_group sg = create_scheduling_group("sg", 100).get0();
    auto cleanup = defer([&] { destroy_scheduling_group(sg).get(); });
    with_scheduling_group(sg, [&] {
        return later().then([&] {
            BOOST_REQUIRE_EQUAL(detail::scheduling_group_index(current_scheduling_group()),
                                detail::scheduling_group_index(sg));
        });
    }).get();
}
