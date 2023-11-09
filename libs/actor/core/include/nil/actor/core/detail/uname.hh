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

#include <string>
#include <initializer_list>
#include <iosfwd>

#include <boost/optional.hpp>

namespace nil {
    namespace actor {

        namespace detail {

            // Representation of a Linux kernel version number
            struct uname_t {
                int version;                        // 4 in "4.5"
                int patchlevel;                     // 5 in "4.5"
                boost::optional<int> sublevel;        // 1 in "4.5.1"
                boost::optional<int> subsublevel;     // 33 in "2.6.44.33"
                boost::optional<int> distro_patch;    // 957 in "3.10.0-957.5.1.el7.x86_64"
                std::string distro_extra;           // .5.1.el7.x86_64

                bool same_as_or_descendant_of(const uname_t &x) const;
                bool same_as_or_descendant_of(const char *x) const;
                bool whitelisted(std::initializer_list<const char *>) const;

                // 3 for "4.5.0", 5 for "5.1.3-33.3.el7"; "el7" doesn't count as a component
                int component_count() const;

                // The "el7" that wasn't counted in components()
                bool has_distro_extra(std::string extra) const;
                friend std::ostream &operator<<(std::ostream &os, const uname_t &u);
            };

            uname_t kernel_uname();

            uname_t parse_uname(const char *u);

        }    // namespace detail
    }        // namespace actor
}    // namespace nil
