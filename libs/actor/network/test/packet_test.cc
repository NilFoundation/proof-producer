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
#include <nil/actor/network/packet.hh>
#include <array>

using namespace nil::actor;
using namespace net;

BOOST_AUTO_TEST_CASE(test_many_fragments) {
    std::vector<char> expected;

    auto append = [&expected](net::packet p, char c, size_t n) {
        auto tmp = temporary_buffer<char>(n);
        std::fill_n(tmp.get_write(), n, c);
        std::fill_n(std::back_inserter(expected), n, c);
        return net::packet(std::move(p), std::move(tmp));
    };

    net::packet p;
    p = append(std::move(p), 'a', 5);
    p = append(std::move(p), 'b', 31);
    p = append(std::move(p), 'c', 65);
    p = append(std::move(p), 'c', 4096);
    p = append(std::move(p), 'd', 4096);

    auto verify = [&expected](const net::packet &p) {
        BOOST_CHECK_EQUAL(p.len(), expected.size());
        auto expected_it = expected.begin();
        for (auto &&frag : p.fragments()) {
            BOOST_CHECK_LE(frag.size, static_cast<size_t>(expected.end() - expected_it));
            BOOST_CHECK(std::equal(frag.base, frag.base + frag.size, expected_it));
            expected_it += frag.size;
        }
    };

    auto trim_front = [&expected](net::packet &p, size_t n) {
        p.trim_front(n);
        expected.erase(expected.begin(), expected.begin() + n);
    };

    verify(p);

    trim_front(p, 1);
    verify(p);

    trim_front(p, 6);
    verify(p);

    trim_front(p, 29);
    verify(p);

    trim_front(p, 1024);
    verify(p);

    net::packet p2;
    p2 = append(std::move(p2), 'z', 9);
    p2 = append(std::move(p2), 'x', 7);

    p.append(std::move(p2));
    verify(p);
}

BOOST_AUTO_TEST_CASE(test_headers_are_contiguous) {
    using tcp_header = std::array<char, 20>;
    using ip_header = std::array<char, 20>;
    char data[1000] = {};
    fragment f {data, sizeof(data)};
    packet p(f);
    p.prepend_header<tcp_header>();
    p.prepend_header<ip_header>();
    BOOST_REQUIRE_EQUAL(p.nr_frags(), 2u);
}

BOOST_AUTO_TEST_CASE(test_headers_are_contiguous_even_with_small_fragment) {
    using tcp_header = std::array<char, 20>;
    using ip_header = std::array<char, 20>;
    char data[100] = {};
    fragment f {data, sizeof(data)};
    packet p(f);
    p.prepend_header<tcp_header>();
    p.prepend_header<ip_header>();
    BOOST_REQUIRE_EQUAL(p.nr_frags(), 2u);
}

BOOST_AUTO_TEST_CASE(test_headers_are_contiguous_even_with_many_fragments) {
    using tcp_header = std::array<char, 20>;
    using ip_header = std::array<char, 20>;
    char data[100] = {};
    fragment f {data, sizeof(data)};
    packet p(f);
    for (int i = 0; i < 7; ++i) {
        p.append(packet(f));
    }
    p.prepend_header<tcp_header>();
    p.prepend_header<ip_header>();
    BOOST_REQUIRE_EQUAL(p.nr_frags(), 9u);
}

