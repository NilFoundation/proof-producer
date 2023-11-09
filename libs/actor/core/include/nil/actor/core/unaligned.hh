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

// The following unaligned_cast<T*>(p) is a portable replacement for
// reinterpret_cast<T*>(p) which should be used every time address p
// is not guaranteed to be properly aligned to alignof(T).
//
// On architectures like x86 and ARM, where unaligned access is allowed,
// unaligned_cast will behave the same as reinterpret_cast and will generate
// the same code.
//
// Certain architectures (e.g., MIPS) make it extremely slow or outright
// forbidden to use ordinary machine instructions on a primitive type at an
// unaligned addresses - e.g., access a uint32_t at an address which is not
// a multiple of 4. Gcc's "undefined behavior sanitizer" (enabled in our debug
// build) also catches such unaligned accesses and reports them as errors,
// even when running on x86.
//
// Therefore, reinterpret_cast<int32_t*> on an address which is not guaranteed
// to be a multiple of 4 may generate extremely slow code or runtime errors,
// and must be avoided. The compiler needs to be told about the unaligned
// access, so it can generate reasonably-efficient code for the access
// (in MIPS, this means generating two instructions "lwl" and "lwr", instead
// of the one instruction "lw" which faults on unaligned/ access). The way to
// tell the compiler this is with __attribute__((packed)). This will also
// cause the sanitizer not to generate runtime alignment checks for this
// access.

#include <type_traits>

namespace nil {
    namespace actor {

        template<typename T>
        struct unaligned {
            // This is made to support only simple types, so it is fine to
            // require them to be trivially copy constructible.
            static_assert(std::is_trivially_copy_constructible<T>::value);
            T raw;
            unaligned() noexcept = default;
            unaligned(T x) noexcept : raw(x) {
            }
            unaligned &operator=(const T &x) noexcept {
                raw = x;
                return *this;
            }
            operator T() const noexcept {
                return raw;
            }
        } __attribute__((packed));

        // deprecated: violates strict aliasing rules
        template<typename T, typename F>
        inline auto unaligned_cast(F *p) noexcept {
            return reinterpret_cast<unaligned<std::remove_pointer_t<T>> *>(p);
        }

        // deprecated: violates strict aliasing rules
        template<typename T, typename F>
        inline auto unaligned_cast(const F *p) noexcept {
            return reinterpret_cast<const unaligned<std::remove_pointer_t<T>> *>(p);
        }

    }    // namespace actor
}    // namespace nil
