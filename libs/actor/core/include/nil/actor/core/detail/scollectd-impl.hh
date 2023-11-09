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

#include <nil/actor/core/scollectd.hh>
#include <nil/actor/core/metrics_api.hh>
#include <nil/actor/network/api.hh>

namespace nil {
    namespace actor {

        namespace scollectd {

            using namespace std::chrono_literals;
            using duration = std::chrono::milliseconds;

            static const ipv4_addr default_addr("239.192.74.66:25826");
            static const std::chrono::milliseconds default_period(1s);

            class impl {
                net::udp_channel _chan;
                timer<> _timer;

                sstring _host = "localhost";
                ipv4_addr _addr = default_addr;
                std::chrono::milliseconds _period = default_period;
                uint64_t _num_packets = 0;
                uint64_t _millis = 0;
                uint64_t _bytes = 0;
                double _avg = 0;

            public:
                typedef nil::actor::metrics::impl::value_map value_list_map;
                typedef value_list_map::value_type value_list_pair;

                void add_polled(const type_instance_id &id, const shared_ptr<value_list> &values, bool enable = true);
                void remove_polled(const type_instance_id &id);
                // explicitly send a type_instance value list (outside polling)
                future<> send_metric(const type_instance_id &id, const value_list &values);
                future<> send_notification(const type_instance_id &id, const sstring &msg);
                // initiates actual value polling -> send to target "loop"
                void start(const sstring &host, const ipv4_addr &addr, const std::chrono::milliseconds period);
                void stop();

                value_list_map &get_value_list_map();
                const sstring &host() const {
                    return _host;
                }

            private:
                void arm();
                void run();

            public:
                shared_ptr<value_list> get_values(const type_instance_id &id) const;
                std::vector<type_instance_id> get_instance_ids() const;
                sstring get_collectd_description_str(const scollectd::type_instance_id &) const;

            private:
                const value_list_map &values() const {
                    return nil::actor::metrics::impl::get_value_map();
                }
                nil::actor::metrics::metric_groups _metrics;
            };

            impl &get_impl();

        };    // namespace scollectd

    }    // namespace actor
}    // namespace nil
