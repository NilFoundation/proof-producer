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
//
#pragma once

#include <iosfwd>
#include <sys/types.h>
#include <sys/un.h>
#include <string>

namespace nil {
    namespace actor {

        /*!
            A helper struct for creating/manipulating UNIX-domain sockets.

            A UNIX-domain socket is either named or unnamed. If named, the name is either
            a path in the filesystem namespace, or an abstract-domain identifier. Abstract-domain
            names start with a null byte, and may contain non-printable characters.

            std::string() can hold a sequence of arbitrary bytes, and has a length() attribute
            that does not rely on using strlen(). Thus it is used here to hold the address.
         */
        struct unix_domain_addr {
            const std::string name;
            const int path_count;    //  either name.length() or name.length()+1. See path_length_aux() below.

            explicit unix_domain_addr(const std::string &fn) : name {fn}, path_count {path_length_aux()} {
            }

            explicit unix_domain_addr(const char *fn) : name {fn}, path_count {path_length_aux()} {
            }

            int path_length() const {
                return path_count;
            }

            //  the following holds:
            //  for abstract name: name.length() == number of meaningful bytes, including the null in name[0].
            //  for filesystem path: name.length() does not count the implicit terminating null.
            //  Here we tweak the outside-visible length of the address.
            int path_length_aux() const {
                auto pl = (int)name.length();
                if (!pl || (name[0] == '\0')) {
                    // unnamed, or abstract-namespace
                    return pl;
                }
                return 1 + pl;
            }

            const char *path_bytes() const {
                return name.c_str();
            }

            bool operator==(const unix_domain_addr &a) const {
                return name == a.name;
            }
            bool operator!=(const unix_domain_addr &a) const {
                return !(*this == a);
            }
        };

        std::ostream &operator<<(std::ostream &, const unix_domain_addr &);

    }    // namespace actor
}    // namespace nil
