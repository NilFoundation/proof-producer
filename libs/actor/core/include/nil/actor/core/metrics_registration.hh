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

#include <memory>
#include <vector>

#include <nil/actor/core/sstring.hh>

/*!
 * \file metrics_registration.hh
 * \brief holds the metric_groups definition needed by class that reports metrics
 *
 * If class A needs to report metrics,
 * typically you include metrics_registration.hh, in A header file and add to A:
 * * metric_groups _metrics as a member
 * * set_metrics() method that would be called in the constructor.
 * \code
 * class A {
 *   metric_groups _metrics
 *
 *   void setup_metrics();
 *
 * };
 * \endcode
 * To define the metrics, include in your source file metircs.hh
 * @see metrics.hh for the definition for adding a metric.
 */

namespace nil {
    namespace actor {

        namespace metrics {

            namespace impl {
                class metric_groups_def;
                struct metric_definition_impl;
                class metric_groups_impl;
            }    // namespace impl

            using group_name_type = sstring; /*!< A group of logically related metrics */
            class metric_groups;

            class metric_definition {
                std::unique_ptr<impl::metric_definition_impl> _impl;

            public:
                metric_definition(const impl::metric_definition_impl &impl) noexcept;
                metric_definition(metric_definition &&m) noexcept;
                ~metric_definition();
                friend metric_groups;
                friend impl::metric_groups_impl;
            };

            class metric_group_definition {
            public:
                group_name_type name;
                std::initializer_list<metric_definition> metrics;
                metric_group_definition(const group_name_type &name, std::initializer_list<metric_definition> l);
                metric_group_definition(const metric_group_definition &) = delete;
                ~metric_group_definition();
            };

            /*!
             * metric_groups
             * \brief holds the metric definition.
             *
             * Add multiple metric groups definitions.
             * Initialization can be done in the constructor or with a call to add_group
             * @see metrics.hh for example and supported metrics
             */
            class metric_groups {
                std::unique_ptr<impl::metric_groups_def> _impl;

            public:
                metric_groups() noexcept;
                metric_groups(metric_groups &&) = default;
                virtual ~metric_groups();
                metric_groups &operator=(metric_groups &&) = default;
                /*!
                 * \brief add metrics belong to the same group in the constructor.
                 *
                 * combine the constructor with the add_group functionality.
                 */
                metric_groups(std::initializer_list<metric_group_definition> mg);

                /*!
                 * \brief Add metrics belonging to the same group.
                 *
                 * Use the metrics creation functions to add metrics.
                 *
                 * For example:
                 *  _metrics.add_group("my_group", {
                 *      make_counter("my_counter_name1", counter, description("my counter description")),
                 *      make_counter("my_counter_name2", counter, description("my second counter description")),
                 *      make_gauge("my_gauge_name1", gauge, description("my gauge description")),
                 *  });
                 *
                 * Metric name should be unique inside the group.
                 * You can chain add_group calls like:
                 * _metrics.add_group("my group1", {...}).add_group("my group2", {...});
                 *
                 * This overload (with initializer_list) is needed because metric_definition
                 * has no copy constructor, so the other overload (with vector) cannot be
                 * invoked on a braced-init-list.
                 */
                metric_groups &add_group(const group_name_type &name,
                                         const std::initializer_list<metric_definition> &l);

                /*!
                 * \brief Add metrics belonging to the same group.
                 *
                 * Use the metrics creation functions to add metrics.
                 *
                 * For example:
                 *  vector<metric_definition> v;
                 *  v.push_back(make_counter("my_counter_name1", counter, description("my counter description")));
                 *  v.push_back(make_counter("my_counter_name2", counter, description("my second counter
                 * description"))); v.push_back(make_gauge("my_gauge_name1", gauge, description("my gauge
                 * description"))); _metrics.add_group("my_group", v);
                 *
                 * Metric name should be unique inside the group.
                 * You can chain add_group calls like:
                 * _metrics.add_group("my group1", vec1).add_group("my group2", vec2);
                 */
                metric_groups &add_group(const group_name_type &name, const std::vector<metric_definition> &l);

                /*!
                 * \brief clear all metrics groups registrations.
                 */
                void clear();
            };

            /*!
             * \brief hold a single metric group
             * Initialization is done in the constructor or
             * with a call to add_group
             */
            class metric_group : public metric_groups {
            public:
                metric_group() noexcept;
                metric_group(const metric_group &) = delete;
                metric_group(metric_group &&) = default;
                virtual ~metric_group();
                metric_group &operator=(metric_group &&) = default;

                /*!
                 * \brief add metrics belong to the same group in the constructor.
                 *
                 *
                 */
                metric_group(const group_name_type &name, std::initializer_list<metric_definition> l);
            };

        }    // namespace metrics
    }        // namespace actor
}    // namespace nil
