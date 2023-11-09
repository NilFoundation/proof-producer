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

#include <vector>

namespace nil {
    namespace actor {
        namespace metrics {

            /*!
             * \brief Histogram bucket type
             *
             * A histogram bucket contains an upper bound and the number
             * of events in the buckets.
             */
            struct histogram_bucket {
                uint64_t count = 0;        // number of events.
                double upper_bound = 0;    // Inclusive.
            };

            /*!
             * \brief Histogram data type
             *
             * The histogram struct is a container for histogram values.
             * It is not a histogram implementation but it will be used by histogram
             * implementation to return its internal values.
             */
            struct histogram {
                uint64_t sample_count = 0;
                double sample_sum = 0;
                std::vector<histogram_bucket>
                    buckets;    // Ordered in increasing order of upper_bound, +Inf bucket is optional.

                /*!
                 * \brief Addition assigning a historgram
                 *
                 * The histogram must match the buckets upper bounds
                 * or an exception will be thrown
                 */
                histogram &operator+=(const histogram &h);

                /*!
                 * \brief Addition historgrams
                 *
                 * Add two histograms and return the result as a new histogram
                 * The histogram must match the buckets upper bounds
                 * or an exception will be thrown
                 */
                histogram operator+(const histogram &h) const;

                /*!
                 * \brief Addition historgrams
                 *
                 * Add two histograms and return the result as a new histogram
                 * The histogram must match the buckets upper bounds
                 * or an exception will be thrown
                 */
                histogram operator+(histogram &&h) const;
            };

        }    // namespace metrics

    }    // namespace actor
}    // namespace nil
