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

#include <nil/actor/network/proxy.hh>
#include <utility>

namespace nil {
    namespace actor {

        namespace net {

            class proxy_net_device : public qp {
            private:
                static constexpr size_t _send_queue_length = 128;
                size_t _send_depth = 0;
                unsigned _cpu;
                device *_dev;
                std::vector<packet> _moving;

            public:
                explicit proxy_net_device(unsigned cpu, device *dev);
                virtual future<> send(packet p) override {
                    abort();
                }
                virtual uint32_t send(circular_buffer<packet> &p) override;
            };

            proxy_net_device::proxy_net_device(unsigned cpu, device *dev) : _cpu(cpu), _dev(dev) {
                _moving.reserve(_send_queue_length);
            }

            uint32_t proxy_net_device::send(circular_buffer<packet> &p) {
                if (!_moving.empty() || _send_depth == _send_queue_length) {
                    return 0;
                }

                for (; !p.empty() && _send_depth < _send_queue_length; _send_depth++) {
                    _moving.push_back(std::move(p.front()));
                    p.pop_front();
                }

                if (!_moving.empty()) {
                    qp *dev = &_dev->queue_for_cpu(_cpu);
                    auto cpu = this_shard_id();
                    // FIXME: future is discarded
                    (void)smp::submit_to(_cpu, [this, dev, cpu]() mutable {
                        for (size_t i = 0; i < _moving.size(); i++) {
                            dev->proxy_send(_moving[i].free_on_cpu(cpu, [this] { _send_depth--; }));
                        }
                    }).then([this] { _moving.clear(); });
                }

                return _moving.size();
            }

            std::unique_ptr<qp> create_proxy_net_device(unsigned master_cpu, device *dev) {
                return std::make_unique<proxy_net_device>(master_cpu, dev);
            }
        }    // namespace net

    }    // namespace actor
}    // namespace nil
