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

#define BOOST_TEST_MODULE core

#include <nil/actor/network/config.hh>
#include <boost/test/included/unit_test.hpp>
#include <exception>
#include <sstream>

using namespace nil::actor::net;

BOOST_AUTO_TEST_CASE(test_valid_config_with_pci_address) {
    std::stringstream ss;
    ss << "{eth0: {pci-address: 0000:06:00.0, ip: 192.168.100.10, gateway: 192.168.100.1, netmask: "
          "255.255.255.0 } , eth1: {pci-address: 0000:06:00.1, dhcp: true } }";
    auto device_configs = parse_config(ss);

    // eth0 tests
    BOOST_REQUIRE(device_configs.find("eth0") != device_configs.end());
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").hw_cfg.pci_address, "0000:06:00.0");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.dhcp, false);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.ip, "192.168.100.10");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.gateway, "192.168.100.1");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.netmask, "255.255.255.0");

    // eth1 tests
    BOOST_REQUIRE(device_configs.find("eth1") != device_configs.end());
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").hw_cfg.pci_address, "0000:06:00.1");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.dhcp, true);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.ip, "");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.gateway, "");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.netmask, "");
}

BOOST_AUTO_TEST_CASE(test_valid_config_with_port_index) {
    std::stringstream ss;
    ss << "{eth0: {port-index: 0, ip: 192.168.100.10, gateway: 192.168.100.1, netmask: "
          "255.255.255.0 } , eth1: {port-index: 1, dhcp: true } }";
    auto device_configs = parse_config(ss);

    // eth0 tests
    BOOST_REQUIRE(device_configs.find("eth0") != device_configs.end());
    BOOST_REQUIRE_EQUAL(*device_configs.at("eth0").hw_cfg.port_index, 0u);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.dhcp, false);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.ip, "192.168.100.10");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.gateway, "192.168.100.1");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.netmask, "255.255.255.0");

    // eth1 tests
    BOOST_REQUIRE(device_configs.find("eth1") != device_configs.end());
    BOOST_REQUIRE_EQUAL(*device_configs.at("eth1").hw_cfg.port_index, 1u);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.dhcp, true);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.ip, "");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.gateway, "");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth1").ip_cfg.netmask, "");
}

BOOST_AUTO_TEST_CASE(test_valid_config_single_device) {
    std::stringstream ss;
    ss << "eth0: {pci-address: 0000:06:00.0, ip: 192.168.100.10, gateway: 192.168.100.1, netmask: "
          "255.255.255.0 }";
    auto device_configs = parse_config(ss);

    // eth0 tests
    BOOST_REQUIRE(device_configs.find("eth0") != device_configs.end());
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").hw_cfg.pci_address, "0000:06:00.0");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.dhcp, false);
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.ip, "192.168.100.10");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.gateway, "192.168.100.1");
    BOOST_REQUIRE_EQUAL(device_configs.at("eth0").ip_cfg.netmask, "255.255.255.0");
}

BOOST_AUTO_TEST_CASE(test_unsupported_key) {
    std::stringstream ss;
    ss << "{eth0: { some_not_supported_tag: xxx, pci-address: 0000:06:00.0, ip: 192.168.100.10, "
          "gateway: 192.168.100.1, netmask: 255.255.255.0 } , eth1: {pci-address: 0000:06:00.1, "
          "dhcp: true } }";

    BOOST_REQUIRE_THROW(parse_config(ss), config_exception);
}

BOOST_AUTO_TEST_CASE(test_bad_yaml_syntax_if_thrown) {
    std::stringstream ss;
    ss << "some bad: [ yaml syntax }";
    BOOST_REQUIRE_THROW(parse_config(ss), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_pci_address_and_port_index_if_thrown) {
    std::stringstream ss;
    ss << "{eth0: {pci-address: 0000:06:00.0, port-index: 0, ip: 192.168.100.10, gateway: "
          "192.168.100.1, netmask: 255.255.255.0 } , eth1: {pci-address: 0000:06:00.1, dhcp: true} "
          "}";
    BOOST_REQUIRE_THROW(parse_config(ss), config_exception);
}

BOOST_AUTO_TEST_CASE(test_dhcp_and_ip_if_thrown) {
    std::stringstream ss;
    ss << "{eth0: {pci-address: 0000:06:00.0, ip: 192.168.100.10, gateway: 192.168.100.1, netmask: "
          "255.255.255.0, dhcp: true } , eth1: {pci-address: 0000:06:00.1, dhcp: true} }";
    BOOST_REQUIRE_THROW(parse_config(ss), config_exception);
}

BOOST_AUTO_TEST_CASE(test_ip_missing_if_thrown) {
    std::stringstream ss;
    ss << "{eth0: {pci-address: 0000:06:00.0, gateway: 192.168.100.1, netmask: 255.255.255.0 } , "
          "eth1: {pci-address: 0000:06:00.1, dhcp: true} }";
    BOOST_REQUIRE_THROW(parse_config(ss), config_exception);
}
