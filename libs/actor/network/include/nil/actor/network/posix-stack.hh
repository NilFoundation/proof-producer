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

#include <nil/actor/core/sharded.hh>
#include <nil/actor/core/detail/pollable_fd.hh>
#include <nil/actor/network/stack.hh>
#include <nil/actor/core/polymorphic_temporary_buffer.hh>
#include <nil/actor/core/detail/buffer_allocator.hh>

#include <boost/program_options.hpp>

namespace nil {
    namespace actor {

        namespace net {

            using namespace nil::actor;

            // We can't keep this in any of the socket servers as instance members, because a connection can
            // outlive the socket server. To avoid having the whole socket_server tracked as a shared pointer,
            // we will have a conntrack structure.
            //
            // Right now this class is used by the posix_server_socket_impl, but it could be used by any other.
            class conntrack {
                class load_balancer {
                    std::vector<unsigned> _cpu_load;

                public:
                    load_balancer() : _cpu_load(size_t(smp::count), 0) {
                    }
                    void closed_cpu(shard_id cpu) {
                        _cpu_load[cpu]--;
                    }
                    shard_id next_cpu() {
                        // FIXME: The naive algorithm will just round robin the connections around the shards.
                        // A more complex version can keep track of the amount of activity in each connection,
                        // and use that information.
                        auto min_el = std::min_element(_cpu_load.begin(), _cpu_load.end());
                        auto cpu = shard_id(std::distance(_cpu_load.begin(), min_el));
                        _cpu_load[cpu]++;
                        return cpu;
                    }
                    shard_id force_cpu(shard_id cpu) {
                        _cpu_load[cpu]++;
                        return cpu;
                    }
                };

                lw_shared_ptr<load_balancer> _lb;
                void closed_cpu(shard_id cpu) {
                    _lb->closed_cpu(cpu);
                }

            public:
                class handle {
                    shard_id _host_cpu;
                    shard_id _target_cpu;
                    foreign_ptr<lw_shared_ptr<load_balancer>> _lb;

                public:
                    handle() : _lb(nullptr) {
                    }
                    handle(shard_id cpu, lw_shared_ptr<load_balancer> lb) :
                        _host_cpu(this_shard_id()), _target_cpu(cpu), _lb(make_foreign(std::move(lb))) {
                    }

                    handle(const handle &) = delete;
                    handle(handle &&) = default;
                    ~handle() {
                        if (!_lb) {
                            return;
                        }
                        // FIXME: future is discarded
                        (void)smp::submit_to(_host_cpu,
                                             [cpu = _target_cpu, lb = std::move(_lb)] { lb->closed_cpu(cpu); });
                    }
                    shard_id cpu() {
                        return _target_cpu;
                    }
                };
                friend class handle;

                conntrack() : _lb(make_lw_shared<load_balancer>()) {
                }
                handle get_handle() {
                    return handle(_lb->next_cpu(), _lb);
                }
                handle get_handle(shard_id cpu) {
                    return handle(_lb->force_cpu(cpu), _lb);
                }
            };

            class posix_data_source_impl final : public data_source_impl, private detail::buffer_allocator {
                boost::container::pmr::polymorphic_allocator<char> *_buffer_allocator;
                pollable_fd _fd;
                connected_socket_input_stream_config _config;

            private:
                virtual temporary_buffer<char> allocate_buffer() override;

            public:
                explicit posix_data_source_impl(
                    pollable_fd fd, connected_socket_input_stream_config config,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) :
                    _buffer_allocator(allocator),
                    _fd(std::move(fd)), _config(config) {
                }
                future<temporary_buffer<char>> get() override;
                future<> close() override;
            };

            class posix_data_sink_impl : public data_sink_impl {
                pollable_fd _fd;
                packet _p;

            public:
                explicit posix_data_sink_impl(pollable_fd fd) : _fd(std::move(fd)) {
                }
                using data_sink_impl::put;
                future<> put(packet p) override;
                future<> put(temporary_buffer<char> buf) override;
                future<> close() override;
            };

