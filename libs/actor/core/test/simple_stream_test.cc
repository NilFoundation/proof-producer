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

#define BOOST_TEST_MODULE simple_stream

#include <boost/test/included/unit_test.hpp>

#include <nil/actor/core/simple_stream.hh>

using namespace nil::actor;

template<typename Input, typename Output>
static void write_read_test(Input in, Output out) {
    auto aa = std::vector<char>(4, 'a');
    auto bb = std::vector<char>(3, 'b');
    auto cc = std::vector<char>(2, 'c');

    out.write(aa.data(), aa.size());
    out.fill('b', 3);

    BOOST_CHECK_THROW(out.fill(' ', 3), std::out_of_range);
    BOOST_CHECK_THROW(out.write("   ", 3), std::out_of_range);

    out.write(cc.data(), cc.size());

    BOOST_CHECK_THROW(out.fill(' ', 1), std::out_of_range);
    BOOST_CHECK_THROW(out.write(" ", 1), std::out_of_range);

    auto actual_aa = std::vector<char>(4);
    in.read(actual_aa.data(), actual_aa.size());
    BOOST_CHECK_EQUAL(aa, actual_aa);

    auto actual_bb = std::vector<char>(3);
    in.read(actual_bb.data(), actual_bb.size());
    BOOST_CHECK_EQUAL(bb, actual_bb);

    actual_aa.resize(1024);
    BOOST_CHECK_THROW(in.read(actual_aa.data(), actual_aa.size()), std::out_of_range);

    auto actual_cc = std::vector<char>(2);
    in.read(actual_cc.data(), actual_cc.size());
    BOOST_CHECK_EQUAL(cc, actual_cc);

    BOOST_CHECK_THROW(in.read(actual_aa.data(), 1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(simple_write_read_test) {
    auto buf = temporary_buffer<char>(9);

    write_read_test(simple_memory_input_stream(buf.get(), buf.size()),
                    simple_memory_output_stream(buf.get_write(), buf.size()));

    std::fill_n(buf.get_write(), buf.size(), 'x');

    auto out = simple_memory_output_stream(buf.get_write(), buf.size());
    write_read_test(out.to_input_stream(), out);
}

BOOST_AUTO_TEST_CASE(fragmented_write_read_test) {
    static constexpr size_t total_size = 9;

    auto bufs = std::vector<temporary_buffer<char>>();
    using iterator_type = std::vector<temporary_buffer<char>>::iterator;

    auto test = [&] {
        write_read_test(fragmented_memory_input_stream<iterator_type>(bufs.begin(), total_size),
                        fragmented_memory_output_stream<iterator_type>(bufs.begin(), total_size));

        auto out = fragmented_memory_output_stream<iterator_type>(bufs.begin(), total_size);
        write_read_test(out.to_input_stream(), out);
    };

    bufs.emplace_back(total_size);
    test();

    bufs.clear();
    for (auto i = 0u; i < total_size; i++) {
        bufs.emplace_back(1);
    }
    test();
}
