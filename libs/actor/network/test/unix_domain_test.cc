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
#include <nil/actor/core/core.hh>
#include <nil/actor/network/api.hh>
#include <nil/actor/network/inet_address.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/detail/log.hh>
#include <nil/actor/detail/std-compat.hh>

using namespace nil::actor;
using std::string;
using namespace std::string_literals;
using namespace std::chrono_literals;

static logger iplog("unix_domain");

class ud_server_client {
public:
    ud_server_client(string server_path, boost::optional<string> client_path, int rounds) :
        ud_server_client(server_path, client_path, rounds, 0) {};

    ud_server_client(string server_path, boost::optional<string> client_path, int rounds, int abort_run) :
        server_addr {unix_domain_addr {server_path}}, client_path {client_path}, rounds {rounds}, rounds_left {rounds},
        abort_after {abort_run} {
    }

    future<> run();
    ud_server_client(ud_server_client &&) = default;
    ud_server_client(const ud_server_client &) = delete;

private:
    const string test_message {"are you still the same?"s};
    future<> init_server();
    future<> client_round();
    const socket_address server_addr;

    const boost::optional<string> client_path;
    server_socket server;
    const int rounds;
    int rounds_left;
    server_socket *lstn_sock;
    nil::actor::thread th;
    int abort_after;               // if set - force the listening socket down after that number of rounds
    bool planned_abort {false};    // set when abort_accept() is called
};

future<> ud_server_client::init_server() {
    return do_with(nil::actor::listen(server_addr), [this](server_socket &lstn) mutable {
        lstn_sock = &lstn;    // required when aborting (on some tests)

        //  start the clients here, where we know the server is listening

        th = nil::actor::thread([this] {
            for (int i = 0; i < rounds; ++i) {
                if (abort_after) {
                    if (--abort_after == 0) {
                        planned_abort = true;
                        lstn_sock->abort_accept();
                        break;
                    }
                }
                (void)client_round().get0();
            }
        });

        return do_until([this]() { return rounds_left <= 0; },
                        [&lstn, this]() {
                            return lstn.accept().then([this](accept_result from_accept) {
                                connected_socket cn = std::move(from_accept.connection);
                                socket_address cn_addr = std::move(from_accept.remote_address);
                                --rounds_left;
                                //  verify the client address
                                if (client_path) {
                                    socket_address tmmp {unix_domain_addr {*client_path}};
                                    BOOST_REQUIRE_EQUAL(cn_addr, socket_address {unix_domain_addr {*client_path}});
                                }

                                return do_with(cn.input(), cn.output(),
                                               [](auto &inp, auto &out) {
                                                   return inp.read()
                                                       .then([&out](auto bb) {
                                                           string ans = "-"s;
                                                           if (bb && bb.size()) {
                                                               ans = "+"s + string {bb.get(), bb.size()};
                                                           }
                                                           return out.write(ans)
                                                               .then([&out]() { return out.flush(); })
                                                               .then([&out]() { return out.close(); });
                                                       })
                                                       .then([&inp]() { return inp.close(); })
                                                       .then([]() { return make_ready_future<>(); });
                                               })
                                    .then([] { return make_ready_future<>(); });
                            });
                        })
            .handle_exception([this](auto e) {
                // OK to get here only if the test was a "planned abort" one
                if (!planned_abort) {
                    std::rethrow_exception(e);
                }
            })
            .finally([this] { return th.join(); });
    });
}

/// Send a message to the server, and expect (almost) the same string back.
/// If 'client_path' is set, the client binds to the named path.
future<> ud_server_client::client_round() {
    auto cc = client_path ?
                  engine().net().connect(server_addr, socket_address {unix_domain_addr {*client_path}}).get0() :
                  engine().net().connect(server_addr).get0();

    return do_with(cc.input(), cc.output(), [this](auto &inp, auto &out) {
        return out.write(test_message)
            .then([&out]() { return out.flush(); })
            .then([&inp]() { return inp.read(); })
            .then([this, &inp](auto bb) {
                BOOST_REQUIRE_EQUAL(std::string_view(bb.begin(), bb.size()), "+"s + test_message);
                return inp.close();
            })
            .then([&out]() { return out.close(); })
            .then([] { return make_ready_future<>(); });
    });
}

future<> ud_server_client::run() {
    return async([this] {
        auto serverfut = init_server();
        (void)serverfut.get();
    });
}

//  testing the various address types, both on the server and on the
//  client side

ACTOR_TEST_CASE(unixdomain_server) {
    system("rm -f /tmp/ry");
    ud_server_client uds("/tmp/ry", boost::none, 3);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
    return make_ready_future<>();
}

ACTOR_TEST_CASE(unixdomain_abs) {
    char sv_name[] {'\0', '1', '1', '1'};
    // ud_server_client uds(string{"\0111",4}, string{"\0112",4}, 1);
    ud_server_client uds(string {sv_name, 4}, boost::none, 4);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
    // return make_ready_future<>();
}

ACTOR_TEST_CASE(unixdomain_abs_bind) {
    char sv_name[] {'\0', '1', '1', '1'};
    char cl_name[] {'\0', '1', '1', '2'};
    ud_server_client uds(string {sv_name, 4}, string {cl_name, 4}, 1);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
}

ACTOR_TEST_CASE(unixdomain_abs_bind_2) {
    char sv_name[] {'\0', '1', '\0', '\12', '1'};
    char cl_name[] {'\0', '1', '\0', '\12', '2'};
    ud_server_client uds(string {sv_name, 5}, string {cl_name, 5}, 2);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
}

ACTOR_TEST_CASE(unixdomain_text) {
    socket_address addr1 {unix_domain_addr {"abc"}};
    BOOST_REQUIRE_EQUAL(format("{}", addr1), "abc");
    socket_address addr2 {unix_domain_addr {""}};
    BOOST_REQUIRE_EQUAL(format("{}", addr2), "{unnamed}");
    socket_address addr3 {unix_domain_addr {std::string("\0abc", 5)}};
    BOOST_REQUIRE_EQUAL(format("{}", addr3), "@abc_");
    return make_ready_future<>();
}

ACTOR_TEST_CASE(unixdomain_bind) {
    system("rm -f 111 112");
    ud_server_client uds("111"s, "112"s, 1);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
}

ACTOR_TEST_CASE(unixdomain_short) {
    system("rm -f 3");
    ud_server_client uds("3"s, boost::none, 10);
    return do_with(std::move(uds), [](auto &uds) { return uds.run(); });
}

//  test our ability to abort the accept()'ing on a socket.
//  The test covers a specific bug in the handling of abort_accept()
ACTOR_TEST_CASE(unixdomain_abort) {
    std::string sockname {"7"s};    // note: no portable & warnings-free option
    std::ignore = ::unlink(sockname.c_str());
    ud_server_client uds(sockname, boost::none, 10, 4);
    return do_with(std::move(uds), [sockname](auto &uds) {
        return uds.run().finally([sockname]() {
            std::ignore = ::unlink(sockname.c_str());
            return nil::actor::make_ready_future<>();
        });
    });
}

