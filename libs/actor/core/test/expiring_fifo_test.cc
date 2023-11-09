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
#include <nil/actor/core/manual_clock.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/core/expiring_fifo.hh>
#include <nil/actor/detail/later.hh>
#include <boost/range/irange.hpp>

using namespace nil::actor;
using namespace std::chrono_literals;

ACTOR_TEST_CASE(test_no_expiry_operations) {
    expiring_fifo<int> fifo;

    BOOST_REQUIRE(fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 0u);
    BOOST_REQUIRE(!bool(fifo));

    fifo.push_back(1);

    BOOST_REQUIRE(!fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
    BOOST_REQUIRE(bool(fifo));
    BOOST_REQUIRE_EQUAL(fifo.front(), 1);

    fifo.push_back(2);
    fifo.push_back(3);

    BOOST_REQUIRE(!fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 3u);
    BOOST_REQUIRE(bool(fifo));
    BOOST_REQUIRE_EQUAL(fifo.front(), 1);

    fifo.pop_front();

    BOOST_REQUIRE(!fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 2u);
    BOOST_REQUIRE(bool(fifo));
    BOOST_REQUIRE_EQUAL(fifo.front(), 2);

    fifo.pop_front();

    BOOST_REQUIRE(!fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
    BOOST_REQUIRE(bool(fifo));
    BOOST_REQUIRE_EQUAL(fifo.front(), 3);

    fifo.pop_front();

    BOOST_REQUIRE(fifo.empty());
    BOOST_REQUIRE_EQUAL(fifo.size(), 0u);
    BOOST_REQUIRE(!bool(fifo));

    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_expiry_operations) {
    return nil::actor::async([] {
        std::vector<int> expired;
        struct my_expiry {
            std::vector<int> &e;
            void operator()(int &v) {
                e.push_back(v);
            }
        };

        expiring_fifo<int, my_expiry, manual_clock> fifo(my_expiry {expired});

        fifo.push_back(1, manual_clock::now() + 1s);

        BOOST_REQUIRE(!fifo.empty());
        BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
        BOOST_REQUIRE(bool(fifo));
        BOOST_REQUIRE_EQUAL(fifo.front(), 1);

        manual_clock::advance(1s);
        later().get();

        BOOST_REQUIRE(fifo.empty());
        BOOST_REQUIRE_EQUAL(fifo.size(), 0u);
        BOOST_REQUIRE(!bool(fifo));
        BOOST_REQUIRE_EQUAL(expired.size(), 1u);
        BOOST_REQUIRE_EQUAL(expired[0], 1);

        expired.clear();

        fifo.push_back(1);
        fifo.push_back(2, manual_clock::now() + 1s);
        fifo.push_back(3);

        manual_clock::advance(1s);
        later().get();

        BOOST_REQUIRE(!fifo.empty());
        BOOST_REQUIRE_EQUAL(fifo.size(), 2u);
        BOOST_REQUIRE(bool(fifo));
        BOOST_REQUIRE_EQUAL(expired.size(), 1u);
        BOOST_REQUIRE_EQUAL(expired[0], 2);
        BOOST_REQUIRE_EQUAL(fifo.front(), 1);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
        BOOST_REQUIRE_EQUAL(fifo.front(), 3);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 0u);

        expired.clear();

        fifo.push_back(1, manual_clock::now() + 1s);
        fifo.push_back(2, manual_clock::now() + 1s);
        fifo.push_back(3);
        fifo.push_back(4, manual_clock::now() + 2s);

        manual_clock::advance(1s);
        later().get();

        BOOST_REQUIRE(!fifo.empty());
        BOOST_REQUIRE_EQUAL(fifo.size(), 2u);
        BOOST_REQUIRE(bool(fifo));
        BOOST_REQUIRE_EQUAL(expired.size(), 2u);
        std::sort(expired.begin(), expired.end());
        BOOST_REQUIRE_EQUAL(expired[0], 1);
        BOOST_REQUIRE_EQUAL(expired[1], 2);
        BOOST_REQUIRE_EQUAL(fifo.front(), 3);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
        BOOST_REQUIRE_EQUAL(fifo.front(), 4);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 0u);

        expired.clear();

        fifo.push_back(1);
        fifo.push_back(2, manual_clock::now() + 1s);
        fifo.push_back(3, manual_clock::now() + 1s);
        fifo.push_back(4, manual_clock::now() + 1s);

        manual_clock::advance(1s);
        later().get();

        BOOST_REQUIRE(!fifo.empty());
        BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
        BOOST_REQUIRE(bool(fifo));
        BOOST_REQUIRE_EQUAL(expired.size(), 3u);
        std::sort(expired.begin(), expired.end());
        BOOST_REQUIRE_EQUAL(expired[0], 2);
        BOOST_REQUIRE_EQUAL(expired[1], 3);
        BOOST_REQUIRE_EQUAL(expired[2], 4);
        BOOST_REQUIRE_EQUAL(fifo.front(), 1);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 0u);

        expired.clear();

        fifo.push_back(1);
        fifo.push_back(2, manual_clock::now() + 1s);
        fifo.push_back(3, manual_clock::now() + 1s);
        fifo.push_back(4, manual_clock::now() + 1s);
        fifo.push_back(5);

        manual_clock::advance(1s);
        later().get();

        BOOST_REQUIRE_EQUAL(fifo.size(), 2u);
        BOOST_REQUIRE_EQUAL(fifo.front(), 1);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 1u);
        BOOST_REQUIRE_EQUAL(fifo.front(), 5);
        fifo.pop_front();
        BOOST_REQUIRE_EQUAL(fifo.size(), 0u);
    });
}