            class posix_ap_server_socket_impl : public server_socket_impl {
                using protocol_and_socket_address = std::tuple<int, socket_address>;
                struct connection {
                    pollable_fd fd;
                    socket_address addr;
                    conntrack::handle connection_tracking_handle;
                    connection(pollable_fd xfd, socket_address xaddr, conntrack::handle cth) :
                        fd(std::move(xfd)), addr(xaddr), connection_tracking_handle(std::move(cth)) {
                    }
                };
                using sockets_map_t = std::unordered_map<protocol_and_socket_address, promise<accept_result>>;
                using conn_map_t = std::unordered_multimap<protocol_and_socket_address, connection>;
                static thread_local sockets_map_t sockets;
                static thread_local conn_map_t conn_q;
                int _protocol;
                socket_address _sa;
                boost::container::pmr::polymorphic_allocator<char> *_allocator;

            public:
                explicit posix_ap_server_socket_impl(
                    int protocol, socket_address sa,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) :
                    _protocol(protocol),
                    _sa(sa), _allocator(allocator) {
                }
                virtual future<accept_result> accept() override;
                virtual void abort_accept() override;
                socket_address local_address() const override {
                    return _sa;
                }
                static void move_connected_socket(int protocol, socket_address sa, pollable_fd fd, socket_address addr,
                                                  conntrack::handle handle,
                                                  boost::container::pmr::polymorphic_allocator<char> *allocator);

                template<typename T>
                friend class std::hash;
            };

            class posix_server_socket_impl : public server_socket_impl {
                socket_address _sa;
                int _protocol;
                pollable_fd _lfd;
                conntrack _conntrack;
                server_socket::load_balancing_algorithm _lba;
                shard_id _fixed_cpu;
                boost::container::pmr::polymorphic_allocator<char> *_allocator;

            public:
                explicit posix_server_socket_impl(
                    int protocol, socket_address sa, pollable_fd lfd, server_socket::load_balancing_algorithm lba,
                    shard_id fixed_cpu,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) :
                    _sa(sa),
                    _protocol(protocol), _lfd(std::move(lfd)), _lba(lba), _fixed_cpu(fixed_cpu), _allocator(allocator) {
                }
                virtual future<accept_result> accept() override;
                virtual void abort_accept() override;
                virtual socket_address local_address() const override;
            };

            class posix_reuseport_server_socket_impl : public server_socket_impl {
                socket_address _sa;
                int _protocol;
                pollable_fd _lfd;
                boost::container::pmr::polymorphic_allocator<char> *_allocator;

            public:
                explicit posix_reuseport_server_socket_impl(
                    int protocol, socket_address sa, pollable_fd lfd,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) :
                    _sa(sa),
                    _protocol(protocol), _lfd(std::move(lfd)), _allocator(allocator) {
                }
                virtual future<accept_result> accept() override;
                virtual void abort_accept() override;
                virtual socket_address local_address() const override;
            };

            class posix_network_stack : public network_stack {
            private:
                const bool _reuseport;

            protected:
                boost::container::pmr::polymorphic_allocator<char> *_allocator;

            public:
                explicit posix_network_stack(
                    boost::program_options::variables_map opts,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator);
                virtual server_socket listen(socket_address sa, listen_options opts) override;
                virtual ::nil::actor::socket socket() override;
                virtual net::udp_channel make_udp_channel(const socket_address &) override;
                static future<std::unique_ptr<network_stack>>
                    create(boost::program_options::variables_map opts,
                           boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) {
                    return make_ready_future<std::unique_ptr<network_stack>>(
                        std::unique_ptr<network_stack>(new posix_network_stack(opts, allocator)));
                }
                virtual bool has_per_core_namespace() override {
                    return _reuseport;
                };
                bool supports_ipv6() const override;
                std::vector<network_interface> network_interfaces() override;
            };

            class posix_ap_network_stack : public posix_network_stack {
            private:
                const bool _reuseport;

            public:
                posix_ap_network_stack(
                    boost::program_options::variables_map opts,
                    boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator);
                virtual server_socket listen(socket_address sa, listen_options opts) override;
                static future<std::unique_ptr<network_stack>>
                    create(boost::program_options::variables_map opts,
                           boost::container::pmr::polymorphic_allocator<char> *allocator = memory::malloc_allocator) {
                    return make_ready_future<std::unique_ptr<network_stack>>(
                        std::unique_ptr<network_stack>(new posix_ap_network_stack(opts, allocator)));
                }
            };

            void register_posix_stack();
        }    // namespace net

    }    // namespace actor
}    // namespace nil
