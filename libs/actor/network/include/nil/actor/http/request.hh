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
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <string>
#include <vector>
#include <strings.h>

#include <nil/actor/core/sstring.hh>
#include <nil/actor/http/common.hh>

namespace nil {
    namespace actor {

        namespace httpd {
            class connection;

            /**
             * A request received from a client.
             */
            struct request {
                enum class ctclass : char {
                    other,
                    multipart,
                    app_x_www_urlencoded,
                };

                struct case_insensitive_cmp {
                    bool operator()(const sstring &s1, const sstring &s2) const {
                        return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(),
                                          [](char a, char b) { return ::tolower(a) == ::tolower(b); });
                    }
                };

                struct case_insensitive_hash {
                    size_t operator()(sstring s) const {
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        return std::hash<sstring>()(s);
                    }
                };

                sstring _method;
                sstring _url;
                sstring _version;
                int http_version_major;
                int http_version_minor;
                ctclass content_type_class;
                size_t content_length = 0;
                std::unordered_map<sstring, sstring, case_insensitive_hash, case_insensitive_cmp> _headers;
                std::unordered_map<sstring, sstring> query_parameters;
                connection *connection_ptr;
                parameters param;
                sstring content;
                sstring protocol_name = "http";

                /**
                 * Search for the first header of a given name
                 * @param name the header name
                 * @return a pointer to the header value, if it exists or empty string
                 */
                sstring get_header(const sstring &name) const {
                    auto res = _headers.find(name);
                    if (res == _headers.end()) {
                        return "";
                    }
                    return res->second;
                }

                /**
                 * Search for the first header of a given name
                 * @param name the header name
                 * @return a pointer to the header value, if it exists or empty string
                 */
                sstring get_query_param(const sstring &name) const {
                    auto res = query_parameters.find(name);
                    if (res == query_parameters.end()) {
                        return "";
                    }
                    return res->second;
                }

                /**
                 * Get the request protocol name. Can be either "http" or "https".
                 */
                sstring get_protocol_name() const {
                    return protocol_name;
                }

                /**
                 * Get the request url.
                 * @return the request url
                 */
                sstring get_url() const {
                    return get_protocol_name() + "://" + get_header("Host") + _url;
                }

                bool is_multi_part() const {
                    return content_type_class == ctclass::multipart;
                }

                bool is_form_post() const {
                    return content_type_class == ctclass::app_x_www_urlencoded;
                }
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
