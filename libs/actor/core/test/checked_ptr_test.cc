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
#include <nil/actor/core/checked_ptr.hh>
#include <nil/actor/core/weak_ptr.hh>

using namespace nil::actor;

static_assert(std::is_nothrow_default_constructible<checked_ptr<int *>>::value);
static_assert(std::is_nothrow_move_constructible<checked_ptr<int *>>::value);
static_assert(std::is_nothrow_copy_constructible<checked_ptr<int *>>::value);

static_assert(std::is_nothrow_default_constructible<checked_ptr<weak_ptr<int>>>::value);
static_assert(std::is_nothrow_move_constructible<checked_ptr<weak_ptr<int>>>::value);

template<typename T>
class may_throw_on_null_ptr : public nil::actor::weak_ptr<T> {
public:
    may_throw_on_null_ptr(std::nullptr_t) {
    }
};

static_assert(!std::is_nothrow_default_constructible_v<may_throw_on_null_ptr<int>>);
static_assert(!std::is_nothrow_default_constructible_v<checked_ptr<may_throw_on_null_ptr<int>>>);
static_assert(std::is_nothrow_move_constructible_v<checked_ptr<may_throw_on_null_ptr<int>>>);
static_assert(!std::is_nothrow_copy_constructible_v<checked_ptr<may_throw_on_null_ptr<int>>>);

struct my_st : public weakly_referencable<my_st> {
    my_st(int a_) : a(a_) {
    }
    int a;
};

void const_ref_check_naked(const nil::actor::checked_ptr<my_st *> &cp) {
    BOOST_REQUIRE(bool(cp));
    BOOST_REQUIRE((*cp).a == 3);
    BOOST_REQUIRE(cp->a == 3);
    BOOST_REQUIRE(cp.get()->a == 3);
}

void const_ref_check_smart(const nil::actor::checked_ptr<::weak_ptr<my_st>> &cp) {
    BOOST_REQUIRE(bool(cp));
    BOOST_REQUIRE((*cp).a == 3);
    BOOST_REQUIRE(cp->a == 3);
    BOOST_REQUIRE(cp.get()->a == 3);
}

BOOST_AUTO_TEST_CASE(test_checked_ptr_is_empty_when_default_initialized) {
    nil::actor::checked_ptr<int *> cp;
    BOOST_REQUIRE(!bool(cp));
}

BOOST_AUTO_TEST_CASE(test_checked_ptr_is_empty_when_nullptr_initialized_nakes_ptr) {
    nil::actor::checked_ptr<int *> cp = nullptr;
    BOOST_REQUIRE(!bool(cp));
}

BOOST_AUTO_TEST_CASE(test_checked_ptr_is_empty_when_nullptr_initialized_smart_ptr) {
    nil::actor::checked_ptr<::weak_ptr<my_st>> cp = nullptr;
    BOOST_REQUIRE(!bool(cp));
}

BOOST_AUTO_TEST_CASE(test_checked_ptr_is_initialized_after_assignment_naked_ptr) {
    nil::actor::checked_ptr<my_st *> cp = nullptr;
    BOOST_REQUIRE(!bool(cp));
    my_st i(3);
    my_st k(3);
    cp = &i;
    nil::actor::checked_ptr<my_st *> cp1(&i);
    nil::actor::checked_ptr<my_st *> cp2(&k);
    BOOST_REQUIRE(bool(cp));
    BOOST_REQUIRE(cp == cp1);
    BOOST_REQUIRE(cp != cp2);
    BOOST_REQUIRE((*cp).a == 3);
    BOOST_REQUIRE(cp->a == 3);
    BOOST_REQUIRE(cp.get()->a == 3);

    const_ref_check_naked(cp);

    cp = nullptr;
    BOOST_REQUIRE(!bool(cp));
}

BOOST_AUTO_TEST_CASE(test_checked_ptr_is_initialized_after_assignment_smart_ptr) {
    nil::actor::checked_ptr<::weak_ptr<my_st>> cp = nullptr;
    BOOST_REQUIRE(!bool(cp));
    std::unique_ptr<my_st> i = std::make_unique<my_st>(3);
    cp = i->weak_from_this();
    nil::actor::checked_ptr<::weak_ptr<my_st>> cp1(i->weak_from_this());
    nil::actor::checked_ptr<::weak_ptr<my_st>> cp2;
    BOOST_REQUIRE(bool(cp));
    BOOST_REQUIRE(cp == cp1);
    BOOST_REQUIRE(cp != cp2);
    BOOST_REQUIRE((*cp).a == 3);
    BOOST_REQUIRE(cp->a == 3);
    BOOST_REQUIRE(cp.get()->a == 3);

    const_ref_check_smart(cp);

    i = nullptr;
    BOOST_REQUIRE(!bool(cp));
    BOOST_REQUIRE(!bool(cp1));
    BOOST_REQUIRE(!bool(cp2));
}
