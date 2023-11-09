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

#include <nil/actor/testing/perf_tests.hh>

#include <fstream>
#include <regex>

#include <boost/predef.h>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <fmt/ostream.h>

#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/sharded.hh>
#include <nil/actor/json/formatter.hh>
#include <nil/actor/detail/later.hh>
#include <nil/actor/testing/random.hh>

#include <csignal>

#if BOOST_OS_MACOS || BOOST_OS_IOS
#include <dispatch/dispatch.h>
#endif

namespace perf_tests {
    namespace detail {

        namespace {

            // We need to use signal-based timer instead of actor ones so that
            // tests that do not suspend can be interrupted.
            // This causes no overhead though since the timer is used only in a dry run.
            class signal_timer {
                std::function<void()> _fn;
#if BOOST_OS_LINUX
                timer_t _timer;
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                dispatch_source_t _timer;
#endif
            public:
                explicit signal_timer(std::function<void()> fn) : _fn(fn) {
#if BOOST_OS_LINUX
                    sigevent se {};
                    se.sigev_notify = SIGEV_SIGNAL;
                    se.sigev_signo = SIGALRM;
                    se.sigev_value.sival_ptr = this;
                    auto ret = timer_create(CLOCK_MONOTONIC, &se, &_timer);
                    if (ret) {
                        throw std::system_error(ret, std::system_category());
                    }
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
                    if (!_timer) {
                        throw std::system_error(1, std::system_category());
                    }
                    dispatch_source_set_event_handler(_timer, ^{
                        raise(SIGALRM);
                    });
#endif
                }

                ~signal_timer() {
#if BOOST_OS_LINUX
                    timer_delete(_timer);
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    dispatch_source_cancel(_timer);
#endif
                }

                void arm(std::chrono::steady_clock::duration dt) {
                    time_t sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
                    auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count();
                    nsec -= std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(sec)).count();

                    itimerspec ts {};
                    ts.it_value.tv_sec = sec;
                    ts.it_value.tv_nsec = nsec;
#if BOOST_OS_LINUX
                    auto ret = timer_settime(_timer, 0, &ts, nullptr);
                    if (ret) {
                        throw std::system_error(ret, std::system_category());
                    }
#elif BOOST_OS_MACOS || BOOST_OS_IOS
                    dispatch_source_set_timer(_timer, dispatch_walltime(&ts.it_value, 0), 1ull * NSEC_PER_SEC, 0);
#endif
                }

                void cancel() {
#if BOOST_OS_LINUX
                    itimerspec ts {};
                    auto ret = timer_settime(_timer, 0, &ts, nullptr);
                    if (ret) {
                        throw std::system_error(ret, std::system_category());
                    }
#elif BOOST_OS_IOS || BOOST_OS_MACOS
                    dispatch_source_set_timer(_timer, dispatch_walltime(NULL, 0), 1ull * NSEC_PER_SEC, 0);
#endif
                }

            public:
                static void init() {
                    struct sigaction sa { };
                    sa.sa_sigaction = &signal_timer::signal_handler;
                    sa.sa_flags = SA_SIGINFO;
                    auto ret = sigaction(SIGALRM, &sa, nullptr);
                    if (ret) {
                        throw std::system_error(ret, std::system_category());
                    }
                }

            private:
                static void signal_handler(int, siginfo_t *si, void *) {
                    auto t = static_cast<signal_timer *>(si->si_value.sival_ptr);
                    t->_fn();
                }
            };

        }    // namespace

        time_measurement measure_time;

        struct config;
        struct result;

        struct result_printer {
            virtual ~result_printer() = default;

            virtual void print_configuration(const config &) = 0;
            virtual void print_result(const result &) = 0;
        };

        struct config {
            uint64_t single_run_iterations;
            std::chrono::nanoseconds single_run_duration;
            unsigned number_of_runs;
            std::vector<std::unique_ptr<result_printer>> printers;
            unsigned random_seed = 0;
        };

        struct result {
            sstring test_name;

            uint64_t total_iterations;
            unsigned runs;

            double median;
            double mad;
            double min;
            double max;
        };

        namespace {

            struct duration {
                double value;
            };

