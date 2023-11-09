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

#include <csignal>
#include <limits>
#include <chrono>
#include <functional>

#include <nil/actor/core/posix.hh>
#include <nil/actor/core/metrics_registration.hh>

#if BOOST_OS_MACOS || BOOST_OS_IOS
#include <dispatch/dispatch.h>
#endif

namespace nil {
    namespace actor {

        class reactor;
        class thread_cputime_clock;

        namespace detail {

            struct cpu_stall_detector_config {
                std::chrono::duration<double> threshold = std::chrono::seconds(2);
                unsigned stall_detector_reports_per_minute = 1;
                float slack = 0.3;               // fraction of threshold that we're allowed to overshoot
                std::function<void()> report;    // alternative reporting function for tests
            };

            // Detects stalls in continuations that run for too long
            class cpu_stall_detector {
#if BOOST_OS_LINUX
                timer_t _timer;
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                dispatch_source_t _timer;
#endif
                std::atomic<uint64_t> _last_tasks_processed_seen {};
                unsigned _stall_detector_reports_per_minute;
                std::atomic<uint64_t> _stall_detector_missed_ticks = {0};
                unsigned _reported = 0;
                unsigned _total_reported = 0;
                unsigned _max_reports_per_minute;
                unsigned _shard_id;
                unsigned _thread_id;
                unsigned _report_at {};
                std::chrono::steady_clock::time_point _minute_mark {};
                std::chrono::steady_clock::time_point _rearm_timer_at {};
                std::chrono::steady_clock::time_point _run_started_at {};
                std::chrono::steady_clock::duration _threshold;
                std::chrono::steady_clock::duration _slack;
                cpu_stall_detector_config _config;
                nil::actor::metrics::metric_groups _metrics;
                friend reactor;

            private:
                void maybe_report();
                void arm_timer();
                void report_suppressions(std::chrono::steady_clock::time_point now);

            public:
                using clock_type = thread_cputime_clock;

            public:
                explicit cpu_stall_detector(const cpu_stall_detector_config& cfg = {});
                ~cpu_stall_detector();
                static int signal_number() {
#if BOOST_OS_LINUX
                    return SIGRTMIN + 1;
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    return SIGUSR2;
#endif
                }
                void start_task_run(std::chrono::steady_clock::time_point now);
                void end_task_run(std::chrono::steady_clock::time_point now);
                void generate_trace();
                void update_config(const cpu_stall_detector_config& cfg);
                cpu_stall_detector_config get_config() const;
                void on_signal();
                void start_sleep();
                void end_sleep();
            };

        }    // namespace detail
    }        // namespace actor
}    // namespace nil
