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

#include <nil/actor/testing/test_case.hh>

#include <nil/actor/core/distributed.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/sleep.hh>
#include <iostream>

using namespace nil::actor;

ACTOR_TEST_CASE(make_foreign_ptr_from_lw_shared_ptr) {
    auto p = make_foreign(make_lw_shared<sstring>("foo"));
    BOOST_REQUIRE(p->size() == 3);
    return make_ready_future<>();
}

ACTOR_TEST_CASE(make_foreign_ptr_from_shared_ptr) {
    auto p = make_foreign(make_shared<sstring>("foo"));
    BOOST_REQUIRE(p->size() == 3);
    return make_ready_future<>();
}

ACTOR_TEST_CASE(foreign_ptr_copy_test) {
    return nil::actor::async([] {
        auto ptr = make_foreign(make_shared<sstring>("foo"));
        BOOST_REQUIRE(ptr->size() == 3);
        auto ptr2 = ptr.copy().get0();
        BOOST_REQUIRE(ptr2->size() == 3);
    });
}

ACTOR_TEST_CASE(foreign_ptr_get_test) {
    auto p = make_foreign(std::make_unique<sstring>("foo"));
    BOOST_REQUIRE_EQUAL(p.get(), &*p);
    return make_ready_future<>();
};

ACTOR_TEST_CASE(foreign_ptr_release_test) {
    auto p = make_foreign(std::make_unique<sstring>("foo"));
    auto raw_ptr = p.get();
    BOOST_REQUIRE(bool(p));
    BOOST_REQUIRE(p->size() == 3);
    auto released_p = p.release();
    BOOST_REQUIRE(!bool(p));
    BOOST_REQUIRE(released_p->size() == 3);
    BOOST_REQUIRE_EQUAL(raw_ptr, released_p.get());
    return make_ready_future<>();
}

ACTOR_TEST_CASE(foreign_ptr_reset_test) {
    auto fp = make_foreign(std::make_unique<sstring>("foo"));
    BOOST_REQUIRE(bool(fp));
    BOOST_REQUIRE(fp->size() == 3);

    fp.reset(std::make_unique<sstring>("foobar"));
    BOOST_REQUIRE(bool(fp));
    BOOST_REQUIRE(fp->size() == 6);

    fp.reset();
    BOOST_REQUIRE(!bool(fp));
    return make_ready_future<>();
}

class dummy {
    unsigned _cpu;

public:
    dummy() : _cpu(this_shard_id()) {
    }
    ~dummy() {
        BOOST_REQUIRE_EQUAL(_cpu, this_shard_id());
    }
};

ACTOR_TEST_CASE(foreign_ptr_cpu_test) {
    if (smp::count == 1) {
        std::cerr << "Skipping multi-cpu foreign_ptr tests. Run with --smp=2 to test multi-cpu delete and reset.";
        return make_ready_future<>();
    }

    using namespace std::chrono_literals;

    return nil::actor::async([] {
               auto p = smp::submit_to(1, [] { return make_foreign(std::make_unique<dummy>()); }).get0();

               p.reset(std::make_unique<dummy>());
           })
        .then([] {
            // Let ~foreign_ptr() take its course. RIP dummy.
            return nil::actor::sleep(100ms);
        });
}

ACTOR_TEST_CASE(foreign_ptr_move_assignment_test) {
    if (smp::count == 1) {
        std::cerr << "Skipping multi-cpu foreign_ptr tests. Run with --smp=2 to test multi-cpu delete and reset.";
        return make_ready_future<>();
    }

    using namespace std::chrono_literals;

    return nil::actor::async([] {
               auto p = smp::submit_to(1, [] { return make_foreign(std::make_unique<dummy>()); }).get0();

               p = foreign_ptr<std::unique_ptr<dummy>>();
           })
        .then([] {
            // Let ~foreign_ptr() take its course. RIP dummy.
            return nil::actor::sleep(100ms);
        });
}
