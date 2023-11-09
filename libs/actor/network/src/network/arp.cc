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

#include <nil/actor/network/arp.hh>

namespace nil {
    namespace actor {

        namespace net {

            arp_for_protocol::arp_for_protocol(arp &a, uint16_t proto_num) : _arp(a), _proto_num(proto_num) {
                _arp.add(proto_num, this);
            }

            arp_for_protocol::~arp_for_protocol() {
                _arp.del(_proto_num);
            }

            arp::arp(interface *netif) :
                _netif(netif), _proto(netif, eth_protocol_num::arp, [this] { return get_packet(); }) {
                // FIXME: ignored future
                (void)_proto.receive([this](packet p, ethernet_address ea) { return process_packet(std::move(p), ea); },
                                     [this](forward_hash &out_hash_data, packet &p, size_t off) {
                                         return forward(out_hash_data, p, off);
                                     });
            }

            boost::optional<l3_protocol::l3packet> arp::get_packet() {
                boost::optional<l3_protocol::l3packet> p;
                if (!_packetq.empty()) {
                    p = std::move(_packetq.front());
                    _packetq.pop_front();
                }
                return p;
            }

            bool arp::forward(forward_hash &out_hash_data, packet &p, size_t off) {
                auto ah = p.get_header<arp_hdr>(off);
                auto i = _arp_for_protocol.find(ntoh(ah->ptype));
                if (i != _arp_for_protocol.end()) {
                    return i->second->forward(out_hash_data, p, off);
                }
                return false;
            }

            void arp::add(uint16_t proto_num, arp_for_protocol *afp) {
                _arp_for_protocol[proto_num] = afp;
            }

            void arp::del(uint16_t proto_num) {
                _arp_for_protocol.erase(proto_num);
            }

            future<> arp::process_packet(packet p, ethernet_address from) {
                auto h = p.get_header(0, arp_hdr::size());
                if (!h) {
                    return make_ready_future<>();
                }
                auto ah = arp_hdr::read(h);
                auto i = _arp_for_protocol.find(ah.ptype);
                if (i != _arp_for_protocol.end()) {
                    return i->second->received(std::move(p));
                }
                return make_ready_future<>();
            }

        }    // namespace net

    }    // namespace actor
}    // namespace nil
