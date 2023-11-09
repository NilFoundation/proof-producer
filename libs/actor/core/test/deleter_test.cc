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
#include <nil/actor/core/deleter.hh>

using namespace nil::actor;

struct TestObject {
    TestObject() : has_ref(true) {
    }
    TestObject(TestObject &&other) {
        has_ref = true;
        other.has_ref = false;
    }
    ~TestObject() {
        if (has_ref) {
            ++deletions_called;
        }
    }
    static int deletions_called;
    int has_ref;
};
int TestObject::deletions_called = 0;

BOOST_AUTO_TEST_CASE(test_deleter_append_does_not_free_shared_object) {
    {
        deleter tested;
        {
            auto obj1 = TestObject();
            deleter del1 = make_object_deleter(std::move(obj1));
            auto obj2 = TestObject();
            deleter del2 = make_object_deleter(std::move(obj2));
            del1.append(std::move(del2));
            tested = del1.share();
            auto obj3 = TestObject();
            deleter del3 = make_object_deleter(std::move(obj3));
            del1.append(std::move(del3));
        }
        // since deleter tested still holds references to first two objects, last objec should be deleted
        BOOST_REQUIRE(TestObject::deletions_called == 1);
    }
    BOOST_REQUIRE(TestObject::deletions_called == 3);
}

BOOST_AUTO_TEST_CASE(test_deleter_append_same_shared_object_twice) {
    TestObject::deletions_called = 0;
    {
        deleter tested;
        {
            deleter del1 = make_object_deleter(TestObject());
            auto del2 = del1.share();

            tested.append(std::move(del1));
            tested.append(std::move(del2));
        }
        BOOST_REQUIRE(TestObject::deletions_called == 0);
    }
    BOOST_REQUIRE(TestObject::deletions_called == 1);
}
