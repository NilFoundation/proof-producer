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

#include <nil/actor/core/sstring.hh>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace nil {
    namespace actor {

        namespace program_options {

            ///
            /// \brief Wrapper for command-line options with arbitrary string associations.
            ///
            /// This type, to be used with Boost.Program_options, will result in an option that stores an arbitrary
            /// number of string associations.
            ///
            /// Values are specified in the form "key0=value0:[key1=value1:...]". Options of this type can be specified
            /// multiple times, and the values will be merged (with the last-provided value for a key taking
            /// precedence).
            ///
            /// \note We need a distinct type (rather than a simple type alias) for overload resolution in the
            /// implementation, but advertizing our inheritance of \c std::unordered_map would introduce the possibility
            /// of memory leaks since STL containers do not declare virtual destructors.
            ///
            class string_map final : private std::unordered_map<sstring, sstring> {
            private:
                using base = std::unordered_map<sstring, sstring>;

            public:
                using base::key_type;
                using base::mapped_type;
                using base::value_type;

                using base::at;
                using base::base;
                using base::clear;
                using base::count;
                using base::emplace;
                using base::find;
                using base::operator[];
                using base::begin;
                using base::end;

                friend bool operator==(const string_map &, const string_map &);
                friend bool operator!=(const string_map &, const string_map &);
            };

            inline bool operator==(const string_map &lhs, const string_map &rhs) {
                return static_cast<const string_map::base &>(lhs) == static_cast<const string_map::base &>(rhs);
            }

            inline bool operator!=(const string_map &lhs, const string_map &rhs) {
                return !(lhs == rhs);
            }

            ///
            /// \brief Query the value of a key in a \c string_map, or a default value if the key doesn't exist.
            ///
            sstring get_or_default(const string_map &, const sstring &key, const sstring &def = sstring());

            std::istream &operator>>(std::istream &is, string_map &);
            std::ostream &operator<<(std::ostream &os, const string_map &);

            /// \cond internal

            //
            // Required implementation hook for Boost.Program_options.
            //
            void validate(boost::any &out, const std::vector<std::string> &in, string_map *, int);

            /// \endcond

        }    // namespace program_options

    }    // namespace actor
}    // namespace nil
