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

#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/vector-data-sink.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/detail/later.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/network/packet.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <vector>

using namespace nil::actor;
using namespace net;

static sstring to_sstring(const packet &p) {
    sstring res = uninitialized_string(p.len());
    auto i = res.begin();
    for (auto &frag : p.fragments()) {
        i = std::copy(frag.base, frag.base + frag.size, i);
    }
    return res;
}

struct stream_maker {
    bool _trim = false;
    size_t _size;

    stream_maker size(size_t size) && {
        _size = size;
        return std::move(*this);
    }

    stream_maker trim(bool do_trim) && {
        _trim = do_trim;
        return std::move(*this);
    }

    lw_shared_ptr<output_stream<char>> operator()(data_sink sink) {
        return make_lw_shared<output_stream<char>>(std::move(sink), _size, _trim);
    }
};

template<typename T, typename StreamConstructor>
future<> assert_split(StreamConstructor stream_maker, std::initializer_list<T> write_calls,
                      std::vector<std::string> expected_split) {
    static int i = 0;
    BOOST_TEST_MESSAGE("checking split: " << i++);
    auto sh_write_calls = make_lw_shared<std::vector<T>>(std::move(write_calls));
    auto sh_expected_splits = make_lw_shared<std::vector<std::string>>(std::move(expected_split));
    auto v = make_shared<std::vector<packet>>();
    auto out = stream_maker(data_sink(std::make_unique<vector_data_sink>(*v)));

    return do_for_each(sh_write_calls->begin(), sh_write_calls->end(),
                       [out, sh_write_calls](auto &&chunk) { return out->write(chunk); })
        .then([out, v, sh_expected_splits] {
            return out->close().then([out, v, sh_expected_splits] {
                BOOST_REQUIRE_EQUAL(v->size(), sh_expected_splits->size());
                int i = 0;
                for (auto &&chunk : *sh_expected_splits) {
                    BOOST_REQUIRE(to_sstring((*v)[i]) == chunk);
                    i++;
                }
            });
        });
}

ACTOR_TEST_CASE(test_splitting) {
    auto ctor = stream_maker().trim(false).size(4);
    return now()
        .then([=] { return assert_split(ctor, {"1"}, {"1"}); })
        .then([=] {
            return assert_split(ctor, {"12", "3"}, {"123"});
        })
        .then([=] {
            return assert_split(ctor, {"12", "34"}, {"1234"});
        })
        .then([=] {
            return assert_split(ctor, {"12", "345"}, {"1234", "5"});
        })
        .then([=] { return assert_split(ctor, {"1234"}, {"1234"}); })
        .then([=] { return assert_split(ctor, {"12345"}, {"12345"}); })
        .then([=] { return assert_split(ctor, {"1234567890"}, {"1234567890"}); })
        .then([=] {
            return assert_split(ctor, {"1", "23456"}, {"1234", "56"});
        })
        .then([=] {
            return assert_split(ctor, {"123", "4567"}, {"1234", "567"});
        })
        .then([=] {
            return assert_split(ctor, {"123", "45678"}, {"1234", "5678"});
        })
        .then([=] {
            return assert_split(ctor, {"123", "4567890"}, {"1234", "567890"});
        })
        .then([=] {
            return assert_split(ctor, {"1234", "567"}, {"1234", "567"});
        })

        .then([] {
            return assert_split(stream_maker().trim(false).size(3), {"1", "234567", "89"}, {"123", "4567", "89"});
        })
        .then([] {
            return assert_split(stream_maker().trim(false).size(3), {"1", "2345", "67"}, {"123", "456", "7"});
        });
}

ACTOR_TEST_CASE(test_splitting_with_trimming) {
    auto ctor = stream_maker().trim(true).size(4);
    return now()
        .then([=] { return assert_split(ctor, {"1"}, {"1"}); })
        .then([=] {
            return assert_split(ctor, {"12", "3"}, {"123"});
        })
        .then([=] {
            return assert_split(ctor, {"12", "3456789"}, {"1234", "5678", "9"});
        })
        .then([=] {
            return assert_split(ctor, {"12", "3456789", "12"}, {"1234", "5678", "912"});
        })
        .then([=] {
            return assert_split(ctor, {"123456789"}, {"1234", "5678", "9"});
        })
        .then([=] {
            return assert_split(ctor, {"12345678"}, {"1234", "5678"});
        })
        .then([=] {
            return assert_split(ctor, {"12345678", "9"}, {"1234", "5678", "9"});
        })
        .then([=] {
            return assert_split(ctor, {"1234", "567890"}, {"1234", "5678", "90"});
        });
}

ACTOR_TEST_CASE(test_flush_on_empty_buffer_does_not_push_empty_packet_down_stream) {
    auto v = make_shared<std::vector<packet>>();
    auto out = make_shared<output_stream<char>>(data_sink(std::make_unique<vector_data_sink>(*v)), 8);

    return out->flush()
        .then([v, out] {
            BOOST_REQUIRE(v->empty());
            return out->close();
        })
        .finally([out] {});
}

ACTOR_THREAD_TEST_CASE(test_simple_write) {
    auto vec = std::vector<net::packet> {};
    auto out = output_stream<char>(data_sink(std::make_unique<vector_data_sink>(vec)), 8);

    auto value1 = sstring("te");
    out.write(value1).get();

    auto value2 = sstring("st");
    out.write(value2).get();

    auto value3 = sstring("abcdefgh1234");
    out.write(value3).get();

    out.close().get();

    auto value = value1 + value2 + value3;
    auto packets = net::packet {};
    for (auto &p : vec) {
        packets.append(std::move(p));
    }
    packets.linearize();
    auto buf = packets.release();
    BOOST_REQUIRE_EQUAL(buf.size(), 1);
    BOOST_REQUIRE_EQUAL(sstring(buf.front().get(), buf.front().size()), value);
}
