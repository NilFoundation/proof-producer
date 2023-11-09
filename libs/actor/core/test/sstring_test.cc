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
#include <nil/actor/core/sstring.hh>
#include <list>

using namespace nil::actor;

BOOST_AUTO_TEST_CASE(test_make_sstring) {
    std::string_view foo = "foo";
    std::string bar = "bar";
    sstring zed = "zed";
    const char *baz = "baz";
    BOOST_REQUIRE_EQUAL(make_sstring(foo, bar, zed, baz, "bah"), sstring("foobarzedbazbah"));
}

BOOST_AUTO_TEST_CASE(test_construction) {
    BOOST_REQUIRE_EQUAL(sstring(std::string_view("abc")), sstring("abc"));
}

BOOST_AUTO_TEST_CASE(test_equality) {
    BOOST_REQUIRE_EQUAL(sstring("aaa"), sstring("aaa"));
}

BOOST_AUTO_TEST_CASE(test_to_sstring) {
    BOOST_REQUIRE_EQUAL(to_sstring(1234567), sstring("1234567"));
}

BOOST_AUTO_TEST_CASE(test_add_literal_to_sstring) {
    BOOST_REQUIRE_EQUAL("x" + sstring("y"), sstring("xy"));
}

BOOST_AUTO_TEST_CASE(test_find_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").find('b'), 1u);
    BOOST_REQUIRE_EQUAL(sstring("babcde").find('b', 1), 2u);
}

BOOST_AUTO_TEST_CASE(test_not_find_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").find('x'), sstring::npos);
}

BOOST_AUTO_TEST_CASE(test_str_find_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").find("bc"), 1u);
    BOOST_REQUIRE_EQUAL(sstring("abcbcde").find("bc", 2), 3u);
}

BOOST_AUTO_TEST_CASE(test_str_not_find_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").find("x"), sstring::npos);
}

BOOST_AUTO_TEST_CASE(test_substr_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").substr(1, 2), "bc");
    BOOST_REQUIRE_EQUAL(sstring("abc").substr(1, 2), "bc");
    BOOST_REQUIRE_EQUAL(sstring("abc").substr(1, 3), "bc");
    BOOST_REQUIRE_EQUAL(sstring("abc").substr(0, 2), "ab");
    BOOST_REQUIRE_EQUAL(sstring("abc").substr(3, 2), "");
    BOOST_REQUIRE_EQUAL(sstring("abc").substr(1), "bc");
}

