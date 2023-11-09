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

#include <ostream>
#include <arpa/inet.h>

#include <boost/functional/hash.hpp>

#include <nil/actor/network/inet_address.hh>
#include <nil/actor/network/socket_defs.hh>
#include <nil/actor/network/dns.hh>
#include <nil/actor/network/ip.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/print.hh>

#if defined(__APPLE__)
#ifndef s6_addr16
#define s6_addr16 __u6_addr.__u6_addr16
#endif
#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif
#endif

namespace nil {
    namespace actor {

        static_assert(std::is_nothrow_default_constructible<net::ipv4_address>::value);
        static_assert(std::is_nothrow_copy_constructible<net::ipv4_address>::value);
        static_assert(std::is_nothrow_move_constructible<net::ipv4_address>::value);

        static_assert(std::is_nothrow_default_constructible<net::ipv6_address>::value);
        static_assert(std::is_nothrow_copy_constructible<net::ipv6_address>::value);
        static_assert(std::is_nothrow_move_constructible<net::ipv6_address>::value);

        static_assert(std::is_nothrow_default_constructible<net::inet_address>::value);
        static_assert(std::is_nothrow_copy_constructible<net::inet_address>::value);
        static_assert(std::is_nothrow_move_constructible<net::inet_address>::value);

        net::inet_address::inet_address() noexcept : inet_address(::in6_addr {}) {
        }

        net::inet_address::inet_address(family f) noexcept : _in_family(f) {
            memset(&_in6, 0, sizeof(_in6));
        }

        net::inet_address::inet_address(::in_addr i) noexcept : _in_family(family::INET), _in(i) {
        }

        net::inet_address::inet_address(::in6_addr i, uint32_t scope) noexcept :
            _in_family(family::INET6), _in6(i), _scope(scope) {
        }

        boost::optional<net::inet_address> net::inet_address::parse_numerical(const sstring &addr) {
            inet_address in;
            if (::inet_pton(AF_INET, addr.c_str(), &in._in)) {
                in._in_family = family::INET;
                return in;
            }
            auto i = addr.find_last_of('%');
            if (i != sstring::npos) {
                auto ext = addr.substr(i + 1);
                auto src = addr.substr(0, i);
                auto res = parse_numerical(src);

                if (res) {
                    uint32_t index = std::numeric_limits<uint32_t>::max();
                    try {
                        index = std::stoul(ext);
                    } catch (...) {
                    }
                    for (auto &nwif : engine().net().network_interfaces()) {
                        if (nwif.index() == index || nwif.name() == ext || nwif.display_name() == ext) {
                            res->_scope = nwif.index();
                            break;
                        }
                    }
                    return *res;
                }
            }
            if (::inet_pton(AF_INET6, addr.c_str(), &in._in6)) {
                in._in_family = family::INET6;
                return in;
            }
            return {};
        }

        net::inet_address::inet_address(const sstring &addr) :
            inet_address([&addr] {
                auto res = parse_numerical(addr);
                if (res) {
                    return std::move(*res);
                }
                throw std::invalid_argument(addr);
            }()) {
        }

        net::inet_address::inet_address(const ipv4_address &in) noexcept : inet_address(::in_addr {hton(in.ip)}) {
        }

        net::inet_address::inet_address(const ipv6_address &in, uint32_t scope) noexcept :
            inet_address(
                [&] {
                    ::in6_addr tmp;
                    std::copy(in.bytes().begin(), in.bytes().end(), tmp.s6_addr);
                    return tmp;
                }(),
                scope) {
        }

        net::ipv4_address net::inet_address::as_ipv4_address() const {
            in_addr in = *this;
            return ipv4_address(ntoh(in.s_addr));
        }

        net::ipv6_address net::inet_address::as_ipv6_address() const noexcept {
            in6_addr in6 = *this;
            return ipv6_address {in6};
        }

