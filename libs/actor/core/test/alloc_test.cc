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
#include <nil/actor/core/memory.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/detail/memory_diagnostics.hh>

#include <vector>
#include <future>
#include <iostream>

#include <cstdlib>

using namespace nil::actor;

ACTOR_TEST_CASE(alloc_almost_all_and_realloc_it_with_a_smaller_size) {
#ifndef ACTOR_DEFAULT_ALLOCATOR
    auto all = memory::stats().total_memory();
    auto reserve = size_t(0.02 * all);
    auto to_alloc = all - (reserve + (10 << 20));
    auto orig_to_alloc = to_alloc;
    auto obj = malloc(to_alloc);
    while (!obj) {
        to_alloc *= 0.9;
        obj = malloc(to_alloc);
    }
    BOOST_REQUIRE(to_alloc > orig_to_alloc / 4);
    BOOST_REQUIRE(obj != nullptr);
    auto obj2 = realloc(obj, to_alloc - (1 << 20));
    BOOST_REQUIRE(obj == obj2);
    free(obj2);
#endif
    return make_ready_future<>();
}

ACTOR_TEST_CASE(malloc_0_and_free_it) {
#ifndef ACTOR_DEFAULT_ALLOCATOR
    auto obj = malloc(0);
    BOOST_REQUIRE(obj != nullptr);
    free(obj);
#endif
    return make_ready_future<>();
}

struct test_live_objects_counter_with_cross_cpu_free : public nil::actor::testing::actor_test {
    const char *get_test_file() override {
        return __FILE__;
    }
    const char *get_name() override {
        return "test_live_objects_counter_with_cross_cpu_free";
    }
    nil::actor::future<> run_test_case() override;
};

static test_live_objects_counter_with_cross_cpu_free test_live_objects_counter_with_cross_cpu_free_instance;

nil::actor::future<> test_live_objects_counter_with_cross_cpu_free::run_test_case() {
    return smp::submit_to(1,
                          [] {
                              auto ret = std::vector<std::unique_ptr<bool>>(1000000);
                              for (auto &o : ret) {
                                  o = std::make_unique<bool>(false);
                              }
                              return ret;
                          })
        .then([](auto &&vec) {
            vec.clear();    // cause cross-cpu free
            BOOST_REQUIRE(memory::stats().live_objects() < std::numeric_limits<size_t>::max() / 2);
        });
}

ACTOR_TEST_CASE(test_aligned_alloc) {
    for (size_t align = sizeof(void *); align <= 65536; align <<= 1) {
        for (size_t size = align; size <= align * 2; size <<= 1) {
            void *p = aligned_alloc(align, size);
            BOOST_REQUIRE(p != nullptr);
            BOOST_REQUIRE((reinterpret_cast<uintptr_t>(p) % align) == 0);
            ::memset(p, 0, size);
            free(p);
        }
    }
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_temporary_buffer_aligned) {
    for (size_t align = sizeof(void *); align <= 65536; align <<= 1) {
        for (size_t size = align; size <= align * 2; size <<= 1) {
            auto buf = temporary_buffer<char>::aligned(align, size);
            void *p = buf.get_write();
            BOOST_REQUIRE(p != nullptr);
            BOOST_REQUIRE((reinterpret_cast<uintptr_t>(p) % align) == 0);
            ::memset(p, 0, size);
        }
    }
    return make_ready_future<>();
}

ACTOR_TEST_CASE(test_memory_diagnostics) {
    auto report = memory::generate_memory_diagnostics_report();
#ifdef ACTOR_DEFAULT_ALLOCATOR
    BOOST_REQUIRE(report.length() == 0);    // empty report with default allocator
#else
    // since the output format is unstructured text, not much
    // to do except test that we get a non-empty string
    BOOST_REQUIRE(report.length() > 0);
    // useful while debugging diagnostics
    // fmt::print("--------------------\n{}--------------------", report);
#endif
    return make_ready_future<>();
}

#ifndef ACTOR_DEFAULT_ALLOCATOR

struct thread_alloc_info {
    memory::statistics before;
    memory::statistics after;
    void *ptr;
};

template<typename Func>
thread_alloc_info run_with_stats(Func &&f) {
    return std::async([&f]() {
               auto before = nil::actor::memory::stats();
               void *ptr = f();
               auto after = nil::actor::memory::stats();
               return thread_alloc_info {before, after, ptr};
           })
        .get();
}

template<typename Func>
void test_allocation_function(Func f) {
    // alien alloc and free
    auto alloc_info = run_with_stats(f);
    auto free_info = std::async([p = alloc_info.ptr]() {
                         auto before = nil::actor::memory::stats();
                         free(p);
                         auto after = nil::actor::memory::stats();
                         return thread_alloc_info {before, after, nullptr};
                     }).get();

    // there were mallocs
    BOOST_REQUIRE(alloc_info.after.foreign_mallocs() - alloc_info.before.foreign_mallocs() > 0);
    // mallocs balanced with frees
    BOOST_REQUIRE(alloc_info.after.foreign_mallocs() - alloc_info.before.foreign_mallocs() ==
                  free_info.after.foreign_frees() - free_info.before.foreign_frees());

    // alien alloc reactor free
    auto info = run_with_stats(f);
    auto before_cross_frees = memory::stats().foreign_cross_frees();
    free(info.ptr);
    BOOST_REQUIRE(memory::stats().foreign_cross_frees() - before_cross_frees == 1);

    // reactor alloc, alien free
    void *p = f();
    auto alien_cross_frees = std::async([p]() {
                                 auto frees_before = memory::stats().cross_cpu_frees();
                                 free(p);
                                 return memory::stats().cross_cpu_frees() - frees_before;
                             }).get();
    BOOST_REQUIRE(alien_cross_frees == 1);
}

ACTOR_TEST_CASE(test_foreign_function_use_glibc_malloc) {
    test_allocation_function([]() -> void * { return malloc(1); });
    test_allocation_function([]() { return realloc(NULL, 10); });
    test_allocation_function([]() {
        auto p = malloc(1);
        return realloc(p, 1000);
    });
    test_allocation_function([]() { return aligned_alloc(4, 1024); });
    return make_ready_future<>();
}
#endif
