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

#include <boost/range/numeric.hpp>

#include <nil/actor/testing/actor_test.hh>
#include <nil/actor/core/file.hh>

namespace nil {
    namespace actor {

        class mock_read_only_file final : public file_impl {
            bool _closed = false;
            uint64_t _total_file_size;
            size_t _allowed_read_requests = 0;
            std::function<void(size_t)> _verify_length;

        private:
            size_t verify_read(uint64_t position, size_t length) {
                BOOST_CHECK(!_closed);
                BOOST_CHECK_LE(position, _total_file_size);
                BOOST_CHECK_LE(position + length, _total_file_size);
                if (position + length != _total_file_size) {
                    _verify_length(length);
                }
                BOOST_CHECK(_allowed_read_requests);
                assert(_allowed_read_requests);
                _allowed_read_requests--;
                return length;
            }

        public:
            explicit mock_read_only_file(uint64_t file_size) noexcept :
                _total_file_size(file_size), _verify_length([](auto) {}) {
            }

            void set_read_size_verifier(std::function<void(size_t)> fn) {
                _verify_length = fn;
            }
            void set_expected_read_size(size_t expected) {
                _verify_length = [expected](auto length) { BOOST_CHECK_EQUAL(length, expected); };
            }
            void set_allowed_read_requests(size_t requests) {
                _allowed_read_requests = requests;
            }

            virtual future<size_t> write_dma(uint64_t, const void *, size_t,
                                             const io_priority_class &) noexcept override {
                return make_exception_future<size_t>(std::bad_function_call());
            }
            virtual future<size_t> write_dma(uint64_t, std::vector<iovec>,
                                             const io_priority_class &) noexcept override {
                return make_exception_future<size_t>(std::bad_function_call());
            }
            virtual future<size_t> read_dma(uint64_t pos, void *, size_t len,
                                            const io_priority_class &) noexcept override {
                return make_ready_future<size_t>(verify_read(pos, len));
            }
            virtual future<size_t> read_dma(uint64_t pos, std::vector<iovec> iov,
                                            const io_priority_class &) noexcept override {
                auto length =
                    boost::accumulate(iov | boost::adaptors::transformed([](auto &&iov) { return iov.iov_len; }),
                                      size_t(0), std::plus<size_t>());
                return make_ready_future<size_t>(verify_read(pos, length));
            }
            virtual future<> flush() noexcept override {
                return make_ready_future<>();
            }
            virtual future<struct stat> stat() noexcept override {
                return make_exception_future<struct stat>(std::bad_function_call());
            }
            virtual future<> truncate(uint64_t) noexcept override {
                return make_exception_future<>(std::bad_function_call());
            }
            virtual future<> discard(uint64_t offset, uint64_t length) noexcept override {
                return make_exception_future<>(std::bad_function_call());
            }
            virtual future<> allocate(uint64_t position, uint64_t length) noexcept override {
                return make_exception_future<>(std::bad_function_call());
            }
            virtual future<uint64_t> size() noexcept override {
                return make_ready_future<uint64_t>(_total_file_size);
            }
            virtual future<> close() noexcept override {
                BOOST_CHECK(!_closed);
                _closed = true;
                return make_ready_future<>();
            }
            virtual subscription<directory_entry> list_directory(std::function<future<>(directory_entry de)>) override {
                throw std::bad_function_call();
            }
            virtual future<temporary_buffer<uint8_t>> dma_read_bulk(uint64_t offset, size_t range_size,
                                                                    const io_priority_class &) noexcept override {
                auto length = verify_read(offset, range_size);
                return make_ready_future<temporary_buffer<uint8_t>>(temporary_buffer<uint8_t>(length));
            }
        };

    }    // namespace actor
}    // namespace nil
