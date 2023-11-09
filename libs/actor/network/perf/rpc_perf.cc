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

#include <random>

#include <nil/actor/rpc/lz4_compressor.hh>
#include <nil/actor/rpc/lz4_fragmented_compressor.hh>

#include <nil/actor/testing/perf_tests.hh>
#include <nil/actor/testing/random.hh>

template<typename Compressor>
struct compression {
    static constexpr size_t small_buffer_size = 128;
    static constexpr size_t large_buffer_size = 16 * 1024 * 1024;

private:
    Compressor _compressor;

    nil::actor::temporary_buffer<char> _small_buffer_random;
    nil::actor::temporary_buffer<char> _small_buffer_zeroes;

    std::vector<nil::actor::temporary_buffer<char>> _large_buffer_random;
    std::vector<nil::actor::temporary_buffer<char>> _large_buffer_zeroes;

    std::vector<nil::actor::temporary_buffer<char>> _small_compressed_buffer_random;
    std::vector<nil::actor::temporary_buffer<char>> _small_compressed_buffer_zeroes;

    std::vector<nil::actor::temporary_buffer<char>> _large_compressed_buffer_random;
    std::vector<nil::actor::temporary_buffer<char>> _large_compressed_buffer_zeroes;

private:
    static nil::actor::rpc::rcv_buf get_rcv_buf(std::vector<temporary_buffer<char>> &input) {
        if (input.size() == 1) {
            return nil::actor::rpc::rcv_buf(input.front().share());
        }
        auto bufs = std::vector<temporary_buffer<char>> {};
        auto total_size =
            std::accumulate(input.begin(), input.end(), size_t(0), [&](size_t n, temporary_buffer<char> &buf) {
                bufs.emplace_back(buf.share());
                return n + buf.size();
            });
        return nil::actor::rpc::rcv_buf(std::move(bufs), total_size);
    }

    static nil::actor::rpc::snd_buf get_snd_buf(std::vector<temporary_buffer<char>> &input) {
        auto bufs = std::vector<temporary_buffer<char>> {};
        auto total_size =
            std::accumulate(input.begin(), input.end(), size_t(0), [&](size_t n, temporary_buffer<char> &buf) {
                bufs.emplace_back(buf.share());
                return n + buf.size();
            });
        return nil::actor::rpc::snd_buf(std::move(bufs), total_size);
    }
    static nil::actor::rpc::snd_buf get_snd_buf(temporary_buffer<char> &input) {
        return nil::actor::rpc::snd_buf(input.share());
    }

public:
    compression() :
        _small_buffer_random(nil::actor::temporary_buffer<char>(small_buffer_size)),
        _small_buffer_zeroes(nil::actor::temporary_buffer<char>(small_buffer_size)) {
        auto &eng = testing::local_random_engine;
        auto dist = std::uniform_int_distribution<char>();

        std::generate_n(_small_buffer_random.get_write(), small_buffer_size, [&] { return dist(eng); });
        for (auto i = 0u; i < large_buffer_size / nil::actor::rpc::snd_buf::chunk_size; i++) {
            _large_buffer_random.emplace_back(nil::actor::rpc::snd_buf::chunk_size);
            std::generate_n(_large_buffer_random.back().get_write(), nil::actor::rpc::snd_buf::chunk_size,
                            [&] { return dist(eng); });
            _large_buffer_zeroes.emplace_back(nil::actor::rpc::snd_buf::chunk_size);
            std::fill_n(_large_buffer_zeroes.back().get_write(), nil::actor::rpc::snd_buf::chunk_size, 0);
        }

        auto rcv = _compressor.compress(0, nil::actor::rpc::snd_buf(_small_buffer_random.share()));
        if (auto buffer = std::get_if<nil::actor::temporary_buffer<char>>(&rcv.bufs)) {
            _small_compressed_buffer_random.emplace_back(std::move(*buffer));
        } else {
            _small_compressed_buffer_random =
                std::move(std::get<std::vector<nil::actor::temporary_buffer<char>>>(rcv.bufs));
        }

        rcv = _compressor.compress(0, nil::actor::rpc::snd_buf(_small_buffer_zeroes.share()));
        if (auto buffer = std::get_if<nil::actor::temporary_buffer<char>>(&rcv.bufs)) {
            _small_compressed_buffer_zeroes.emplace_back(std::move(*buffer));
        } else {
            _small_compressed_buffer_zeroes =
                std::move(std::get<std::vector<nil::actor::temporary_buffer<char>>>(rcv.bufs));
        }

        auto bufs = std::vector<temporary_buffer<char>> {};
        for (auto &&b : _large_buffer_random) {
            bufs.emplace_back(b.clone());
        }
        rcv = _compressor.compress(0, nil::actor::rpc::snd_buf(std::move(bufs), large_buffer_size));
        if (auto buffer = std::get_if<nil::actor::temporary_buffer<char>>(&rcv.bufs)) {
            _large_compressed_buffer_random.emplace_back(std::move(*buffer));
        } else {
            _large_compressed_buffer_random =
                std::move(std::get<std::vector<nil::actor::temporary_buffer<char>>>(rcv.bufs));
        }

        bufs = std::vector<temporary_buffer<char>> {};
        for (auto &&b : _large_buffer_zeroes) {
            bufs.emplace_back(b.clone());
        }
        rcv = _compressor.compress(0, nil::actor::rpc::snd_buf(std::move(bufs), large_buffer_size));
        if (auto buffer = std::get_if<nil::actor::temporary_buffer<char>>(&rcv.bufs)) {
            _large_compressed_buffer_zeroes.emplace_back(std::move(*buffer));
        } else {
            _large_compressed_buffer_zeroes =
                std::move(std::get<std::vector<nil::actor::temporary_buffer<char>>>(rcv.bufs));
        }
    }

