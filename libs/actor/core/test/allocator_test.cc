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

#include <nil/actor/core/memory.hh>
#include <nil/actor/core/timer.hh>
#include <nil/actor/testing/test_runner.hh>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <memory>
#include <chrono>
#include <boost/program_options.hpp>

using namespace nil::actor;

struct allocation {
    size_t n;
    std::unique_ptr<char[]> data;
    char poison;
    allocation(size_t n, char poison) : n(n), data(new char[n]), poison(poison) {
        std::fill_n(data.get(), n, poison);
    }
    ~allocation() {
        verify();
    }
    allocation(allocation &&x) noexcept = default;
    void verify() {
        if (data) {
            assert(std::find_if(data.get(), data.get() + n, [this](char c) { return c != poison; }) == data.get() + n);
        }
    }
    allocation &operator=(allocation &&x) {
        verify();
        if (this != &x) {
            data = std::move(x.data);
            n = x.n;
            poison = x.poison;
        }
        return *this;
    }
};

#ifdef __cpp_aligned_new

template<size_t N>
struct alignas(N) cpp17_allocation final {
    char v;
};

struct test17 {
    struct handle {
        const test17 *d;
        void *p;
        handle(const test17 *d, void *p) : d(d), p(p) {
        }
        handle(const handle &) = delete;
        handle(handle &&x) noexcept : d(std::exchange(x.d, nullptr)), p(std::exchange(x.p, nullptr)) {
        }
        handle &operator=(const handle &) = delete;
        handle &operator=(handle &&x) noexcept {
            std::swap(d, x.d);
            std::swap(p, x.p);
            return *this;
        }
        ~handle() {
            if (d) {
                d->free(p);
            }
        }
    };
    virtual ~test17() {
    }
    virtual handle alloc() const = 0;
    virtual void free(void *ptr) const = 0;
};

template<size_t N>
struct test17_concrete : test17 {
    using value_type = cpp17_allocation<N>;
    static_assert(sizeof(value_type) == N, "language does not guarantee size >= align");
    virtual handle alloc() const override {
        auto ptr = new value_type();
        assert((reinterpret_cast<uintptr_t>(ptr) & (N - 1)) == 0);
        return handle {this, ptr};
    }
    virtual void free(void *ptr) const override {
        delete static_cast<value_type *>(ptr);
    }
};

void test_cpp17_aligned_allocator() {
    std::vector<std::unique_ptr<test17>> tv;
    tv.push_back(std::make_unique<test17_concrete<1>>());
    tv.push_back(std::make_unique<test17_concrete<2>>());
    tv.push_back(std::make_unique<test17_concrete<4>>());
    tv.push_back(std::make_unique<test17_concrete<8>>());
    tv.push_back(std::make_unique<test17_concrete<16>>());
    tv.push_back(std::make_unique<test17_concrete<64>>());
    tv.push_back(std::make_unique<test17_concrete<128>>());
    tv.push_back(std::make_unique<test17_concrete<2048>>());
    tv.push_back(std::make_unique<test17_concrete<4096>>());
    tv.push_back(std::make_unique<test17_concrete<4096 * 16>>());
    tv.push_back(std::make_unique<test17_concrete<4096 * 256>>());

    std::default_random_engine random_engine(testing::local_random_engine());
    std::uniform_int_distribution<> type_dist(0, 1);
    std::uniform_int_distribution<size_t> size_dist(0, tv.size() - 1);
    std::uniform_real_distribution<> which_dist(0, 1);

    std::vector<test17::handle> allocs;
    for (unsigned i = 0; i < 10000; ++i) {
        auto type = type_dist(random_engine);
        switch (type) {
            case 0: {
                size_t sz_idx = size_dist(random_engine);
                allocs.push_back(tv[sz_idx]->alloc());
                break;
            }
            case 1:
                if (!allocs.empty()) {
                    size_t idx = which_dist(random_engine) * allocs.size();
                    std::swap(allocs[idx], allocs.back());
                    allocs.pop_back();
                }
                break;
        }
    }
}

#else

void test_cpp17_aligned_allocator() {
}

#endif

int main(int ac, char **av) {
    namespace bpo = boost::program_options;
    bpo::options_description opts("Allowed options");
    opts.add_options()("help", "produce this help message")(
        "iterations",
        bpo::value<unsigned>(), "run s specified number of iterations")("time", bpo::value<float>()->default_value(5.0),
                                                                        "run for a specified amount of time, in "
                                                                        "seconds")("random-seed",
                                                                                   boost::program_options::value<
                                                                                       unsigned>(),
                                                                                   "Random number generator seed");
    ;
    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(ac, av, opts), vm);
    bpo::notify(vm);
    test_cpp17_aligned_allocator();
    auto seed = vm.count("random-seed") ? vm["random-seed"].as<unsigned>() : std::random_device {}();
    std::default_random_engine random_engine(seed);
    std::exponential_distribution<> distr(0.2);
    std::uniform_int_distribution<> type(0, 1);
    std::uniform_int_distribution<char> poison(-128, 127);
    std::uniform_real_distribution<> which(0, 1);
    std::vector<allocation> allocations;
    auto iteration = [&] {
        auto typ = type(random_engine);
        switch (typ) {
            case 0: {
                size_t n = std::min<double>(std::exp(distr(random_engine)), 1 << 25);
                try {
                    allocations.emplace_back(n, poison(random_engine));
                } catch (std::bad_alloc &) {
                }
                break;
            }
            case 1: {
                if (allocations.empty()) {
                    break;
                }
                size_t i = which(random_engine) * allocations.size();
                allocations[i] = std::move(allocations.back());
                allocations.pop_back();
                break;
            }
        }
    };
    if (vm.count("help")) {
        std::cout << opts << "\n";
        return 1;
    }
    std::cout << "random-seed=" << seed << "\n";
    if (vm.count("iterations")) {
        auto iterations = vm["iterations"].as<unsigned>();
        for (unsigned i = 0; i < iterations; ++i) {
            iteration();
        }
    } else {
        auto time = vm["time"].as<float>();
        using clock = steady_clock_type;
        auto end = clock::now() + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1) * time);
        while (clock::now() < end) {
            for (unsigned i = 0; i < 1000; ++i) {
                iteration();
            }
        }
    }
    return 0;
}
