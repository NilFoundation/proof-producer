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
#include <nil/actor/http/reply.hh>
#include <nil/actor/json/json_elements.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * The base_exception is a base for all http exception.
             * It contains a message that will be return as the message content
             * and a status that will be return as a status code.
             */
            class base_exception : public std::exception {
            public:
                base_exception(const std::string &msg, reply::status_type status) : _msg(msg), _status(status) {
                }

                virtual const char *what() const throw() {
                    return _msg.c_str();
                }

                reply::status_type status() const {
                    return _status;
                }

                virtual const std::string &str() const {
                    return _msg;
                }

            private:
                std::string _msg;
                reply::status_type _status;
            };

            /**
             * Throwing this exception will result in a redirect to the given url
             */
            class redirect_exception : public base_exception {
            public:
                redirect_exception(const std::string &url) :
                    base_exception("", reply::status_type::moved_permanently), url(url) {
                }
                std::string url;
            };

            /**
             * Throwing this exception will result in a 404 not found result
             */
            class not_found_exception : public base_exception {
            public:
                not_found_exception(const std::string &msg = "Not found") :
                    base_exception(msg, reply::status_type::not_found) {
                }
            };

            /**
             * Throwing this exception will result in a 400 bad request result
             */

            class bad_request_exception : public base_exception {
            public:
                bad_request_exception(const std::string &msg) : base_exception(msg, reply::status_type::bad_request) {
                }
            };

            class bad_param_exception : public bad_request_exception {
            public:
                bad_param_exception(const std::string &msg) : bad_request_exception(msg) {
                }
            };

            class missing_param_exception : public bad_request_exception {
            public:
                missing_param_exception(const std::string &param) :
                    bad_request_exception(std::string("Missing mandatory parameter '") + param + "'") {
                }
            };

            class server_error_exception : public base_exception {
            public:
                server_error_exception(const std::string &msg) :
                    base_exception(msg, reply::status_type::internal_server_error) {
                }
            };

            class json_exception : public json::json_base {
            public:
                json::json_element<std::string> _msg;
                json::json_element<int> _code;
                void register_params() {
                    add(&_msg, "message");
                    add(&_code, "code");
                }

                json_exception(const base_exception &e) {
                    set(e.str(), e.status());
                }

                json_exception(std::exception_ptr e) {
                    std::ostringstream exception_description;
                    exception_description << e;
                    set(exception_description.str(), reply::status_type::internal_server_error);
                }

            private:
                void set(const std::string &msg, reply::status_type code) {
                    register_params();
                    _msg = msg;
                    _code = (int)code;
                }
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
