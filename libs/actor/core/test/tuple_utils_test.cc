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

#include <nil/actor/detail/tuple_utils.hh>

#include <boost/test/included/unit_test.hpp>

#include <sstream>
#include <type_traits>

using namespace nil::actor;

BOOST_AUTO_TEST_CASE(map) {
    const auto pairs = tuple_map(std::make_tuple(10, 5.5, true), [](auto &&e) { return std::make_tuple(e, e); });

    BOOST_REQUIRE(pairs ==
                  std::make_tuple(std::make_tuple(10, 10), std::make_tuple(5.5, 5.5), std::make_tuple(true, true)));
}

BOOST_AUTO_TEST_CASE(for_each) {
    std::ostringstream os;

    tuple_for_each(std::make_tuple('a', 10, false, 5.4), [&os](auto &&e) { os << e; });

    BOOST_REQUIRE_EQUAL(os.str(), "a1005.4");
}

namespace {

    template<typename T>
    struct transform_type final {
        using type = T;
    };

    template<>
    struct transform_type<bool> final {
        using type = int;
    };

    template<>
    struct transform_type<double> final {
        using type = char;
    };

}    // namespace

BOOST_AUTO_TEST_CASE(map_types) {
    using before_tuple = std::tuple<double, bool, const char *>;
    using after_tuple = typename tuple_map_types<transform_type, before_tuple>::type;

    BOOST_REQUIRE((std::is_same<after_tuple, std::tuple<char, int, const char *>>::value));
}

namespace {

    //
    // Strip all `bool` fields.
    //

    template<typename>
    struct keep_type final {
        static constexpr auto value = true;
    };

    template<>
    struct keep_type<bool> final {
        static constexpr auto value = false;
    };

}    // namespace

BOOST_AUTO_TEST_CASE(filter_by_type) {
    using before_tuple = std::tuple<bool, int, bool, double, bool, char>;

    const auto t = tuple_filter_by_type<keep_type>(before_tuple {true, 10, false, 5.5, true, 'a'});
    using filtered_type = typename std::decay<decltype(t)>::type;

    BOOST_REQUIRE((std::is_same<filtered_type, std::tuple<int, double, char>>::value));
    BOOST_REQUIRE(t == std::make_tuple(10, 5.5, 'a'));
}
