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

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/network/api.hh>
#include <nil/actor/network/inet_address.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/detail/log.hh>

using namespace nil::actor;

static logger iplog("ipv6");

static bool check_ipv6_support() {
    if (!engine().net().supports_ipv6()) {
        iplog.info("No IPV6 support detected. Skipping...");
        return false;
    }
    return true;
}

ACTOR_TEST_CASE(udp_packet_test) {
    if (!check_ipv6_support()) {
        return make_ready_future<>();
    }

    auto sc = make_udp_channel(ipv6_addr {"::1"});

    BOOST_REQUIRE(sc.local_address().addr().is_ipv6());

    auto cc = make_udp_channel(ipv6_addr {"::1"});

    auto f1 = cc.send(sc.local_address(), "apa");

    return f1.then([cc = std::move(cc), sc = std::move(sc)]() mutable {
        auto src = cc.local_address();
        cc.close();
        auto f2 = sc.receive();

        return f2.then([sc = std::move(sc), src](auto pkt) mutable {
            auto a = sc.local_address();
            sc.close();
            BOOST_REQUIRE_EQUAL(src, pkt.get_src());
            auto dst = pkt.get_dst();
            // Don't always get a dst address.
            if (dst != socket_address()) {
                BOOST_REQUIRE_EQUAL(a, pkt.get_dst());
            }
        });
    });
}

ACTOR_TEST_CASE(tcp_packet_test) {
    if (!check_ipv6_support()) {
        return make_ready_future<>();
    }

    return async([] {
        auto sc = server_socket(engine().net().listen(ipv6_addr {"::1"}, {}));
        auto la = sc.local_address();

        BOOST_REQUIRE(la.addr().is_ipv6());

        auto cc = connect(la).get0();
        auto lc = std::move(sc.accept().get0().connection);

        auto strm = cc.output();
        strm.write("los lobos").get();
        strm.flush().get();

        auto in = lc.input();

        using consumption_result_type = typename input_stream<char>::consumption_result_type;
        using stop_consuming_type = typename consumption_result_type::stop_consuming_type;
        using tmp_buf = stop_consuming_type::tmp_buf;

        in.consume([](tmp_buf buf) {
              return make_ready_future<consumption_result_type>(stop_consuming<char>({}));
          }).get();

        strm.close().get();
        in.close().get();
        sc.abort_accept();
    });
}

