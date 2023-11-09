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

#include <vector>

#include <nil/actor/core/do_with.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/json/formatter.hh>

using namespace nil::actor;
using namespace json;

ACTOR_TEST_CASE(test_simple_values) {
    BOOST_CHECK_EQUAL("3", formatter::to_json(3));
    BOOST_CHECK_EQUAL("3", formatter::to_json(3.0));
    BOOST_CHECK_EQUAL("3.5", formatter::to_json(3.5));
    BOOST_CHECK_EQUAL("true", formatter::to_json(true));
    BOOST_CHECK_EQUAL("false", formatter::to_json(false));
    BOOST_CHECK_EQUAL("\"apa\"", formatter::to_json("apa"));

    return make_ready_future();
}

ACTOR_TEST_CASE(test_collections) {
    BOOST_CHECK_EQUAL("{1:2,3:4}", formatter::to_json(std::map<int, int>({{1, 2}, {3, 4}})));
    BOOST_CHECK_EQUAL("[1,2,3,4]", formatter::to_json(std::vector<int>({1, 2, 3, 4})));
    BOOST_CHECK_EQUAL("[{1:2},{3:4}]", formatter::to_json(std::vector<std::pair<int, int>>({{1, 2}, {3, 4}})));
    BOOST_CHECK_EQUAL("[{1:2},{3:4}]", formatter::to_json(std::vector<std::map<int, int>>({{{1, 2}}, {{3, 4}}})));
    BOOST_CHECK_EQUAL("[[1,2],[3,4]]", formatter::to_json(std::vector<std::vector<int>>({{1, 2}, {3, 4}})));

    return make_ready_future();
}
