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

#include <nil/actor/core/reactor.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/test_runner.hh>
#include <nil/actor/network/ip.hh>

using namespace nil::actor;
using namespace net;

ACTOR_TEST_CASE(test_connection_attempt_is_shutdown) {
    ipv4_addr server_addr("172.16.0.1");
    auto unconn = make_socket();
    auto f = unconn.connect(make_ipv4_address(server_addr)).then_wrapped([](auto &&f) {
        try {
            f.get();
            BOOST_REQUIRE(false);
        } catch (...) {
        }
    });
    unconn.shutdown();
    return f.finally([unconn = std::move(unconn)] {});
}

ACTOR_TEST_CASE(test_unconnected_socket_shutsdown_established_connection) {
    // Use a random port to reduce chance of conflict.
    // TODO: retry a few times on failure.
    std::default_random_engine &rnd = testing::local_random_engine;
    auto distr = std::uniform_int_distribution<uint16_t>(12000, 65000);
    auto sa = make_ipv4_address({"127.0.0.1", distr(rnd)});
    return do_with(engine().net().listen(sa, listen_options()), [sa](auto &listener) {
        auto f = listener.accept();
        auto unconn = make_socket();
        auto connf = unconn.connect(sa);
        return connf
            .then([unconn = std::move(unconn)](auto &&conn) mutable {
                unconn.shutdown();
                return do_with(std::move(conn), [](auto &conn) {
                    return do_with(conn.output(1), [](auto &out) {
                        return out.write("ping").then_wrapped([](auto &&f) {
                            try {
                                f.get();
                                BOOST_REQUIRE(false);
                            } catch (...) {
                            }
                        });
                    });
                });
            })
            .finally([f = std::move(f)]() mutable { return std::move(f); });
    });
}

ACTOR_TEST_CASE(test_accept_after_abort) {
    std::default_random_engine &rnd = testing::local_random_engine;
    auto distr = std::uniform_int_distribution<uint16_t>(12000, 65000);
    auto sa = make_ipv4_address({"127.0.0.1", distr(rnd)});
    return do_with(nil::actor::server_socket(engine().net().listen(sa, listen_options())), [](auto &listener) {
        using ftype = future<accept_result>;
        promise<ftype> p;
        future<ftype> done = p.get_future();
        auto f = listener.accept().then_wrapped([&listener, p = std::move(p)](auto f) mutable {
            f.ignore_ready_future();
            p.set_value(listener.accept());
        });
        listener.abort_accept();
        return done.then([](ftype f) {
            return f.then_wrapped([](ftype f) {
                BOOST_REQUIRE(f.failed());
                if (f.available()) {
                    f.ignore_ready_future();
                }
            });
        });
    });
}
