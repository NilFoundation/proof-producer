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

#include <boost/range.hpp>
#include <boost/range/irange.hpp>

#include <nil/actor/testing/perf_tests.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/detail/later.hh>

struct parallel_for_each {
    std::vector<int> empty_range;
    std::vector<int> range;
    int value;

    parallel_for_each() : empty_range(), range(boost::copy_range<std::vector<int>>(boost::irange(1, 100))) {
    }
};

PERF_TEST_F(parallel_for_each, empty) {
    return nil::actor::parallel_for_each(empty_range, [](int) -> future<> { abort(); });
}

[[gnu::noinline]] future<> immediate(int v, int &vs) {
    vs += v;
    return make_ready_future<>();
}

PERF_TEST_F(parallel_for_each, immediate) {
    return nil::actor::parallel_for_each(range, [this](int v) { return immediate(v, value); }).then([this] {
        perf_tests::do_not_optimize(value);
    });
}

[[gnu::noinline]] future<> suspend(int v, int &vs) {
    vs += v;
    return later();
}

PERF_TEST_F(parallel_for_each, suspend) {
    return nil::actor::parallel_for_each(range, [this](int v) { return suspend(v, value); }).then([this] {
        perf_tests::do_not_optimize(value);
    });
}
