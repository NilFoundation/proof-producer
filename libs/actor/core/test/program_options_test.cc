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

#include <nil/actor/detail/program-options.hh>

#include <boost/program_options.hpp>
#include <boost/test/included/unit_test.hpp>

#include <initializer_list>
#include <vector>

namespace bpo = boost::program_options;

using namespace nil::actor;

static bpo::variables_map parse(const bpo::options_description &desc, std::initializer_list<const char *> args) {
    std::vector<const char *> raw_args {"program_options_test"};
    for (const char *arg : args) {
        raw_args.push_back(arg);
    }

    bpo::variables_map vars;
    bpo::store(bpo::parse_command_line(raw_args.size(), raw_args.data(), desc), vars);
    bpo::notify(vars);

    return vars;
}

BOOST_AUTO_TEST_CASE(string_map) {
    bpo::options_description desc;
    desc.add_options()("ages", bpo::value<program_options::string_map>());

    const auto vars = parse(desc, {"--ages", "joe=15:sally=20", "--ages", "phil=18:joe=11"});
    const auto &ages = vars["ages"].as<program_options::string_map>();

    // `string_map` values can be specified multiple times. The last association takes precedence.
    BOOST_REQUIRE_EQUAL(ages.at("joe"), "11");
    BOOST_REQUIRE_EQUAL(ages.at("phil"), "18");
    BOOST_REQUIRE_EQUAL(ages.at("sally"), "20");

    BOOST_REQUIRE_THROW(parse(desc, {"--ages", "tim:"}), bpo::invalid_option_value);
}