        bool net::inet_address::operator==(const inet_address &o) const noexcept {
            if (o._in_family != _in_family) {
                return false;
            }
            switch (_in_family) {
                case family::INET:
                    return _in.s_addr == o._in.s_addr;
                case family::INET6:
                    return std::equal(std::begin(_in6.s6_addr), std::end(_in6.s6_addr), std::begin(o._in6.s6_addr));
                default:
                    return false;
            }
        }

        net::inet_address::operator ::in_addr() const {
            if (_in_family != family::INET) {
                if (IN6_IS_ADDR_V4MAPPED(&_in6)) {
                    ::in_addr in;
                    in.s_addr = _in6.s6_addr32[3];
                    return in;
                }
                throw std::invalid_argument("Not an IPv4 address");
            }
            return _in;
        }

        net::inet_address::operator ::in6_addr() const noexcept {
            if (_in_family == family::INET) {
                in6_addr in6 = IN6ADDR_ANY_INIT;
                in6.s6_addr32[2] = htonl(0xffff);
                in6.s6_addr32[3] = _in.s_addr;
                return in6;
            }
            return _in6;
        }

        net::inet_address::operator net::ipv6_address() const noexcept {
            return as_ipv6_address();
        }

        size_t net::inet_address::size() const noexcept {
            switch (_in_family) {
                case family::INET:
                    return sizeof(::in_addr);
                case family::INET6:
                    return sizeof(::in6_addr);
                default:
                    return 0;
            }
        }

        const void *net::inet_address::data() const noexcept {
            return &_in;
        }

        net::ipv6_address::ipv6_address(const ::in6_addr &in) noexcept {
            std::copy(std::begin(in.s6_addr), std::end(in.s6_addr), ip.begin());
        }

        net::ipv6_address::ipv6_address(const ipv6_bytes &in) noexcept : ip(in) {
        }

        net::ipv6_address::ipv6_address(const ipv6_addr &addr) noexcept : ipv6_address(addr.ip) {
        }

        net::ipv6_address::ipv6_address() noexcept : ipv6_address(::in6addr_any) {
        }

        net::ipv6_address::ipv6_address(const std::string &addr) {
            if (!::inet_pton(AF_INET6, addr.c_str(), ip.data())) {
                throw std::runtime_error(
                    format("Wrong format for IPv6 address {}. Please ensure it's in colon-hex format", addr));
            }
        }

        net::ipv6_address net::ipv6_address::read(const char *s) noexcept {
            auto *b = reinterpret_cast<const uint8_t *>(s);
            ipv6_address in;
            std::copy(b, b + ipv6_address::size(), in.ip.begin());
            return in;
        }

        net::ipv6_address net::ipv6_address::consume(const char *&p) noexcept {
            auto res = read(p);
            p += size();
            return res;
        }

        void net::ipv6_address::write(char *p) const noexcept {
            std::copy(ip.begin(), ip.end(), p);
        }

        void net::ipv6_address::produce(char *&p) const noexcept {
            write(p);
            p += size();
        }

        bool net::ipv6_address::is_unspecified() const noexcept {
            return std::all_of(ip.begin(), ip.end(), [](uint8_t b) { return b == 0; });
        }

        std::ostream &net::operator<<(std::ostream &os, const ipv4_address &a) {
            auto ip = a.ip;
            return fmt_print(os, "{:d}.{:d}.{:d}.{:d}", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff,
                             (ip >> 0) & 0xff);
        }

        std::ostream &net::operator<<(std::ostream &os, const ipv6_address &a) {
            char buffer[64];
            return os << ::inet_ntop(AF_INET6, a.ip.data(), buffer, sizeof(buffer));
        }

        ipv6_addr::ipv6_addr(const ipv6_bytes &b, uint16_t p) noexcept : ip(b), port(p) {
        }

        ipv6_addr::ipv6_addr(uint16_t p) noexcept : ipv6_addr(net::inet_address(), p) {
        }

        ipv6_addr::ipv6_addr(const ::in6_addr &in6, uint16_t p) noexcept :
            ipv6_addr(net::ipv6_address(in6).bytes(), p) {
        }

