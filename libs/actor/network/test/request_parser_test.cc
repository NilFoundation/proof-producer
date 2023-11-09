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

#include <nil/actor/core/ragel.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/http/request.hh>
#include <nil/actor/http/request_parser.hh>
#include <nil/actor/testing/test_case.hh>

#include <tuple>
#include <utility>
#include <vector>

using namespace nil::actor;

ACTOR_TEST_CASE(test_header_parsing) {
    struct test_set {
        sstring msg;
        bool parsable;
        sstring header_name = "";
        sstring header_value = "";

        temporary_buffer<char> buf() {
            return temporary_buffer<char>(msg.c_str(), msg.size());
        }
    };

    std::vector<test_set> tests = {
        {"GET /test HTTP/1.1\r\nHost: test\r\n\r\n", true, "Host", "test"},
        {"GET /hello HTTP/1.0\r\nHeader: Field\r\n\r\n", true, "Header", "Field"},
        {"GET /hello HTTP/1.0\r\nHeader: \r\n\r\n", true, "Header", ""},
        {"GET /hello HTTP/1.0\r\nHeader:  f  i e l d  \r\n\r\n", true, "Header", "f  i e l d"},
        {"GET /hello HTTP/1.0\r\nHeader: fiel\r\n    d\r\n\r\n", true, "Header", "fiel d"},
        {"GET /hello HTTP/1.0\r\ntchars.^_`|123: printable!@#%^&*()obs_text\x80\x81\xff\r\n\r\n", true,
         "tchars.^_`|123", "printable!@#%^&*()obs_text\x80\x81\xff"},
        {"GET /hello HTTP/1.0\r\nHeader: Field\r\nHeader: Field2\r\n\r\n", true, "Header", "Field,Field2"},
        {"GET /hello HTTP/1.0\r\n\r\n", true},
        {"GET /hello HTTP/1.0\r\nHeader : Field\r\n\r\n", false},
        {"GET /hello HTTP/1.0\r\nHeader Field\r\n\r\n", false},
        {"GET /hello HTTP/1.0\r\nHeader@: Field\r\n\r\n", false},
        {"GET /hello HTTP/1.0\r\nHeader: fiel\r\nd \r\n\r\n", false}};

    http_request_parser parser;
    for (auto &tset : tests) {
        parser.init();
        BOOST_REQUIRE(parser(tset.buf()).get0().has_value());
        BOOST_REQUIRE_NE(parser.failed(), tset.parsable);
        if (tset.parsable) {
            auto req = parser.get_parsed_request();
            BOOST_REQUIRE_EQUAL(req->get_header(std::move(tset.header_name)), std::move(tset.header_value));
        }
    }
    return make_ready_future<>();
}
