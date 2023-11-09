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

#include <iterator>

/// \addtogroup logging
/// @{

namespace nil {
    namespace actor {

        /// \cond internal
        namespace detail {

            /// A buffer to format log messages into.
            ///
            /// It was designed to allow formatting the entire message into it, without any
            /// intermediary buffers. To minimize the amount of reallocations it supports
            /// using an external buffer. When this is full it moves to using buffers
            /// allocated by itself.
            /// To accommodate the most widely used way of formatting messages -- fmt --,
            /// it provides an output iterator interface for writing into it.
            class log_buf {
                char *_begin;
                char *_end;
                char *_current;
                bool _own_buf;

            private:
                void free_buffer() noexcept;
                void realloc_buffer();

            public:
                class inserter_iterator {
                public:
                    using iterator_category = std::output_iterator_tag;
                    using difference_type = std::ptrdiff_t;
                    using value_type = char;
                    using pointer = char *;
                    using reference = char &;

                private:
                    log_buf *_buf;
                    char *_current;

                public:
                    explicit inserter_iterator(log_buf &buf) noexcept : _buf(&buf), _current(_buf->_current) {
                    }
                    inserter_iterator(const inserter_iterator &o) noexcept : _buf(o._buf), _current(o._current) {
                    }

                    reference operator*() {
                        if (__builtin_expect(_current == _buf->_end, false)) {
                            _buf->realloc_buffer();
                            _current = _buf->_current;
                        }
                        return *_current;
                    }
                    inserter_iterator &operator++() noexcept {
                        if (__builtin_expect(_current == _buf->_current, true)) {
                            ++_buf->_current;
                        }
                        ++_current;
                        return *this;
                    }
                    inserter_iterator operator++(int) noexcept {
                        inserter_iterator o(*this);
                        ++(*this);
                        return o;
                    }
                };

                /// Default ctor.
                ///
                /// Allocates an internal buffer of 512 bytes.
                log_buf();
                /// External buffer ctor.
                ///
                /// Use the external buffer until its full, then switch to internally
                /// allocated buffer. log_buf doesn't take ownership of the buffer.
                log_buf(char *external_buf, size_t size) noexcept;
                ~log_buf();
                /// Create an output iterator which allows writing into the buffer.
                inserter_iterator back_insert_begin() noexcept {
                    return inserter_iterator(*this);
                }
                /// The amount of data written so far.
                const size_t size() const noexcept {
                    return _current - _begin;
                }
                /// The size of the buffer.
                const size_t capacity() const noexcept {
                    return _end - _begin;
                }
                /// Read only pointer to the buffer.
                /// Note that the buffer is not guaranteed to be null terminated. The writer
                /// has to ensure that, should it wish to.
                const char *data() const noexcept {
                    return _begin;
                }
                /// A view of the buffer content.
                std::string_view view() const noexcept {
                    return std::string_view(_begin, size());
                }
            };

        }    // namespace detail
        /// \endcond

    }    // namespace actor
}    // namespace nil
