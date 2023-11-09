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

#pragma once

#include <atomic>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>

#include <nil/actor/core/align.hh>
#include <nil/actor/core/cacheline.hh>

namespace nil {
    namespace actor {

        template<size_t N, int RW, int LOC>
        struct prefetcher;

        template<int RW, int LOC>
        struct prefetcher<0, RW, LOC> {
            prefetcher(uintptr_t ptr) {
            }
        };

        template<size_t N, int RW, int LOC>
        struct prefetcher {
            prefetcher(uintptr_t ptr) {
                __builtin_prefetch(reinterpret_cast<void *>(ptr), RW, LOC);
                std::atomic_signal_fence(std::memory_order_seq_cst);
                prefetcher<N - cache_line_size, RW, LOC>(ptr + cache_line_size);
            }
        };

        // LOC is a locality from __buitin_prefetch() gcc documentation:
        // "The value locality must be a compile-time constant integer between zero and three. A value of
        //  zero means that the data has no temporal locality, so it need not be left in the cache after
        //  the access. A value of three means that the data has a high degree of temporal locality and
        //  should be left in all levels of cache possible. Values of one and two mean, respectively, a
        //  low or moderate degree of temporal locality. The default is three."
        template<typename T, int LOC = 3>
        void prefetch(T *ptr) {
            prefetcher<align_up(sizeof(T), cache_line_size), 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
        }

        template<typename Iterator, int LOC = 3>
        void prefetch(Iterator begin, Iterator end) {
            std::for_each(begin, end, [](auto v) { prefetch<decltype(*v), LOC>(v); });
        }

        template<size_t C, typename T, int LOC = 3>
        void prefetch_n(T **pptr) {
            boost::mpl::for_each<boost::mpl::range_c<size_t, 0, C>>(
                [pptr](size_t x) { prefetch<T, LOC>(*(pptr + x)); });
        }

        template<size_t L, int LOC = 3>
        void prefetch(void *ptr) {
            prefetcher<L * cache_line_size, 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
        }

        template<size_t L, typename Iterator, int LOC = 3>
        void prefetch_n(Iterator begin, Iterator end) {
            std::for_each(begin, end, [](auto v) { prefetch<L, LOC>(v); });
        }

        template<size_t L, size_t C, typename T, int LOC = 3>
        void prefetch_n(T **pptr) {
            boost::mpl::for_each<boost::mpl::range_c<size_t, 0, C>>(
                [pptr](size_t x) { prefetch<L, LOC>(*(pptr + x)); });
        }

        template<typename T, int LOC = 3>
        void prefetchw(T *ptr) {
            prefetcher<align_up(sizeof(T), cache_line_size), 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
        }

        template<typename Iterator, int LOC = 3>
        void prefetchw_n(Iterator begin, Iterator end) {
            std::for_each(begin, end, [](auto v) { prefetchw<decltype(*v), LOC>(v); });
        }

        template<size_t C, typename T, int LOC = 3>
        void prefetchw_n(T **pptr) {
            boost::mpl::for_each<boost::mpl::range_c<size_t, 0, C>>(
                [pptr](size_t x) { prefetchw<T, LOC>(*(pptr + x)); });
        }

        template<size_t L, int LOC = 3>
        void prefetchw(void *ptr) {
            prefetcher<L * cache_line_size, 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
        }

        template<size_t L, typename Iterator, int LOC = 3>
        void prefetchw_n(Iterator begin, Iterator end) {
            std::for_each(begin, end, [](auto v) { prefetchw<L, LOC>(v); });
        }

        template<size_t L, size_t C, typename T, int LOC = 3>
        void prefetchw_n(T **pptr) {
            boost::mpl::for_each<boost::mpl::range_c<size_t, 0, C>>(
                [pptr](size_t x) { prefetchw<L, LOC>(*(pptr + x)); });
        }

    }    // namespace actor
}    // namespace nil
