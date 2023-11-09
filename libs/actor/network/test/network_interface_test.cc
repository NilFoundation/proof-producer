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
#include <nil/actor/network/ethernet.hh>
#include <nil/actor/network/ip.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/detail/log.hh>

using namespace nil::actor;

static logger niflog("network_interface_test");

ACTOR_TEST_CASE(list_interfaces) {
    // just verifying we have something. And can access all the stuff.
    auto interfaces = engine().net().network_interfaces();
    BOOST_REQUIRE_GT(interfaces.size(), 0);

    for (auto &nif : interfaces) {
        niflog.info("Iface: {}, index = {}, mtu = {}, loopback = {}, virtual = {}, up = {}", nif.name(), nif.index(),
                    nif.mtu(), nif.is_loopback(), nif.is_virtual(), nif.is_up());
        if (nif.hardware_address().size() >= 6) {
            niflog.info("   HW: {}", net::ethernet_address(nif.hardware_address().data()));
        }
        for (auto &addr : nif.addresses()) {
            niflog.info("   Addr: {}", addr);
        }
    }

    return make_ready_future();
}

ACTOR_TEST_CASE(match_ipv6_scope) {
    auto interfaces = engine().net().network_interfaces();

    for (auto &nif : interfaces) {
        if (nif.is_loopback()) {
            continue;
        }
        auto i = std::find_if(nif.addresses().begin(), nif.addresses().end(), std::mem_fn(&net::inet_address::is_ipv6));
        if (i == nif.addresses().end()) {
            continue;
        }

        std::ostringstream ss;
        ss << net::inet_address(i->as_ipv6_address()) << "%" << nif.name();
        auto text = ss.str();

        net::inet_address na(text);

        BOOST_REQUIRE_EQUAL(na.as_ipv6_address(), i->as_ipv6_address());
        // also verify that the inet_address itself matches
        BOOST_REQUIRE_EQUAL(na, *i);
        // and that inet_address _without_ scope matches.
        BOOST_REQUIRE_EQUAL(net::inet_address(na.as_ipv6_address()), *i);
        BOOST_REQUIRE_EQUAL(na.scope(), nif.index());
        // and that they are not ipv4 addresses
        BOOST_REQUIRE_THROW(i->as_ipv4_address(), std::invalid_argument);
        BOOST_REQUIRE_THROW(na.as_ipv4_address(), std::invalid_argument);

        niflog.info("Org: {}, Parsed: {}, Text: {}", *i, na, text);
    }

    return make_ready_future();
}