    Compressor &compressor() {
        return _compressor;
    }

    nil::actor::rpc::snd_buf small_buffer_random() {
        return get_snd_buf(_small_buffer_random);
    }
    nil::actor::rpc::snd_buf small_buffer_zeroes() {
        return get_snd_buf(_small_buffer_zeroes);
    }

    nil::actor::rpc::snd_buf large_buffer_random() {
        return get_snd_buf(_large_buffer_random);
    }
    nil::actor::rpc::snd_buf large_buffer_zeroes() {
        return get_snd_buf(_large_buffer_zeroes);
    }

    nil::actor::rpc::rcv_buf small_compressed_buffer_random() {
        return get_rcv_buf(_small_compressed_buffer_random);
    }
    nil::actor::rpc::rcv_buf small_compressed_buffer_zeroes() {
        return get_rcv_buf(_small_compressed_buffer_zeroes);
    }

    nil::actor::rpc::rcv_buf large_compressed_buffer_random() {
        return get_rcv_buf(_large_compressed_buffer_random);
    }
    nil::actor::rpc::rcv_buf large_compressed_buffer_zeroes() {
        return get_rcv_buf(_large_compressed_buffer_zeroes);
    }
};

using lz4 = compression<nil::actor::rpc::lz4_compressor>;

PERF_TEST_F(lz4, small_random_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, small_buffer_random()));
}

PERF_TEST_F(lz4, small_zeroed_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, small_buffer_zeroes()));
}

PERF_TEST_F(lz4, large_random_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, large_buffer_random()));
}

PERF_TEST_F(lz4, large_zeroed_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, large_buffer_zeroes()));
}

PERF_TEST_F(lz4, small_random_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(small_compressed_buffer_random()));
}

PERF_TEST_F(lz4, small_zeroed_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(small_compressed_buffer_zeroes()));
}

PERF_TEST_F(lz4, large_random_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(large_compressed_buffer_random()));
}

PERF_TEST_F(lz4, large_zeroed_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(large_compressed_buffer_zeroes()));
}

using lz4_fragmented = compression<nil::actor::rpc::lz4_fragmented_compressor>;

PERF_TEST_F(lz4_fragmented, small_random_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, small_buffer_random()));
}

PERF_TEST_F(lz4_fragmented, small_zeroed_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, small_buffer_zeroes()));
}

PERF_TEST_F(lz4_fragmented, large_random_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, large_buffer_random()));
}

PERF_TEST_F(lz4_fragmented, large_zeroed_buffer_compress) {
    perf_tests::do_not_optimize(compressor().compress(0, large_buffer_zeroes()));
}

PERF_TEST_F(lz4_fragmented, small_random_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(small_compressed_buffer_random()));
}

PERF_TEST_F(lz4_fragmented, small_zeroed_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(small_compressed_buffer_zeroes()));
}

PERF_TEST_F(lz4_fragmented, large_random_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(large_compressed_buffer_random()));
}

PERF_TEST_F(lz4_fragmented, large_zeroed_buffer_decompress) {
    perf_tests::do_not_optimize(compressor().decompress(large_compressed_buffer_zeroes()));
}
