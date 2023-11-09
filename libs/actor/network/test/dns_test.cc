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

#include <vector>
#include <algorithm>

#include <nil/actor/core/do_with.hh>
#include <nil/actor/testing/test_case.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/network/dns.hh>
#include <nil/actor/network/inet_address.hh>

using namespace nil::actor;
using namespace nil::actor::net;

static const sstring actor_name = "actor.io";

static future<> test_resolve(dns_resolver::options opts) {
    auto d = ::make_lw_shared<dns_resolver>(std::move(opts));
    return d->get_host_by_name(actor_name, inet_address::family::INET)
        .then([d](hostent e) {
            return d->get_host_by_addr(e.addr_list.front()).then([d, a = e.addr_list.front()](hostent e) {
                return d->get_host_by_name(e.names.front(), inet_address::family::INET).then([a](hostent e) {
                    BOOST_REQUIRE(std::count(e.addr_list.begin(), e.addr_list.end(), a));
                });
            });
        })
        .finally([d] { return d->close(); });
}

static future<> test_bad_name(dns_resolver::options opts) {
    auto d = ::make_lw_shared<dns_resolver>(std::move(opts));
    return d->get_host_by_name("apa.ninja.gnu", inet_address::family::INET)
        .then_wrapped([d](future<hostent> f) {
            try {
                f.get();
                BOOST_FAIL("should not succeed");
            } catch (...) {
                // ok.
            }
        })
        .finally([d] { return d->close(); });
}

ACTOR_TEST_CASE(test_resolve_udp) {
    return test_resolve(dns_resolver::options());
}

ACTOR_TEST_CASE(test_bad_name_udp) {
    return test_bad_name(dns_resolver::options());
}

ACTOR_TEST_CASE(test_timeout_udp) {
    dns_resolver::options opts;
    opts.servers = std::vector<inet_address>({inet_address("1.2.3.4")});    // not a server
    opts.udp_port = 29953;                                                  // not a dns port
    opts.timeout = std::chrono::milliseconds(500);

    auto d = ::make_lw_shared<dns_resolver>(engine().net(), opts);
    return d->get_host_by_name(actor_name, inet_address::family::INET)
        .then_wrapped([d](future<hostent> f) {
            try {
                f.get();
                BOOST_FAIL("should not succeed");
            } catch (...) {
                // ok.
            }
        })
        .finally([d] { return d->close(); });
}

// Currently failing, disable until fixed (#521)
#if 0
ACTOR_TEST_CASE(test_resolve_tcp) {
    dns_resolver::options opts;
    opts.use_tcp_query = true;
    return test_resolve(opts);
}
#endif

ACTOR_TEST_CASE(test_bad_name_tcp) {
    dns_resolver::options opts;
    opts.use_tcp_query = true;
    return test_bad_name(opts);
}

static const sstring imaps_service = "imaps";
static const sstring gmail_domain = "gmail.com";

static future<> test_srv() {
    auto d = ::make_lw_shared<dns_resolver>();
    return d->get_srv_records(dns_resolver::srv_proto::tcp, imaps_service, gmail_domain)
        .then([d](dns_resolver::srv_records records) {
            BOOST_REQUIRE(!records.empty());
            for (auto &record : records) {
                // record.target should end with "gmail.com"
                BOOST_REQUIRE_GT(record.target.size(), gmail_domain.size());
                BOOST_REQUIRE_EQUAL(record.target.compare(record.target.size() - gmail_domain.size(),
                                                          gmail_domain.size(), gmail_domain),
                                    0);
            }
        })
        .finally([d] { return d->close(); });
}

ACTOR_TEST_CASE(test_srv_tcp) {
    return test_srv();
}
