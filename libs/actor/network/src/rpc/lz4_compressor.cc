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

#include <nil/actor/rpc/lz4_compressor.hh>
#include <nil/actor/core/byteorder.hh>

namespace nil {
    namespace actor {

        namespace rpc {

            sstring lz4_compressor::name() const {
                return factory {}.supported();
            }

            const sstring &lz4_compressor::factory::supported() const {
                const static sstring name = "LZ4";
                return name;
            }

            std::unique_ptr<rpc::compressor> lz4_compressor::factory::negotiate(sstring feature, bool is_server) const {
                return feature == supported() ? std::make_unique<lz4_compressor>() : nullptr;
            }

            // Reusable contiguous buffers needed for LZ4 compression and decompression functions.
            class reusable_buffer {
                static constexpr size_t chunk_size = 128 * 1024;
                static_assert(snd_buf::chunk_size == chunk_size, "snd_buf::chunk_size == chunk_size");

                std::unique_ptr<char[]> _data;
                size_t _size;

            private:
                void reserve(size_t n) {
                    if (_size < n) {
                        _data.reset();
                        // Not using std::make_unique to avoid value-initialisation.
                        _data = std::unique_ptr<char[]>(new char[n]);
                        _size = n;
                    }
                }

            public:
                // Returns a pointer to a contiguous buffer containing all data stored in input.
                // The pointer remains valid until next call to this.
                const char *
                    prepare(const std::variant<std::vector<temporary_buffer<char>>, temporary_buffer<char>> &input,
                            size_t size) {
                    if (const auto single = std::get_if<temporary_buffer<char>>(&input)) {
                        return single->get();
                    }
                    reserve(size);
                    auto dst = _data.get();
                    for (const auto &fragment : std::get<std::vector<temporary_buffer<char>>>(input)) {
                        dst = std::copy_n(fragment.begin(), fragment.size(), dst);
                    }
                    return _data.get();
                }

                // Calls function fn passing to it a pointer to a temporary contigiuous max_size
                // buffer.
                // fn is supposed to return the actual size of the data.
                // with_reserved() returns an Output object (snd_buf or rcv_buf or compatible),
                // containing data that was written to the temporary buffer.
                // Output should be either snd_buf or rcv_buf.
                template<typename Output, typename Function>
#ifdef BOOST_HAS_CONCEPTS
                requires requires(Function fn, char *ptr) {
                    { fn(ptr) }
                    ->std::convertible_to<size_t>;
                }
                &&(std::is_same<Output, snd_buf>::value || std::is_same<Output, rcv_buf>::value)
#endif
                    Output with_reserved(size_t max_size, Function &&fn) {
                    if (max_size <= chunk_size) {
                        auto dst = temporary_buffer<char>(max_size);
                        size_t dst_size = fn(dst.get_write());
                        dst.trim(dst_size);
                        return Output(std::move(dst));
                    }

                    reserve(max_size);
                    size_t dst_size = fn(_data.get());
                    if (dst_size <= chunk_size) {
                        return Output(temporary_buffer<char>(_data.get(), dst_size));
                    }

                    auto left = dst_size;
                    auto pos = _data.get();
                    std::vector<temporary_buffer<char>> buffers;
                    while (left) {
                        auto this_size = std::min(left, chunk_size);
                        buffers.emplace_back(this_size);
                        std::copy_n(pos, this_size, buffers.back().get_write());
                        pos += this_size;
                        left -= this_size;
                    }
                    return Output(std::move(buffers), dst_size);
                }

                void clear() noexcept {
                    _data.reset();
                    _size = 0;
                }
            };

            // in cpp 14 declaration of static variables is mandatory even
            // though the assignment took place inside the class declaration
            // - no inline static variables (like in Cpp17).
            constexpr size_t reusable_buffer::chunk_size;

            static thread_local reusable_buffer reusable_buffer_compressed_data;
            static thread_local reusable_buffer reusable_buffer_decompressed_data;
            static thread_local size_t buffer_use_count = 0;
            static constexpr size_t drop_buffers_trigger = 100'000;

            static void after_buffer_use() noexcept {
                if (buffer_use_count++ == drop_buffers_trigger) {
                    reusable_buffer_compressed_data.clear();
                    reusable_buffer_decompressed_data.clear();
                    buffer_use_count = 0;
                }
            }

            snd_buf lz4_compressor::compress(size_t head_space, snd_buf data) {
                head_space += 4;
                auto dst_size = head_space + LZ4_compressBound(data.size);
                auto dst = reusable_buffer_compressed_data.with_reserved<snd_buf>(dst_size, [&](char *dst) {
                    auto src_size = data.size;
                    auto src = reusable_buffer_decompressed_data.prepare(data.bufs, data.size);

#ifdef ACTOR_HAVE_LZ4_COMPRESS_DEFAULT
                    auto size = LZ4_compress_default(src, dst + head_space, src_size, LZ4_compressBound(src_size));
#else
        // Safe since output buffer is sized properly.
        auto size = LZ4_compress(src, dst + head_space, src_size);
#endif
                    if (size == 0) {
                        throw std::runtime_error("RPC frame LZ4 compression failure");
                    }
                    write_le<uint32_t>(dst + (head_space - 4), src_size);
                    return size + head_space;
                });
                after_buffer_use();
                return dst;
            }

            rcv_buf lz4_compressor::decompress(rcv_buf data) {
                if (data.size < 4) {
                    return rcv_buf();
                } else {
                    auto src_size = data.size;
                    auto src = reusable_buffer_decompressed_data.prepare(data.bufs, data.size);

                    auto dst_size = read_le<uint32_t>(src);
                    if (!dst_size) {
                        throw std::runtime_error(
                            "RPC frame LZ4 decompression failure: decompressed size cannot be zero");
                    }
                    src += sizeof(uint32_t);
                    src_size -= sizeof(uint32_t);

                    auto dst = reusable_buffer_compressed_data.with_reserved<rcv_buf>(dst_size, [&](char *dst) {
                        if (LZ4_decompress_safe(src, dst, src_size, dst_size) < 0) {
                            throw std::runtime_error("RPC frame LZ4 decompression failure");
                        }
                        return dst_size;
                    });
                    after_buffer_use();
                    return dst;
                }
            }

        }    // namespace rpc

    }    // namespace actor
}    // namespace nil
