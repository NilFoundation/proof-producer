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

#include <nil/actor/detail/log.hh>
#include <nil/actor/detail/program-options.hh>

#include <nil/actor/core/sstring.hh>

#include <boost/program_options.hpp>

#include <algorithm>
#include <unordered_map>

/// \addtogroup logging
/// @{
namespace nil {
    namespace actor {

        ///
        /// \brief Configure application logging at run-time with program options.
        ///
        namespace log_cli {

            ///
            /// \brief Options for controlling logging at run-time.
            ///
            boost::program_options::options_description get_options_description();

            ///
            /// \brief Print a human-friendly list of the available loggers.
            ///
            void print_available_loggers(std::ostream &os);

            ///
            /// \brief Parse a log-level ({error, warn, info, debug, trace}) string, throwing \c std::runtime_error for
            /// an invalid level.
            ///
            log_level parse_log_level(const sstring &);

            //
            // \brief Parse associations from loggers to log-levels and write the resulting pairs to the output
            // iterator.
            //
            // \throws \c std::runtime_error for an invalid log-level.
            //
            template<class OutputIter>
            void parse_logger_levels(const program_options::string_map &levels, OutputIter out) {
                std::for_each(levels.begin(), levels.end(), [&out](auto &&pair) {
                    *out++ = std::make_pair(pair.first, parse_log_level(pair.second));
                });
            }

            ///
            /// \brief Extract CLI options into a logging configuration.
            //
            logging_settings extract_settings(const boost::program_options::variables_map &);

        }    // namespace log_cli

    }    // namespace actor
}    // namespace nil

/// @}
