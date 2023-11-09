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

#include <nil/actor/core/detail/io_intent.hh>

#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/core/align.hh>

namespace nil {
    namespace actor {
        namespace detail {

            template<typename CharType>
            struct file_read_state {
                typedef temporary_buffer<CharType> tmp_buf_type;

                file_read_state(uint64_t offset, uint64_t front, size_t to_read, size_t memory_alignment,
                                size_t disk_alignment, io_intent *intent) :
                    buf(tmp_buf_type::aligned(memory_alignment, align_up(to_read, disk_alignment))),
                    _offset(offset), _to_read(to_read), _front(front), _iref(intent) {
                }

                bool done() const {
                    return eof || pos >= _to_read;
                }

                /**
                 * Trim the buffer to the actual number of read bytes and cut the
                 * bytes from offset 0 till "_front".
                 *
                 * @note this function has to be called only if we read bytes beyond
                 *       "_front".
                 */
                void trim_buf_before_ret() {
                    if (have_good_bytes()) {
                        buf.trim(pos);
                        buf.trim_front(_front);
                    } else {
                        buf.trim(0);
                    }
                }

                uint64_t cur_offset() const {
                    return _offset + pos;
                }

                size_t left_space() const {
                    return buf.size() - pos;
                }

                size_t left_to_read() const {
                    // positive as long as (done() == false)
                    return _to_read - pos;
                }

                void append_new_data(tmp_buf_type &new_data) {
                    auto to_copy = std::min(left_space(), new_data.size());

                    std::memcpy(buf.get_write() + pos, new_data.get(), to_copy);
                    pos += to_copy;
                }

                bool have_good_bytes() const {
                    return pos > _front;
                }

                io_intent *get_intent() {
                    return _iref.retrieve();
                }

            public:
                bool eof = false;
                tmp_buf_type buf;
                size_t pos = 0;

            private:
                uint64_t _offset;
                size_t _to_read;
                uint64_t _front;
                detail::intent_reference _iref;
            };

        }    // namespace detail
    }        // namespace actor
}    // namespace nil
