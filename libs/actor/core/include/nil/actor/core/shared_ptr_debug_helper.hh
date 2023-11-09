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

#ifdef ACTOR_DEBUG_SHARED_PTR

#include <thread>
#include <cassert>

namespace nil {
    namespace actor {

        // A counter that is only comfortable being incremented on the cpu
        // it was created on.  Useful for verifying that a shared_ptr
        // or lw_shared_ptr isn't misued across cores.
        class debug_shared_ptr_counter_type {
            long _counter = 0;
            std::thread::id _cpu = std::this_thread::get_id();

        public:
            debug_shared_ptr_counter_type(long x) noexcept : _counter(x) {
            }
            operator long() const {
                check();
                return _counter;
            }
            debug_shared_ptr_counter_type &operator++() {
                check();
                ++_counter;
                return *this;
            }
            long operator++(int) {
                check();
                return _counter++;
            }
            debug_shared_ptr_counter_type &operator--() {
                check();
                --_counter;
                return *this;
            }
            long operator--(int) {
                check();
                return _counter--;
            }

        private:
            void check() const {
                assert(_cpu == std::this_thread::get_id());
            }
        };

    }    // namespace actor
}    // namespace nil

#endif
