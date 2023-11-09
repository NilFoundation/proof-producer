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

#include <chrono>
#include <nil/actor/network/api.hh>
#include <nil/actor/core/memory.hh>
#include <nil/actor/core/detail/api-level.hh>

namespace nil {
    namespace actor {

        namespace net {

            /// \cond internal
            class connected_socket_impl {
            public:
                virtual ~connected_socket_impl() {
                }
                virtual data_source source() = 0;
                virtual data_source source(connected_socket_input_stream_config csisc);
                virtual data_sink sink() = 0;
                virtual void shutdown_input() = 0;
                virtual void shutdown_output() = 0;
                virtual void set_nodelay(bool nodelay) = 0;
                virtual bool get_nodelay() const = 0;
                virtual void set_keepalive(bool keepalive) = 0;
                virtual bool get_keepalive() const = 0;
                virtual void set_keepalive_parameters(const keepalive_params &) = 0;
                virtual keepalive_params get_keepalive_parameters() const = 0;
                virtual void set_sockopt(int level, int optname, const void *data, size_t len) = 0;
                virtual int get_sockopt(int level, int optname, void *data, size_t len) const = 0;
            };

            class socket_impl {
            public:
                virtual ~socket_impl() {
                }
                virtual future<connected_socket> connect(socket_address sa, socket_address local,
                                                         transport proto = transport::TCP) = 0;
                virtual void set_reuseaddr(bool reuseaddr) = 0;
                virtual bool get_reuseaddr() const = 0;
                virtual void shutdown() = 0;
            };

            class server_socket_impl {
            public:
                virtual ~server_socket_impl() {
                }
                virtual future<accept_result> accept() = 0;
                virtual void abort_accept() = 0;
                virtual socket_address local_address() const = 0;
            };

            class udp_channel_impl {
            public:
                virtual ~udp_channel_impl() {
                }
                virtual socket_address local_address() const = 0;
                virtual future<udp_datagram> receive() = 0;
                virtual future<> send(const socket_address &dst, const char *msg) = 0;
                virtual future<> send(const socket_address &dst, packet p) = 0;
                virtual void shutdown_input() = 0;
                virtual void shutdown_output() = 0;
                virtual bool is_closed() const = 0;
                virtual void close() = 0;
            };

            class network_interface_impl {
            public:
                virtual ~network_interface_impl() {
                }
                virtual uint32_t index() const = 0;
                virtual uint32_t mtu() const = 0;

                virtual const sstring &name() const = 0;
                virtual const sstring &display_name() const = 0;
                virtual const std::vector<net::inet_address> &addresses() const = 0;
                virtual const std::vector<uint8_t> hardware_address() const = 0;

                virtual bool is_loopback() const = 0;
                virtual bool is_virtual() const = 0;
                virtual bool is_up() const = 0;
                virtual bool supports_ipv6() const = 0;
            };

            /// \endcond
        }    // namespace net
    }        // namespace actor
}    // namespace nil
