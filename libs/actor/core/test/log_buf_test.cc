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

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/detail/log.hh>

using namespace nil::actor;

ACTOR_TEST_CASE(log_buf_realloc) {
    std::array<char, 128> external_buf;

    const auto external_buf_ptr = reinterpret_cast<uintptr_t>(external_buf.data());

    detail::log_buf b(external_buf.data(), external_buf.size());

    BOOST_REQUIRE_EQUAL(reinterpret_cast<uintptr_t>(b.data()), external_buf_ptr);

    auto it = b.back_insert_begin();

    BOOST_REQUIRE_EQUAL(reinterpret_cast<uintptr_t>(&*it), external_buf_ptr);

    for (auto i = 0; i < 128; ++i) {
        *it++ = 'a';
    }

    *it = 'a';    // should trigger realloc

    BOOST_REQUIRE_NE(reinterpret_cast<uintptr_t>(b.data()), reinterpret_cast<uintptr_t>(external_buf.data()));
    BOOST_REQUIRE_NE(reinterpret_cast<uintptr_t>(&*it), reinterpret_cast<uintptr_t>(external_buf.data() + 128));

    const char *p = b.data();
    for (auto i = 0; i < 129; ++i) {
        BOOST_REQUIRE_EQUAL(p[i], 'a');
    }

    return make_ready_future<>();
}
