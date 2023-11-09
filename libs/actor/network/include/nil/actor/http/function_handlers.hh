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

#include <functional>

#include <nil/actor/http/handlers.hh>
#include <nil/actor/json/json_elements.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * A request function is a lambda expression that gets only the request
             * as its parameter
             */
            typedef std::function<sstring(const_req req)> request_function;

            /**
             * A handle function is a lambda expression that gets request and reply
             */
            typedef std::function<sstring(const_req req, reply &)> handle_function;

            /**
             * A json request function is a lambda expression that gets only the request
             * as its parameter and return a json response.
             * Using the json response is done implicitly.
             */
            typedef std::function<json::json_return_type(const_req req)> json_request_function;

            /**
             * A future_json_function is a function that returns a future json reponse.
             * Similiar to the json_request_function, using the json reponse is done
             * implicitly.
             */
            typedef std::function<future<json::json_return_type>(std::unique_ptr<request> req)> future_json_function;

            typedef std::function<future<std::unique_ptr<reply>>(std::unique_ptr<request> req,
                                                                 std::unique_ptr<reply> rep)>
                future_handler_function;
            /**
             * The function handler get a lambda expression in the constructor.
             * it will call that expression to get the result
             * This is suited for very simple handlers
             *
             */
            class function_handler : public handler_base {
            public:
                function_handler(const handle_function &f_handle, const sstring &type) :
                    _f_handle([f_handle](std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
                        rep->_content += f_handle(*req.get(), *rep.get());
                        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                    }),
                    _type(type) {
                }

                function_handler(const future_handler_function &f_handle, const sstring &type) :
                    _f_handle(f_handle), _type(type) {
                }

                function_handler(const request_function &_handle, const sstring &type) :
                    _f_handle([_handle](std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
                        rep->_content += _handle(*req.get());
                        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                    }),
                    _type(type) {
                }

                function_handler(const json_request_function &_handle) :
                    _f_handle([_handle](std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
                        json::json_return_type res = _handle(*req.get());
                        rep->_content += res._res;
                        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                    }),
                    _type("json") {
                }

                function_handler(const future_json_function &_handle) :
                    _f_handle([_handle](std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
                        return _handle(std::move(req))
                            .then([rep = std::move(rep)](json::json_return_type &&res) mutable {
                                if (res._body_writer) {
                                    rep->write_body("json", std::move(res._body_writer));
                                } else {
                                    rep->_content += res._res;
                                }
                                return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                            });
                    }),
                    _type("json") {
                }

                function_handler(const function_handler &) = default;

                future<std::unique_ptr<reply>> handle(const sstring &path, std::unique_ptr<request> req,
                                                      std::unique_ptr<reply> rep) override {
                    return _f_handle(std::move(req), std::move(rep)).then([this](std::unique_ptr<reply> rep) {
                        rep->done(_type);
                        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                    });
                }

            protected:
                future_handler_function _f_handle;
                sstring _type;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
