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
#include <nil/actor/core/weak_ptr.hh>

using namespace nil::actor;

class myclass : public weakly_referencable<myclass> { };

static_assert(std::is_nothrow_default_constructible<myclass>::value);

static_assert(std::is_nothrow_default_constructible<weak_ptr<myclass>>::value);
static_assert(std::is_nothrow_move_constructible<weak_ptr<myclass>>::value);

BOOST_AUTO_TEST_CASE(test_weak_ptr_is_empty_when_default_initialized) {
    weak_ptr<myclass> wp;
    BOOST_REQUIRE(!bool(wp));
}

BOOST_AUTO_TEST_CASE(test_weak_ptr_is_reset) {
    auto owning_ptr = std::make_unique<myclass>();
    weak_ptr<myclass> wp = owning_ptr->weak_from_this();
    BOOST_REQUIRE(bool(wp));
    BOOST_REQUIRE(&*wp == &*owning_ptr);
    owning_ptr = {};
    BOOST_REQUIRE(!bool(wp));
}

BOOST_AUTO_TEST_CASE(test_weak_ptr_can_be_moved) {
    auto owning_ptr = std::make_unique<myclass>();
    weak_ptr<myclass> wp1 = owning_ptr->weak_from_this();
    weak_ptr<myclass> wp2 = owning_ptr->weak_from_this();
    weak_ptr<myclass> wp3 = owning_ptr->weak_from_this();

    auto wp3_moved = std::move(wp3);
    auto wp1_moved = std::move(wp1);
    auto wp2_moved = std::move(wp2);
    BOOST_REQUIRE(!bool(wp1));
    BOOST_REQUIRE(!bool(wp2));
    BOOST_REQUIRE(!bool(wp3));
    BOOST_REQUIRE(bool(wp1_moved));
    BOOST_REQUIRE(bool(wp2_moved));
    BOOST_REQUIRE(bool(wp3_moved));

    owning_ptr = {};

    BOOST_REQUIRE(!bool(wp1_moved));
    BOOST_REQUIRE(!bool(wp2_moved));
    BOOST_REQUIRE(!bool(wp3_moved));
}

BOOST_AUTO_TEST_CASE(test_multipe_weak_ptrs) {
    auto owning_ptr = std::make_unique<myclass>();

    weak_ptr<myclass> wp1 = owning_ptr->weak_from_this();
    BOOST_REQUIRE(bool(wp1));
    BOOST_REQUIRE(&*wp1 == &*owning_ptr);

    weak_ptr<myclass> wp2 = owning_ptr->weak_from_this();
    BOOST_REQUIRE(bool(wp2));
    BOOST_REQUIRE(&*wp2 == &*owning_ptr);

    owning_ptr = {};

    BOOST_REQUIRE(!bool(wp1));
    BOOST_REQUIRE(!bool(wp2));
}

BOOST_AUTO_TEST_CASE(test_multipe_weak_ptrs_going_away_first) {
    auto owning_ptr = std::make_unique<myclass>();

    weak_ptr<myclass> wp1 = owning_ptr->weak_from_this();
    weak_ptr<myclass> wp2 = owning_ptr->weak_from_this();
    weak_ptr<myclass> wp3 = owning_ptr->weak_from_this();

    BOOST_REQUIRE(bool(wp1));
    BOOST_REQUIRE(bool(wp2));
    BOOST_REQUIRE(bool(wp3));

    wp2 = {};

    owning_ptr = std::make_unique<myclass>();

    BOOST_REQUIRE(!bool(wp1));
    BOOST_REQUIRE(!bool(wp2));
    BOOST_REQUIRE(!bool(wp3));

    wp1 = owning_ptr->weak_from_this();
    wp2 = owning_ptr->weak_from_this();
    wp3 = owning_ptr->weak_from_this();

    BOOST_REQUIRE(bool(wp1));
    BOOST_REQUIRE(bool(wp2));
    BOOST_REQUIRE(bool(wp3));

    wp3 = {};
    owning_ptr = std::make_unique<myclass>();

    BOOST_REQUIRE(!bool(wp1));
    BOOST_REQUIRE(!bool(wp2));
    BOOST_REQUIRE(!bool(wp3));

    wp1 = owning_ptr->weak_from_this();
    wp2 = owning_ptr->weak_from_this();
    wp3 = owning_ptr->weak_from_this();

    wp1 = {};
    wp3 = {};
    owning_ptr = std::make_unique<myclass>();

    BOOST_REQUIRE(!bool(wp1));
    BOOST_REQUIRE(!bool(wp2));
    BOOST_REQUIRE(!bool(wp3));
}