BOOST_AUTO_TEST_CASE(test_substr_eor_sstring) {
    BOOST_REQUIRE_THROW(sstring("abcde").substr(6, 1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(test_at_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("abcde").at(1), 'b');
    BOOST_REQUIRE_THROW(sstring("abcde").at(6), std::out_of_range);
    sstring s("abcde");
    s.at(1) = 'd';
    BOOST_REQUIRE_EQUAL(s, "adcde");
}

BOOST_AUTO_TEST_CASE(test_find_last_sstring) {
    BOOST_REQUIRE_EQUAL(sstring("ababa").find_last_of('a'), 4u);
    BOOST_REQUIRE_EQUAL(sstring("ababa").find_last_of('a', 5), 4u);
    BOOST_REQUIRE_EQUAL(sstring("ababa").find_last_of('a', 4), 4u);
    BOOST_REQUIRE_EQUAL(sstring("ababa").find_last_of('a', 3), 2u);
    BOOST_REQUIRE_EQUAL(sstring("ababa").find_last_of('x'), sstring::npos);
    BOOST_REQUIRE_EQUAL(sstring("").find_last_of('a'), sstring::npos);
}

BOOST_AUTO_TEST_CASE(test_append) {
    BOOST_REQUIRE_EQUAL(sstring("aba").append("1234", 3), "aba123");
    BOOST_REQUIRE_EQUAL(sstring("aba").append("1234", 4), "aba1234");
    BOOST_REQUIRE_EQUAL(sstring("aba").append("1234", 0), "aba");
}

BOOST_AUTO_TEST_CASE(test_replace) {
    BOOST_REQUIRE_EQUAL(sstring("abc").replace(1, 1, "xyz", 1), "axc");
    BOOST_REQUIRE_EQUAL(sstring("abc").replace(3, 2, "xyz", 2), "abcxy");
    BOOST_REQUIRE_EQUAL(sstring("abc").replace(2, 2, "xyz", 2), "abxy");
    BOOST_REQUIRE_EQUAL(sstring("abc").replace(0, 2, "", 0), "c");
    BOOST_REQUIRE_THROW(sstring("abc").replace(4, 1, "xyz", 1), std::out_of_range);
    const char *s = "xyz";
    sstring str("abcdef");
    BOOST_REQUIRE_EQUAL(str.replace(str.begin() + 1, str.begin() + 3, s + 1, s + 3), "ayzdef");
    BOOST_REQUIRE_THROW(sstring("abc").replace(4, 1, "xyz", 1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(test_insert) {
    sstring str("abc");
    const char *s = "xyz";
    str.insert(str.begin() + 1, s + 1, s + 2);
    BOOST_REQUIRE_EQUAL(str, "aybc");
    str = "abc";
    BOOST_REQUIRE_THROW(str.insert(str.begin() + 5, s + 1, s + 2), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(test_erase) {
    sstring str("abcdef");
    auto i = str.erase(str.begin() + 1, str.begin() + 3);
    BOOST_REQUIRE_EQUAL(*i, 'd');
    BOOST_REQUIRE_EQUAL(str, "adef");
}

BOOST_AUTO_TEST_CASE(test_ctor_iterator) {
    std::list<char> data {{'a', 'b', 'c'}};
    sstring s(data.begin(), data.end());
    BOOST_REQUIRE_EQUAL(s, "abc");
}

BOOST_AUTO_TEST_CASE(test_nul_termination) {
    using stype = basic_sstring<char, uint32_t, 15, true>;

    for (int size = 1; size <= 32; size *= 2) {
        auto s1 = uninitialized_string<stype>(size - 1);
        BOOST_REQUIRE_EQUAL(s1.c_str()[size - 1], '\0');
        auto s2 = uninitialized_string<stype>(size);
        BOOST_REQUIRE_EQUAL(s2.c_str()[size], '\0');

        s1 = stype("01234567890123456789012345678901", size - 1);
        BOOST_REQUIRE_EQUAL(s1.c_str()[size - 1], '\0');
        s2 = stype("01234567890123456789012345678901", size);
        BOOST_REQUIRE_EQUAL(s2.c_str()[size], '\0');

        s1 = stype(size - 1, ' ');
        BOOST_REQUIRE_EQUAL(s1.c_str()[size - 1], '\0');
        s2 = stype(size, ' ');
        BOOST_REQUIRE_EQUAL(s2.c_str()[size], '\0');

        s2 = s1;
        BOOST_REQUIRE_EQUAL(s2.c_str()[s1.size()], '\0');
        s2.resize(s1.size());
        BOOST_REQUIRE_EQUAL(s2.c_str()[s1.size()], '\0');
        BOOST_REQUIRE_EQUAL(s1, s2);

        auto new_size = size / 2;
        s2 = s1;
        s2.resize(new_size);
        BOOST_REQUIRE_EQUAL(s2.c_str()[new_size], '\0');
        BOOST_REQUIRE(!strncmp(s1.c_str(), s2.c_str(), new_size));

        new_size = size * 2;
        s2 = s1;
        s2.resize(new_size);
        BOOST_REQUIRE_EQUAL(s2.c_str()[new_size], '\0');
        BOOST_REQUIRE(!strncmp(s1.c_str(), s2.c_str(), std::min(s1.size(), s2.size())));

        new_size = size * 2;
        s2 = s1 + s1;
        BOOST_REQUIRE_EQUAL(s2.c_str()[s2.size()], '\0');
        BOOST_REQUIRE(!strncmp(s1.c_str(), s2.c_str(), std::min(s1.size(), s2.size())));
    }
}
