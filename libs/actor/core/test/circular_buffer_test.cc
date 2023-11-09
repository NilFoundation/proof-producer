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

#define BOOST_TEST_MODULE core

#include <boost/test/included/unit_test.hpp>
#include <stdlib.h>
#include <chrono>
#include <deque>
#include <nil/actor/core/circular_buffer.hh>

using namespace nil::actor;

BOOST_AUTO_TEST_CASE(test_erasing) {
    circular_buffer<int> buf;

    buf.push_back(3);
    buf.erase(buf.begin(), buf.end());

    BOOST_REQUIRE(buf.size() == 0);
    BOOST_REQUIRE(buf.empty());

    buf.push_back(1);
    buf.push_back(2);
    buf.push_back(3);
    buf.push_back(4);
    buf.push_back(5);

    buf.erase(std::remove_if(buf.begin(), buf.end(), [](int v) { return (v & 1) == 0; }), buf.end());

    BOOST_REQUIRE(buf.size() == 3);
    BOOST_REQUIRE(!buf.empty());
    {
        auto i = buf.begin();
        BOOST_REQUIRE_EQUAL(*i++, 1);
        BOOST_REQUIRE_EQUAL(*i++, 3);
        BOOST_REQUIRE_EQUAL(*i++, 5);
        BOOST_REQUIRE(i == buf.end());
    }
}

BOOST_AUTO_TEST_CASE(test_erasing_at_beginning_or_end_does_not_invalidate_iterators) {
    // This guarantee comes from std::deque, which circular_buffer is supposed to mimic.

    circular_buffer<int> buf;

    buf.push_back(1);
    buf.push_back(2);
    buf.push_back(3);
    buf.push_back(4);
    buf.push_back(5);

    int *ptr_to_3 = &buf[2];
    auto iterator_to_3 = buf.begin() + 2;
    assert(*ptr_to_3 == 3);
    assert(*iterator_to_3 == 3);

    buf.erase(buf.begin(), buf.begin() + 2);

    BOOST_REQUIRE(*ptr_to_3 == 3);
    BOOST_REQUIRE(*iterator_to_3 == 3);

    buf.erase(buf.begin() + 1, buf.end());

    BOOST_REQUIRE(*ptr_to_3 == 3);
    BOOST_REQUIRE(*iterator_to_3 == 3);

    BOOST_REQUIRE(buf.size() == 1);
}

BOOST_AUTO_TEST_CASE(test_erasing_in_the_middle) {
    circular_buffer<int> buf;

    for (int i = 0; i < 10; ++i) {
        buf.push_back(i);
    }

    auto i = buf.erase(buf.begin() + 3, buf.begin() + 6);
    BOOST_REQUIRE_EQUAL(*i, 6);

    i = buf.begin();
    BOOST_REQUIRE_EQUAL(*i++, 0);
    BOOST_REQUIRE_EQUAL(*i++, 1);
    BOOST_REQUIRE_EQUAL(*i++, 2);
    BOOST_REQUIRE_EQUAL(*i++, 6);
    BOOST_REQUIRE_EQUAL(*i++, 7);
    BOOST_REQUIRE_EQUAL(*i++, 8);
    BOOST_REQUIRE_EQUAL(*i++, 9);
    BOOST_REQUIRE(i == buf.end());
}
