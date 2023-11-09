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

// Clang currently only supports the TS
#if __has_include(<coroutine>) && !defined(__clang__)
#include <coroutine>
#define ACTOR_INTERNAL_COROUTINE_NAMESPACE std
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define ACTOR_INTERNAL_COROUTINE_NAMESPACE std::experimental
#else
#define ACTOR_INTERNAL_COROUTINE_NAMESPACE std::experimental

// We are not exactly allowed to defined anything in the std namespace, but this
// makes coroutines work with libstdc++. All of this is experimental anyway.

namespace std::experimental {

    template<typename Promise>
    class coroutine_handle {
        void *_pointer = nullptr;

    public:
        coroutine_handle() = default;

        coroutine_handle &operator=(nullptr_t) noexcept {
            _pointer = nullptr;
            return *this;
        }

        explicit operator bool() const noexcept {
            return _pointer;
        }

        static coroutine_handle from_address(void *ptr) noexcept {
            coroutine_handle hndl;
            hndl._pointer = ptr;
            return hndl;
        }
        void *address() const noexcept {
            return _pointer;
        }

        static coroutine_handle from_promise(Promise &promise) noexcept {
            coroutine_handle hndl;
            hndl._pointer = __builtin_coro_promise(&promise, alignof(Promise), true);
            return hndl;
        }
        Promise &promise() const noexcept {
            return *reinterpret_cast<Promise *>(__builtin_coro_promise(_pointer, alignof(Promise), false));
        }

        void operator()() noexcept {
            resume();
        }

        void resume() const noexcept {
            __builtin_coro_resume(_pointer);
        }
        void destroy() const noexcept {
            __builtin_coro_destroy(_pointer);
        }
        bool done() const noexcept {
            return __builtin_coro_done(_pointer);
        }
    };

    struct suspend_never {
        constexpr bool await_ready() const noexcept {
            return true;
        }
        template<typename T>
        constexpr void await_suspend(coroutine_handle<T>) noexcept {
        }
        constexpr void await_resume() noexcept {
        }
    };

    struct suspend_always {
        constexpr bool await_ready() const noexcept {
            return false;
        }
        template<typename T>
        constexpr void await_suspend(coroutine_handle<T>) noexcept {
        }
        constexpr void await_resume() noexcept {
        }
    };

    template<typename T, typename... Args>
    class coroutine_traits { };

}    // namespace std::experimental

#endif
