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

#include <nil/actor/network/packet.hh>
#include <nil/actor/core/iostream.hh>

namespace nil {
    namespace actor {

        namespace net {

            class packet_data_source final : public data_source_impl {
                size_t _cur_frag = 0;
                packet _p;

            public:
                explicit packet_data_source(net::packet &&p) : _p(std::move(p)) {
                }

                virtual future<temporary_buffer<char>> get() override {
                    if (_cur_frag != _p.nr_frags()) {
                        auto &f = _p.fragments()[_cur_frag++];
                        return make_ready_future<temporary_buffer<char>>(temporary_buffer<char>(
                            f.base, f.size, make_deleter(deleter(), [p = _p.share()]() mutable {})));
                    }
                    return make_ready_future<temporary_buffer<char>>(temporary_buffer<char>());
                }
            };

            static inline input_stream<char> as_input_stream(packet &&p) {
                return input_stream<char>(data_source(std::make_unique<packet_data_source>(std::move(p))));
            }

        }    // namespace net

    }    // namespace actor
}    // namespace nil
