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

#include <chrono>
#include <functional>

#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <nil/actor/core/future.hh>
#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {

        class app_template {
        public:
            struct config {
                /// The name of the application.
                ///
                /// Will be used in the --help output to distinguish command line args
                /// registered by the application, as opposed to those registered by
                /// actor and its subsystems.
                sstring name = "App";
                /// The description of the application.
                ///
                /// Will be printed on the top of the --help output. Lines should be
                /// hard-wrapped for 80 chars.
                sstring description = "";
                std::chrono::duration<double> default_task_quota = std::chrono::microseconds(500);
                /// \brief Handle SIGINT/SIGTERM by calling reactor::stop()
                ///
                /// When true, =nil; Actor will set up signal handlers for SIGINT/SIGTERM that call
                /// reactor::stop(). The reactor will then execute callbacks installed by
                /// reactor::at_exit().
                ///
                /// When false, =nil; Actor will not set up signal handlers for SIGINT/SIGTERM
                /// automatically. The default behavior (terminate the program) will be kept.
                /// You can adjust the behavior of SIGINT/SIGTERM by installing signal handlers
                /// via reactor::handle_signal().
                bool auto_handle_sigint_sigterm = true;
                config() {
                }
            };

            using configuration_reader = std::function<void(boost::program_options::variables_map &)>;

        private:
            config _cfg;
            boost::program_options::options_description _opts;
            boost::program_options::options_description _opts_conf_file;
            boost::program_options::positional_options_description _pos_opts;
            boost::optional<boost::program_options::variables_map> _configuration;
            configuration_reader _conf_reader;

            configuration_reader get_default_configuration_reader();

        public:
            struct positional_option {
                const char *name;
                const boost::program_options::value_semantic *value_semantic;
                const char *help;
                int max_count;
            };
            explicit app_template(config cfg = config());

            boost::program_options::options_description &get_options_description();
            boost::program_options::options_description &get_conf_file_options_description();
            boost::program_options::options_description_easy_init add_options();
            void add_positional_options(std::initializer_list<positional_option> options);
            boost::program_options::variables_map &configuration();
            int run_deprecated(int ac, char **av, std::function<void()> &&func);

            void set_configuration_reader(configuration_reader conf_reader);

            // Runs given function and terminates the application when the future it
            // returns resolves. The value with which the future resolves will be
            // returned by this function.
            int run(int ac, char **av, std::function<future<int>()> &&func);

            // Like run() which takes std::function<future<int>()>, but returns
            // with exit code 0 when the future returned by func resolves
            // successfully.
            int run(int ac, char **av, std::function<future<>()> &&func);
        };

    }    // namespace actor
}    // namespace nil