            static inline std::ostream &operator<<(std::ostream &os, duration d) {
                auto value = d.value;
                if (value < 1'000) {
                    os << fmt::format("{:.3f}ns", value);
                } else if (value < 1'000'000) {
                    // fmt hasn't discovered unicode yet so we are stuck with uicroseconds
                    // See: https://github.com/fmtlib/fmt/issues/628
                    os << fmt::format("{:.3f}us", value / 1'000);
                } else if (value < 1'000'000'000) {
                    os << fmt::format("{:.3f}ms", value / 1'000'000);
                } else {
                    os << fmt::format("{:.3f}s", value / 1'000'000'000);
                }
                return os;
            }

        }    // namespace

        static constexpr auto format_string = "{:<40} {:>11} {:>11} {:>11} {:>11} {:>11}\n";

        struct stdout_printer final : result_printer {
            virtual void print_configuration(const config &c) override {
                fmt::print("{:<25} {}\n{:<25} {}\n{:<25} {}\n{:<25} {}\n{:<25} {}\n\n",
                           "single run iterations:", c.single_run_iterations,
                           "single run duration:", duration {double(c.single_run_duration.count())},
                           "number of runs:", c.number_of_runs, "number of cores:", smp::count,
                           "random seed:", c.random_seed);
                fmt::print(format_string, "test", "iterations", "median", "mad", "min", "max");
            }

            virtual void print_result(const result &r) override {
                fmt::print(format_string, r.test_name, r.total_iterations / r.runs, duration {r.median},
                           duration {r.mad}, duration {r.min}, duration {r.max});
            }
        };

        class json_printer final : public result_printer {
            std::string _output_file;
            std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, double>>>
                _root;

        public:
            explicit json_printer(const std::string &file) : _output_file(file) {
            }

            ~json_printer() {
                std::ofstream out(_output_file);
                out << json::formatter::to_json(_root);
            }

            virtual void print_configuration(const config &) override {
            }

            virtual void print_result(const result &r) override {
                auto &result = _root["results"][r.test_name];
                result["runs"] = r.runs;
                result["total_iterations"] = r.total_iterations;
                result["median"] = r.median;
                result["mad"] = r.mad;
                result["min"] = r.min;
                result["max"] = r.max;
            }
        };

        void performance_test::do_run(const config &conf) {
            _max_single_run_iterations = conf.single_run_iterations;
            if (!_max_single_run_iterations) {
                _max_single_run_iterations = std::numeric_limits<uint64_t>::max();
            }

            signal_timer tmr([this] { _max_single_run_iterations.store(0, std::memory_order_relaxed); });

            // dry run, estimate the number of iterations
            if (conf.single_run_duration.count()) {
                // switch out of actor thread
                later()
                    .then([&] {
                        tmr.arm(conf.single_run_duration);
                        return do_single_run().finally([&] {
                            tmr.cancel();
                            _max_single_run_iterations = _single_run_iterations;
                        });
                    })
                    .get();
            }

            auto results = std::vector<double>(conf.number_of_runs);
            uint64_t total_iterations = 0;
            for (auto i = 0u; i < conf.number_of_runs; i++) {
                // switch out of actor thread
                later()
                    .then([&] {
                        _single_run_iterations = 0;
                        return do_single_run().then([&](clock_type::duration dt) {
                            double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count();
                            results[i] = ns / _single_run_iterations;

                            total_iterations += _single_run_iterations;
                        });
                    })
                    .get();
            }

            result r {};
            r.test_name = name();
            r.total_iterations = total_iterations;
            r.runs = conf.number_of_runs;

            auto mid = conf.number_of_runs / 2;

            boost::range::sort(results);
            r.median = results[mid];

            auto diffs = boost::copy_range<std::vector<double>>(
                results | boost::adaptors::transformed([&](double x) { return fabs(x - r.median); }));
            boost::range::sort(diffs);
            r.mad = diffs[mid];

            r.min = results[0];
            r.max = results[results.size() - 1];

            for (auto &rp : conf.printers) {
                rp->print_result(r);
            }
        }

        void performance_test::run(const config &conf) {
            set_up();
            try {
                do_run(conf);
            } catch (...) {
                tear_down();
                throw;
            }
            tear_down();
        }

        std::vector<std::unique_ptr<performance_test>> &all_tests() {
            static std::vector<std::unique_ptr<performance_test>> tests;
            return tests;
        }

        void performance_test::register_test(std::unique_ptr<performance_test> test) {
            all_tests().emplace_back(std::move(test));
        }

        void run_all(const std::vector<std::string> &tests, const config &conf) {
            auto can_run = [tests = boost::copy_range<std::vector<std::regex>>(tests)](auto &&test) {
                auto it = boost::range::find_if(
                    tests, [&test](const std::regex &regex) { return std::regex_match(test->name(), regex); });
                return tests.empty() || it != tests.end();
            };

            for (auto &rp : conf.printers) {
                rp->print_configuration(conf);
            }
            for (auto &&test : all_tests() | boost::adaptors::filtered(std::move(can_run))) {
                test->run(conf);
            }
        }

    }    // namespace detail
}    // namespace perf_tests

int main(int ac, char **av) {
    using namespace perf_tests::detail;
    namespace bpo = boost::program_options;

    app_template app;
    app.add_options()("iterations,i", bpo::value<size_t>()->default_value(0), "number of iterations in a single run")(
        "duration,d", bpo::value<double>()->default_value(1),
        "duration of a single run in seconds")("runs,r", bpo::value<size_t>()->default_value(5), "number of runs")(
        "test,t", bpo::value<std::vector<std::string>>(),
        "tests to execute")("random-seed,S", bpo::value<unsigned>()->default_value(0), "random number generator seed")(
        "no-stdout", "do not print to stdout")("json-output", bpo::value<std::string>(),
                                               "output json file")("list", "list available tests");

    return app.run(ac, av, [&] {
        return async([&] {
            signal_timer::init();

            config conf;
            conf.single_run_iterations = app.configuration()["iterations"].as<size_t>();
            auto dur = std::chrono::duration<double>(app.configuration()["duration"].as<double>());
            conf.single_run_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
            conf.number_of_runs = app.configuration()["runs"].as<size_t>();
            conf.random_seed = app.configuration()["random-seed"].as<unsigned>();

            std::vector<std::string> tests_to_run;
            if (app.configuration().count("test")) {
                tests_to_run = app.configuration()["test"].as<std::vector<std::string>>();
            }

            if (app.configuration().count("list")) {
                fmt::print("available tests:\n");
                for (auto &&t : all_tests()) {
                    fmt::print("\t{}\n", t->name());
                }
                return;
            }

            if (!app.configuration().count("no-stdout")) {
                conf.printers.emplace_back(std::make_unique<stdout_printer>());
            }

            if (app.configuration().count("json-output")) {
                conf.printers.emplace_back(
                    std::make_unique<json_printer>(app.configuration()["json-output"].as<std::string>()));
            }

            if (!conf.random_seed) {
                conf.random_seed = std::random_device()();
            }
            smp::invoke_on_all([seed = conf.random_seed] {
                auto local_seed = seed + this_shard_id();
                testing::local_random_engine.seed(local_seed);
            }).get();

            run_all(tests_to_run, conf);
        });
    });
}
