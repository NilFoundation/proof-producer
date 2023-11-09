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
    \brief Some non-INET-specific socket address code

    Extracted from inet_address.cc.
 */
#include <ostream>
#include <arpa/inet.h>

#include <nil/actor/network/socket_defs.hh>
#include <nil/actor/network/inet_address.hh>
#include <nil/actor/network/ip.hh>

#include <nil/actor/core/print.hh>

#include <boost/functional/hash.hpp>

/*
 * Equality
 * NOTE: Some of kernel programming environment (for example, openbsd/sparc)
 * does not supply memcmp().  For userland memcmp() is preferred as it is
 * in ANSI standard.
 *
 * This modification is specific to FreeBSD's struct sin6_addr implementation.
 */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SIN6_ARE_ADDR_EQUAL(a, b) \
    (memcmp(&(a)->sin6_addr.s6_addr[0], &(b)->sin6_addr.s6_addr[0], sizeof(struct in6_addr)) == 0)
#endif /* (_POSIX_C_SOURCE && !_DARWIN_C_SOURCE) */

using namespace std::string_literals;

size_t std::hash<nil::actor::socket_address>::operator()(const nil::actor::socket_address &a) const {
    auto h = std::hash<nil::actor::net::inet_address>()(a.addr());
    boost::hash_combine(h, a.as_posix_sockaddr_in().sin_port);
    return h;
}

namespace nil {
    namespace actor {

        static_assert(std::is_nothrow_default_constructible<socket_address>::value);
        static_assert(std::is_nothrow_copy_constructible<socket_address>::value);
        static_assert(std::is_nothrow_move_constructible<socket_address>::value);

        socket_address::socket_address() noexcept
            // set max addr_length, as we (probably) want to use the constructed object
            // in accept() or get_address()
            :
            addr_length(sizeof(::sockaddr_storage)) {
            static_assert(AF_UNSPEC == 0, "just checking");
            memset(&u, 0, sizeof(u));
        }

        socket_address::socket_address(uint16_t p) noexcept : socket_address(ipv4_addr(p)) {
        }

        socket_address::socket_address(ipv4_addr addr) noexcept {
            addr_length = sizeof(::sockaddr_in);
            u.in.sin_family = AF_INET;
            u.in.sin_port = htons(addr.port);
            u.in.sin_addr.s_addr = htonl(addr.ip);
        }

        socket_address::socket_address(const ipv6_addr &addr, uint32_t scope) noexcept {
            addr_length = sizeof(::sockaddr_in6);
            u.in6.sin6_family = AF_INET6;
            u.in6.sin6_port = htons(addr.port);
            u.in6.sin6_flowinfo = 0;
            u.in6.sin6_scope_id = scope;
            std::copy(addr.ip.begin(), addr.ip.end(), u.in6.sin6_addr.s6_addr);
        }

        socket_address::socket_address(const ipv6_addr &addr) noexcept :
            socket_address(addr, net::inet_address::invalid_scope) {
        }

        socket_address::socket_address(uint32_t ipv4, uint16_t p) noexcept :
            socket_address(make_ipv4_address(ipv4, p)) {
        }

        socket_address::socket_address(const net::inet_address &a, uint16_t p) noexcept :
            socket_address(a.is_ipv6() ? socket_address(ipv6_addr(a, p), a.scope()) : socket_address(ipv4_addr(a, p))) {
        }

        socket_address::socket_address(const unix_domain_addr &s) noexcept {
            u.un.sun_family = AF_UNIX;
            memset(u.un.sun_path, '\0', sizeof(u.un.sun_path));
            auto path_length = std::min((int)sizeof(u.un.sun_path), s.path_length());
            memcpy(u.un.sun_path, s.path_bytes(), path_length);
            addr_length = path_length + offsetof(struct ::sockaddr_un, sun_path);
        }

        bool socket_address::is_unspecified() const noexcept {
            return u.sa.sa_family == AF_UNSPEC;
        }

        static int adjusted_path_length(const socket_address &a) noexcept {
            int l = std::max(0, (int)a.addr_length - (int)(offsetof(sockaddr_un, sun_path)));
            // "un-count" a trailing null in filesystem-namespace paths
            if (a.u.un.sun_path[0] != '\0' && (l > 1) && a.u.un.sun_path[l - 1] == '\0') {
                --l;
            }
            return l;
        }

        bool socket_address::operator==(const socket_address &a) const noexcept {
            if (u.sa.sa_family != a.u.sa.sa_family) {
                return false;
            }
            if (u.sa.sa_family == AF_UNIX) {
                // tolerate counting/not counting a terminating null in filesystem-namespace paths
                int adjusted_len = adjusted_path_length(*this);
                int a_adjusted_len = adjusted_path_length(a);
                if (adjusted_len != a_adjusted_len) {
                    return false;
                }
                return (memcmp(u.un.sun_path, a.u.un.sun_path, adjusted_len) == 0);
            }

            // an INET address
            if (u.in.sin_port != a.u.in.sin_port) {
                return false;
            }
            switch (u.sa.sa_family) {
                case AF_INET:
                    return u.in.sin_addr.s_addr == a.u.in.sin_addr.s_addr;
                case AF_UNSPEC:
                case AF_INET6:
                    // handled below
                    break;
                default:
                    return false;
            }

            auto &in1 = as_posix_sockaddr_in6();
            auto &in2 = a.as_posix_sockaddr_in6();

#if defined(__APPLE__) || defined(__FreeBSD__)
            return SIN6_ARE_ADDR_EQUAL(&in1, &in2);
#elif defined(__linux__)
            return IN6_ARE_ADDR_EQUAL(&in1, &in2);
#endif
        }

        std::string unix_domain_addr_text(const socket_address &sa) {
            if (sa.length() <= offsetof(sockaddr_un, sun_path)) {
                return "{unnamed}"s;
            }
            if (sa.u.un.sun_path[0]) {
                // regular (filesystem-namespace) path
                return std::string {sa.u.un.sun_path};
            }

            const size_t path_length {sa.length() - offsetof(sockaddr_un, sun_path)};
            std::string ud_path(path_length, 0);
            char *targ = &ud_path[0];
            *targ++ = '@';
            const char *src = sa.u.un.sun_path + 1;
            int k = (int)path_length;

            for (; --k > 0; src++) {
                *targ++ = std::isprint(*src) ? *src : '_';
            }
            return ud_path;
        }

        std::ostream &operator<<(std::ostream &os, const socket_address &a) {
            if (a.is_af_unix()) {
                return os << unix_domain_addr_text(a);
            }

            auto addr = a.addr();
            // CMH. maybe skip brackets for ipv4-mapped
            auto bracket = addr.in_family() == nil::actor::net::inet_address::family::INET6;

            if (bracket) {
                os << '[';
            }
            os << addr;
            if (bracket) {
                os << ']';
            }

            return os << ':' << ntohs(a.u.in.sin_port);
        }

    }    // namespace actor
}    // namespace nil
