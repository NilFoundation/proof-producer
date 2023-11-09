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

#include <nil/actor/http/request.hh>
#include <nil/actor/http/common.hh>
#include <nil/actor/http/reply.hh>

#include <unordered_map>

namespace nil {
    namespace actor {

        namespace httpd {

            typedef const httpd::request &const_req;

            /**
             * handlers holds the logic for serving an incoming request.
             * All handlers inherit from the base httpserver_handler and
             * implement the handle method.
             *
             */
            class handler_base {
            public:
                /**
                 * All handlers should implement this method.
                 *  It fill the reply according to the request.
                 * @param path the url path used in this call
                 * @param req the original request
                 * @param rep the reply
                 */
                virtual future<std::unique_ptr<reply>> handle(const sstring &path, std::unique_ptr<request> req,
                                                              std::unique_ptr<reply> rep) = 0;

                virtual ~handler_base() = default;

                /**
                 * Add a mandatory parameter
                 * @param param a parameter name
                 * @return a reference to the handler
                 */
                handler_base &mandatory(const sstring &param) {
                    _mandatory_param.push_back(param);
                    return *this;
                }

                std::vector<sstring> _mandatory_param;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil

