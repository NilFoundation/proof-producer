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

#include <nil/actor/network/tcp.hh>
#include <nil/actor/network/tcp-stack.hh>
#include <nil/actor/network/ip.hh>
#include <nil/actor/core/align.hh>
#include <nil/actor/core/future.hh>
#include <nil/actor/network/detail/native-stack-impl.hh>

namespace nil {
    namespace actor {
        namespace net {
            void tcp_option::parse(uint8_t *beg1, uint8_t *end1) {
                const char *beg = reinterpret_cast<const char *>(beg1);
                const char *end = reinterpret_cast<const char *>(end1);
                while (beg < end) {
                    auto kind = option_kind(*beg);
                    if (kind != option_kind::nop && kind != option_kind::eol) {
                        // Make sure there is enough room for this option
                        auto len = uint8_t(beg[1]);
                        if (beg + len > end) {
                            return;
                        }
                    }
                    switch (kind) {
                        case option_kind::mss:
                            _mss_received = true;
                            _remote_mss = mss::read(beg).mss;
                            beg += option_len::mss;
                            break;
                        case option_kind::win_scale:
                            _win_scale_received = true;
                            _remote_win_scale = win_scale::read(beg).shift;
                            // We can turn on win_scale option, 7 is Linux's default win scale size
                            _local_win_scale = 7;
                            beg += option_len::win_scale;
                            break;
                        case option_kind::sack:
                            _sack_received = true;
                            beg += option_len::sack;
                            break;
                        case option_kind::nop:
                            beg += option_len::nop;
                            break;
                        case option_kind::eol:
                            return;
                        default:
                            // Ignore options we do not understand
                            uint8_t len = *(beg + 1);
                            beg += len;
                            // Prevent infinite loop
                            if (len == 0) {
                                return;
                            }
                            break;
                    }
                }
            }

            uint8_t tcp_option::fill(void *h, const tcp_hdr *th, uint8_t options_size) {
                auto hdr = reinterpret_cast<char *>(h);
                auto off = hdr + tcp_hdr::len;
                uint8_t size = 0;
                bool syn_on = th->f_syn;
                bool ack_on = th->f_ack;

                if (syn_on) {
                    if (_mss_received || !ack_on) {
                        auto mss = tcp_option::mss();
                        mss.mss = _local_mss;
                        mss.write(off);
                        off += mss.len;
                        size += mss.len;
                    }
                    if (_win_scale_received || !ack_on) {
                        auto win_scale = tcp_option::win_scale();
                        win_scale.shift = _local_win_scale;
                        win_scale.write(off);
                        off += win_scale.len;
                        size += win_scale.len;
                    }
                }
                if (size > 0) {
                    // Insert NOP option
                    auto size_max = align_up(uint8_t(size + 1), tcp_option::align);
                    while (size < size_max - uint8_t(option_len::eol)) {
                        auto nop = tcp_option::nop();
                        nop.write(off);
                        off += option_len::nop;
                        size += option_len::nop;
                    }
                    auto eol = tcp_option::eol();
                    eol.write(off);
                    size += option_len::eol;
                }
                assert(size == options_size);

                return size;
            }

            uint8_t tcp_option::get_size(bool syn_on, bool ack_on) {
                uint8_t size = 0;
                if (syn_on) {
                    if (_mss_received || !ack_on) {
                        size += option_len::mss;
                    }
                    if (_win_scale_received || !ack_on) {
                        size += option_len::win_scale;
                    }
                }
                if (size > 0) {
                    size += option_len::eol;
                    // Insert NOP option to align on 32-bit
                    size = align_up(size, tcp_option::align);
                }
                return size;
            }

            ipv4_tcp::ipv4_tcp(ipv4 &inet) : _inet_l4(inet), _tcp(std::make_unique<tcp<ipv4_traits>>(_inet_l4)) {
            }

            ipv4_tcp::~ipv4_tcp() {
            }

            void ipv4_tcp::received(packet p, ipv4_address from, ipv4_address to) {
                _tcp->received(std::move(p), from, to);
            }

            bool ipv4_tcp::forward(forward_hash &out_hash_data, packet &p, size_t off) {

                return _tcp->forward(out_hash_data, p, off);
            }

            server_socket tcpv4_listen(tcp<ipv4_traits> &tcpv4, uint16_t port, listen_options opts) {
                return server_socket(std::make_unique<native_server_socket_impl<tcp<ipv4_traits>>>(tcpv4, port, opts));
            }

            ::nil::actor::socket tcpv4_socket(tcp<ipv4_traits> &tcpv4) {
                return ::nil::actor::socket(std::make_unique<native_socket_impl<tcp<ipv4_traits>>>(tcpv4));
            }
        }    // namespace net
    }    // namespace actor
}    // namespace nil
