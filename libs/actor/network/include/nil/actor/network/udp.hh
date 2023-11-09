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

#include <unordered_map>
#include <assert.h>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/network/api.hh>
#include <nil/actor/network/const.hh>
#include <nil/actor/network/net.hh>

namespace nil {
    namespace actor {

        namespace net {

            struct udp_hdr {
                packed<uint16_t> src_port;
                packed<uint16_t> dst_port;
                packed<uint16_t> len;
                packed<uint16_t> cksum;

                template<typename Adjuster>
                auto adjust_endianness(Adjuster a) {
                    return a(src_port, dst_port, len, cksum);
                }
            } __attribute__((packed));

            struct udp_channel_state {
                queue<udp_datagram> _queue;
                // Limit number of data queued into send queue
                semaphore _user_queue_space = {212992};
                udp_channel_state(size_t queue_size) : _queue(queue_size) {
                }
                future<> wait_for_send_buffer(size_t len) {
                    return _user_queue_space.wait(len);
                }
                void complete_send(size_t len) {
                    _user_queue_space.signal(len);
                }
            };

        }    // namespace net

    }    // namespace actor
}    // namespace nil
