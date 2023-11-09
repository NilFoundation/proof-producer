//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/scollectd.hh>
#include <nil/actor/core/metrics_api.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/detail/log.hh>
#include <nil/actor/detail/log-cli.hh>

#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>

#include <fstream>
#include <cstdlib>

namespace nil {
    namespace actor {

        namespace bpo = boost::program_options;

        static reactor_config reactor_config_from_app_config(app_template::config cfg) {
            reactor_config ret;
            ret.auto_handle_sigint_sigterm = cfg.auto_handle_sigint_sigterm;
            ret.task_quota = cfg.default_task_quota;
            return ret;
        }

        app_template::app_template(app_template::config cfg) :
            _cfg(std::move(cfg)), _opts(_cfg.name + " options"), _conf_reader(get_default_configuration_reader()) {
            _opts.add_options()("help,h", "show help message");

            smp::register_network_stacks();
            _opts_conf_file.add(reactor::get_options_description(reactor_config_from_app_config(_cfg)));
            _opts_conf_file.add(metrics::get_options_description());
            _opts_conf_file.add(smp::get_options_description());
            _opts_conf_file.add(scollectd::get_options_description());
            _opts_conf_file.add(log_cli::get_options_description());

            _opts.add(_opts_conf_file);
        }

        app_template::configuration_reader app_template::get_default_configuration_reader() {
            return [this](boost::program_options::variables_map &configuration) {
                auto home = std::getenv("HOME");
                if (home) {
                    std::ifstream ifs(std::string(home) + "/.config/actor/actor.conf");
                    if (ifs) {
                        boost::program_options::store(boost::program_options::parse_config_file(ifs, _opts_conf_file),
                                                      configuration);
                    }
                    std::ifstream ifs_io(std::string(home) + "/.config/actor/io.conf");
                    if (ifs_io) {
                        boost::program_options::store(
                            boost::program_options::parse_config_file(ifs_io, _opts_conf_file), configuration);
                    }
                }
            };
        }

        void app_template::set_configuration_reader(configuration_reader conf_reader) {
            _conf_reader = conf_reader;
        }

        boost::program_options::options_description &app_template::get_options_description() {
            return _opts;
        }

        boost::program_options::options_description &app_template::get_conf_file_options_description() {
            return _opts_conf_file;
        }

        boost::program_options::options_description_easy_init app_template::add_options() {
            return _opts.add_options();
        }

        void app_template::add_positional_options(std::initializer_list<positional_option> options) {
            for (auto &&o : options) {
                _opts.add(
                    boost::make_shared<boost::program_options::option_description>(o.name, o.value_semantic, o.help));
                _pos_opts.add(o.name, o.max_count);
            }
        }

        bpo::variables_map &app_template::configuration() {
            return *_configuration;
        }

        int app_template::run(int ac, char **av, std::function<future<int>()> &&func) {
            return run_deprecated(ac, av, [func = std::move(func)]() mutable {
                auto func_done = make_lw_shared<promise<>>();
                engine().at_exit([func_done] { return func_done->get_future(); });
                // No need to wait for this future.
                // func's returned exit_code is communicated via engine().exit()
                (void)futurize_invoke(func)
                    .finally([func_done] { func_done->set_value(); })
                    .then([](int exit_code) { return engine().exit(exit_code); })
                    .or_terminate();
            });
        }

        int app_template::run(int ac, char **av, std::function<future<>()> &&func) {
            return run(ac, av, [func = std::move(func)] { return func().then([]() { return 0; }); });
        }

        int app_template::run_deprecated(int ac, char **av, std::function<void()> &&func) {
#ifdef ACTOR_DEBUG
            fmt::print("WARNING: debug mode. Not for benchmarking or production\n");
#endif
            boost::program_options::variables_map configuration;
            try {
                boost::program_options::store(
                    boost::program_options::command_line_parser(ac, av).options(_opts).positional(_pos_opts).run(),
                    configuration);
                _conf_reader(configuration);
            } catch (boost::program_options::error &e) {
                fmt::print("error: {}\n\nTry --help.\n", e.what());
                return 2;
            }
            if (configuration.count("help")) {
                if (!_cfg.description.empty()) {
                    std::cout << _cfg.description << "\n";
                }
                std::cout << _opts << "\n";
                return 1;
            }
            if (configuration["help-loggers"].as<bool>()) {
                log_cli::print_available_loggers(std::cout);
                return 1;
            }

            try {
                boost::program_options::notify(configuration);
            } catch (const boost::program_options::required_option &ex) {
                std::cout << ex.what() << std::endl;
                return 1;
            }

            // Needs to be before `smp::configure()`.
            try {
                apply_logging_settings(log_cli::extract_settings(configuration));
            } catch (const std::runtime_error &exn) {
                std::cout << "logging configuration error: " << exn.what() << '\n';
                return 1;
            }

            configuration.emplace("argv0", boost::program_options::variable_value(std::string(av[0]), false));
            try {
                smp::configure(configuration, reactor_config_from_app_config(_cfg));
            } catch (...) {
                std::cerr << "Could not initialize actor: " << std::current_exception() << std::endl;
                return 1;
            }
            _configuration = {std::move(configuration)};
            // No need to wait for this future.
            // func is waited on via engine().run()
            (void)engine()
                .when_started()
                .then([this] {
                    return metrics::configure(this->configuration()).then([this] {
                        // set scollectd use the metrics configuration, so the later
                        // need to be set first
                        scollectd::configure(this->configuration());
                    });
                })
                .then(std::move(func))
                .then_wrapped([](auto &&f) {
                    try {
                        f.get();
                    } catch (std::exception &ex) {
                        std::cout << "program failed with uncaught exception: " << ex.what() << "\n";
                        engine().exit(1);
                    }
                });
            auto exit_code = engine().run();
            smp::cleanup();
            return exit_code;
        }

    }    // namespace actor
}    // namespace nil