        ipv6_addr::ipv6_addr(const std::string &s) :
            ipv6_addr([&] {
                auto lc = s.find_last_of(']');
                auto cp = s.find_first_of(':', lc);
                auto port = cp != std::string::npos ? std::stoul(s.substr(cp + 1)) : 0;
                auto ss = lc != std::string::npos ? s.substr(1, lc - 1) : s;
                return ipv6_addr(net::ipv6_address(ss).bytes(), uint16_t(port));
            }()) {
        }

        ipv6_addr::ipv6_addr(const std::string &s, uint16_t p) : ipv6_addr(net::ipv6_address(s).bytes(), p) {
        }

        ipv6_addr::ipv6_addr(const net::inet_address &i, uint16_t p) noexcept :
            ipv6_addr(i.as_ipv6_address().bytes(), p) {
        }

        ipv6_addr::ipv6_addr(const ::sockaddr_in6 &s) noexcept : ipv6_addr(s.sin6_addr, net::ntoh(s.sin6_port)) {
        }

        ipv6_addr::ipv6_addr(const socket_address &s) noexcept : ipv6_addr(s.as_posix_sockaddr_in6()) {
        }

        bool ipv6_addr::is_ip_unspecified() const noexcept {
            return std::all_of(ip.begin(), ip.end(), [](uint8_t b) { return b == 0; });
        }

        net::inet_address socket_address::addr() const noexcept {
            switch (family()) {
                case AF_INET:
                    return net::inet_address(as_posix_sockaddr_in().sin_addr);
                case AF_INET6:
                    return net::inet_address(as_posix_sockaddr_in6().sin6_addr, as_posix_sockaddr_in6().sin6_scope_id);
                default:
                    return net::inet_address();
            }
        }

        ::in_port_t socket_address::port() const noexcept {
            return net::ntoh(u.in.sin_port);
        }

        bool socket_address::is_wildcard() const noexcept {
            switch (family()) {
                case AF_INET: {
                    ipv4_addr addr(*this);
                    return addr.is_ip_unspecified() && addr.is_port_unspecified();
                }
                default:
                case AF_INET6: {
                    ipv6_addr addr(*this);
                    return addr.is_ip_unspecified() && addr.is_port_unspecified();
                }
                case AF_UNIX:
                    return length() <= sizeof(::sa_family_t);
            }
        }
        std::ostream &net::operator<<(std::ostream &os, const inet_address &addr) {
            char buffer[64];
            os << inet_ntop(int(addr.in_family()), addr.data(), buffer, sizeof(buffer));
            if (addr.scope() != inet_address::invalid_scope) {
                os << "%" << addr.scope();
            }
            return os;
        }

        std::ostream &net::operator<<(std::ostream &os, const inet_address::family &f) {
            switch (f) {
                case inet_address::family::INET:
                    os << "INET";
                    break;
                case inet_address::family::INET6:
                    os << "INET6";
                    break;
                default:
                    break;
            }
            return os;
        }
    }    // namespace actor
}    // namespace nil

std::ostream &operator<<(std::ostream &os, const nil::actor::ipv4_addr &a) {
    return os << nil::actor::socket_address(a);
}

std::ostream &operator<<(std::ostream &os, const nil::actor::ipv6_addr &a) {
    return os << nil::actor::socket_address(a);
}

size_t std::hash<nil::actor::net::inet_address>::operator()(const nil::actor::net::inet_address &a) const {
    switch (a.in_family()) {
        case nil::actor::net::inet_address::family::INET:
            return std::hash<nil::actor::net::ipv4_address>()(a.as_ipv4_address());
        case nil::actor::net::inet_address::family::INET6:
            return std::hash<nil::actor::net::ipv6_address>()(a.as_ipv6_address());
        default:
            return 0;
    }
}

size_t std::hash<nil::actor::net::ipv6_address>::operator()(const nil::actor::net::ipv6_address &a) const {
    return boost::hash_range(a.ip.begin(), a.ip.end());
}

size_t std::hash<nil::actor::ipv4_addr>::operator()(const nil::actor::ipv4_addr &x) const {
    size_t h = x.ip;
    boost::hash_combine(h, x.port);
    return h;
}
