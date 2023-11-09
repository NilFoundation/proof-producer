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

namespace nil {
    namespace actor {

        namespace scollectd {

            using collectd_value = nil::actor::metrics::impl::metric_value;

            std::vector<collectd_value> get_collectd_value(const scollectd::type_instance_id &id);

            std::vector<scollectd::type_instance_id> get_collectd_ids();

            sstring get_collectd_description_str(const scollectd::type_instance_id &);

            bool is_enabled(const scollectd::type_instance_id &id);
            /**
             * Enable or disable collectd metrics on local instance
             * @param id - the metric to enable or disable
             * @param enable - should the collectd metrics be enable or disable
             */
            void enable(const scollectd::type_instance_id &id, bool enable);

            metrics::impl::value_map get_value_map();
        }    // namespace scollectd

    }    // namespace actor
}    // namespace nil
