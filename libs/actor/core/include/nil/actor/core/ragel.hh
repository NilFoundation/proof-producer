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

#include <algorithm>
#include <memory>
#include <cassert>

#include <boost/optional.hpp>

#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/core/future.hh>

#include <nil/actor/detail/eclipse.hh>

namespace nil {
    namespace actor {

        // Support classes for Ragel parsers

        // Builds an sstring that can be scattered across multiple packets.
        //
        // Use a sstring_build::guard variable to designate each scattered
        // char array, and call mark_start() and mark_end() at the start
        // and end points, respectively.  sstring_builder will collect data
        // from intervening segments, if needed.
        //
        // After mark_end() has been called, use the get() method to obtain
        // the built string.
        //
        // FIXME: switch to string_view.
        //
        class sstring_builder {
            sstring _value;
            const char *_start = nullptr;

        public:
            class guard;

        public:
            sstring get() && {
                return std::move(_value);
            }
            void reset() {
                _value = {};
                _start = nullptr;
            }
            friend class guard;
        };

        class sstring_builder::guard {
            sstring_builder &_builder;
            const char *_block_end;

        public:
            guard(sstring_builder &builder, const char *block_start, const char *block_end) :
                _builder(builder), _block_end(block_end) {
                if (!_builder._value.empty()) {
                    mark_start(block_start);
                }
            }
            ~guard() {
                if (_builder._start) {
                    mark_end(_block_end);
                }
            }
            void mark_start(const char *p) {
                _builder._start = p;
            }
            void mark_end(const char *p) {
                if (_builder._value.empty()) {
                    // avoid an allocation in the common case
                    _builder._value = sstring(_builder._start, p);
                } else {
                    _builder._value += sstring(_builder._start, p);
                }
                _builder._start = nullptr;
            }
        };

        // CRTP
        template<typename ConcreteParser>
        class ragel_parser_base {
        protected:
            int _fsm_cs;
            std::unique_ptr<int[]> _fsm_stack = nullptr;
            int _fsm_stack_size = 0;
            int _fsm_top;
            int _fsm_act;
            char *_fsm_ts;
            char *_fsm_te;
            sstring_builder _builder;

        protected:
            void init_base() {
                _builder.reset();
            }
            void prepush() {
                if (_fsm_top == _fsm_stack_size) {
                    auto old = _fsm_stack_size;
                    _fsm_stack_size = std::max(_fsm_stack_size * 2, 16);
                    assert(_fsm_stack_size > old);
                    std::unique_ptr<int[]> new_stack {new int[_fsm_stack_size]};
                    std::copy(_fsm_stack.get(), _fsm_stack.get() + _fsm_top, new_stack.get());
                    std::swap(_fsm_stack, new_stack);
                }
            }
            void postpop() {
            }
            sstring get_str() {
                return std::move(_builder).get();
            }

        public:
            using unconsumed_remainder = boost::optional<temporary_buffer<char>>;
            future<unconsumed_remainder> operator()(temporary_buffer<char> buf) {
                char *p = buf.get_write();
                char *pe = p + buf.size();
                char *eof = buf.empty() ? pe : nullptr;
                char *parsed = static_cast<ConcreteParser *>(this)->parse(p, pe, eof);
                if (parsed) {
                    buf.trim_front(parsed - p);
                    return make_ready_future<unconsumed_remainder>(std::move(buf));
                }
                return make_ready_future<unconsumed_remainder>();
            }
        };

    }    // namespace actor
}    // namespace nil
