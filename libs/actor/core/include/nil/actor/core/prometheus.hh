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

#include <nil/actor/http/httpd.hh>
#include <nil/actor/core/metrics.hh>
#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        namespace prometheus {

            /*!
             * Holds prometheus related configuration
             */
            struct config {
                sstring metric_help;    //!< Default help message for the returned metrics
                sstring hostname;       //!< hostname is deprecated, use label instead
                boost::optional<metrics::label_instance>
                    label;                     //!< A label that will be added to all metrics, we advice
                                               //!< not to use it and set it on the prometheus server
                sstring prefix = "actor";    //!< a prefix that will be added to metric names
            };

            future<> start(httpd::http_server_control &http_server, config ctx);

            /// \defgroup add_prometheus_routes adds a /metrics endpoint that returns prometheus metrics
            ///    both in txt format and in protobuf according to the prometheus spec
            /// @{
            future<> add_prometheus_routes(distributed<http_server> &server, config ctx);
            future<> add_prometheus_routes(http_server &server, config ctx);
            /// @}
        }    // namespace prometheus
    }        // namespace actor
}    // namespace nil
