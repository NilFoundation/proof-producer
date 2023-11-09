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
#include <nil/actor/detail/noncopyable_function.hh>

using namespace nil::actor;

BOOST_AUTO_TEST_CASE(basic_tests) {
    struct s {
        int f1(int x) const {
            return x + 1;
        }
        int f2(int x) {
            return x + 2;
        }
        static int f3(int x) {
            return x + 3;
        }
        int operator()(int x) const {
            return x + 4;
        }
    };
    s obj, obj2;
    auto fn1 = noncopyable_function<int(const s *, int)>(&s::f1);
    auto fn2 = noncopyable_function<int(s *, int)>(&s::f2);
    auto fn3 = noncopyable_function<int(int)>(&s::f3);
    auto fn4 = noncopyable_function<int(int)>(std::move(obj2));
    BOOST_REQUIRE_EQUAL(fn1(&obj, 1), 2);
    BOOST_REQUIRE_EQUAL(fn2(&obj, 1), 3);
    BOOST_REQUIRE_EQUAL(fn3(1), 4);
    BOOST_REQUIRE_EQUAL(fn4(1), 5);
}

template<size_t Extra>
struct payload {
    static unsigned live;
    char extra[Extra];
    std::unique_ptr<int> v;
    payload(int x) : v(std::make_unique<int>(x)) {
        ++live;
    }
    payload(payload &&x) noexcept : v(std::move(x.v)) {
        ++live;
    }
    void operator=(payload &&) = delete;
    ~payload() {
        --live;
    }
    int operator()() const {
        return *v;
    }
};

template<size_t Extra>
unsigned payload<Extra>::live;

template<size_t Extra>
void do_move_tests() {
    using payload = ::payload<Extra>;
    auto f1 = noncopyable_function<int()>(payload(3));
    BOOST_REQUIRE_EQUAL(payload::live, 1u);
    BOOST_REQUIRE_EQUAL(f1(), 3);
    auto f2 = noncopyable_function<int()>();
    BOOST_CHECK_THROW(f2(), std::bad_function_call);
    f2 = std::move(f1);
    BOOST_CHECK_THROW(f1(), std::bad_function_call);
    BOOST_REQUIRE_EQUAL(f2(), 3);
    BOOST_REQUIRE_EQUAL(payload::live, 1u);
    f2 = {};
    BOOST_REQUIRE_EQUAL(payload::live, 0u);
    BOOST_CHECK_THROW(f2(), std::bad_function_call);
}

BOOST_AUTO_TEST_CASE(small_move_tests) {
    do_move_tests<1>();
}

BOOST_AUTO_TEST_CASE(large_move_tests) {
    do_move_tests<1000>();
}
