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

#include <nil/actor/core/detail/uname.hh>

using namespace nil::actor::detail;

BOOST_AUTO_TEST_CASE(test_nowait_aio_fix) {
    auto check = [](const char *uname) {
        return parse_uname(uname).whitelisted({"5.1", "5.0.8", "4.19.35", "4.14.112"});
    };
    BOOST_REQUIRE_EQUAL(check("5.1.0"), true);
    BOOST_REQUIRE_EQUAL(check("5.1.1"), true);
    BOOST_REQUIRE_EQUAL(check("5.1.1-44.distro"), true);
    BOOST_REQUIRE_EQUAL(check("5.1.1-44.7.distro"), true);
    BOOST_REQUIRE_EQUAL(check("5.0.0"), false);
    BOOST_REQUIRE_EQUAL(check("5.0.7"), false);
    BOOST_REQUIRE_EQUAL(check("5.0.7-55.el19"), false);
    BOOST_REQUIRE_EQUAL(check("5.0.8"), true);
    BOOST_REQUIRE_EQUAL(check("5.0.9"), true);
    BOOST_REQUIRE_EQUAL(check("5.0.8-200.fedora"), true);
    BOOST_REQUIRE_EQUAL(check("5.0.9-200.fedora"), true);
    BOOST_REQUIRE_EQUAL(check("5.2.0"), true);
    BOOST_REQUIRE_EQUAL(check("5.2.9"), true);
    BOOST_REQUIRE_EQUAL(check("5.2.9-77.el153"), true);
    BOOST_REQUIRE_EQUAL(check("6.0.0"), true);
    BOOST_REQUIRE_EQUAL(check("3.9.0"), false);
    BOOST_REQUIRE_EQUAL(check("4.19"), false);
    BOOST_REQUIRE_EQUAL(check("4.19.34"), false);
    BOOST_REQUIRE_EQUAL(check("4.19.35"), true);
    BOOST_REQUIRE_EQUAL(check("4.19.36"), true);
    BOOST_REQUIRE_EQUAL(check("4.20.36"), false);
    BOOST_REQUIRE_EQUAL(check("4.14.111"), false);
    BOOST_REQUIRE_EQUAL(check("4.14.112"), true);
    BOOST_REQUIRE_EQUAL(check("4.14.113"), true);
}

BOOST_AUTO_TEST_CASE(test_xfs_concurrency_fix) {
    auto check = [](const char *uname) { return parse_uname(uname).whitelisted({"3.15", "3.10.0-325.el7"}); };
    BOOST_REQUIRE_EQUAL(check("3.15.0"), true);
    BOOST_REQUIRE_EQUAL(check("5.1.0"), true);
    BOOST_REQUIRE_EQUAL(check("3.14.0"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.0"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.14"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.0-325.ubuntu"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.0-325"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.0-325.el7"), true);
    BOOST_REQUIRE_EQUAL(check("3.10.0-326.el7"), true);
    BOOST_REQUIRE_EQUAL(check("3.10.0-324.el7"), false);
    BOOST_REQUIRE_EQUAL(check("3.10.0-325.665.el7"), true);
}
