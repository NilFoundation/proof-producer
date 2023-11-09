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

#include <istream>
#include <string>
#include <unordered_map>

#include <boost/optional.hpp>

namespace nil {
    namespace actor {
        namespace net {

            struct ipv4_config {
                std::string ip;
                std::string netmask;
                std::string gateway;
                bool dhcp {false};
            };

            struct hw_config {
                std::string pci_address;
                boost::optional<unsigned> port_index;
                bool lro {true};
                bool tso {true};
                bool ufo {true};
                bool hw_fc {true};
                bool event_index {true};
                bool csum_offload {true};
                boost::optional<unsigned> ring_size;
            };

            struct device_config {
                ipv4_config ip_cfg;
                hw_config hw_cfg;
            };

            std::unordered_map<std::string, device_config> parse_config(std::istream &input);

            class config_exception : public std::runtime_error {
            public:
                config_exception(const std::string &msg) : std::runtime_error(msg) {
                }
            };
        }    // namespace net
    }        // namespace actor
}    // namespace nil
