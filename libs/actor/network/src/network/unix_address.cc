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
/*! \file
  \brief unix-domain address structures, to be used for creating socket_address-es for unix-domain
         sockets.

  Note that the path in a unix-domain address may start with a null character.
*/

#include <ostream>
#include <cassert>

#include <nil/actor/network/socket_defs.hh>

namespace nil {
    namespace actor {

        std::ostream &operator<<(std::ostream &os, const unix_domain_addr &addr) {
            if (addr.path_length() == 0) {
                return os << "{unnamed}";
            }
            if (addr.name[0]) {
                // regular (filesystem-namespace) path
                return os << addr.name;
            }

            os << '@';
            const char *src = addr.path_bytes() + 1;

            for (auto k = addr.path_length(); --k > 0; src++) {
                os << (std::isprint(*src) ? *src : '_');
            }
            return os;
        }

    }    // namespace actor
}    // namespace nil

size_t std::hash<nil::actor::unix_domain_addr>::operator()(const nil::actor::unix_domain_addr &a) const {
    return std::hash<std::string>()(a.name);
}
