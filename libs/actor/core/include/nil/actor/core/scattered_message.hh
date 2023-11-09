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

#pragma once

#include <nil/actor/core/deleter.hh>
#include <nil/actor/core/temporary_buffer.hh>
#include <nil/actor/network/packet.hh>
#include <nil/actor/core/sstring.hh>
#include <memory>
#include <vector>
#include <nil/actor/detail/std-compat.hh>

namespace nil {
    namespace actor {

        template<typename CharType>
        class scattered_message {
        private:
            using fragment = net::fragment;
            using packet = net::packet;
            using char_type = CharType;
            packet _p;

        public:
            scattered_message() {
            }
            scattered_message(scattered_message &&) = default;
            scattered_message(const scattered_message &) = delete;

            void append_static(const char_type *buf, size_t size) {
                if (size) {
                    _p = packet(std::move(_p), fragment {(char_type *)buf, size}, deleter());
                }
            }

            template<size_t N>
            void append_static(const char_type (&s)[N]) {
                append_static(s, N - 1);
            }

            void append_static(const char_type *s) {
                append_static(s, strlen(s));
            }

            template<typename size_type, size_type max_size>
            void append_static(const basic_sstring<char_type, size_type, max_size> &s) {
                append_static(s.begin(), s.size());
            }

            void append_static(const std::string_view &s) {
                append_static(s.data(), s.size());
            }

            void append(std::string_view v) {
                if (v.size()) {
                    _p = packet(std::move(_p), temporary_buffer<char>::copy_of(v));
                }
            }

            template<typename size_type, size_type max_size>
            void append(basic_sstring<char_type, size_type, max_size> s) {
                if (s.size()) {
                    _p = packet(std::move(_p), std::move(s).release());
                }
            }

            template<typename size_type, size_type max_size, typename Callback>
            void append(const basic_sstring<char_type, size_type, max_size> &s, Callback callback) {
                if (s.size()) {
                    _p = packet(std::move(_p), fragment {s.begin(), s.size()}, make_deleter(std::move(callback)));
                }
            }

            void reserve(int n_frags) {
                _p.reserve(n_frags);
            }

            packet release() && {
                return std::move(_p);
            }

            template<typename Callback>
            void on_delete(Callback callback) {
                _p = packet(std::move(_p), make_deleter(std::move(callback)));
            }

            operator bool() const {
                return _p.len();
            }

            size_t size() {
                return _p.len();
            }
        };

    }    // namespace actor
}    // namespace nil
