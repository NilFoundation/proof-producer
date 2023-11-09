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

#include <vector>
#include <unordered_map>
#include <tuple>

#include <nil/actor/http/common.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/http/routes.hh>
#include <nil/actor/http/function_handlers.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * A json_operation contain a method and a nickname.
             * operation are associated to a path, that can
             * have multiple methods
             */
            struct json_operation {
                /**
                 * default constructor
                 */
                json_operation() : method(GET) {
                }

                /**
                 * Construct with assignment
                 * @param method the http method type
                 * @param nickname the http nickname
                 */
                json_operation(operation_type method, const sstring &nickname) : method(method), nickname(nickname) {
                }

                operation_type method;
                sstring nickname;
            };

            /**
             * path description holds the path in the system.
             * It maps a nickname to an operation, which allows
             * defining the operation (path and method) by its
             * nickname.
             *
             * A path_description has a type, a base path and a list of
             * url components.
             * Each component can be a regular path parameter, a path parameter that
             * contains everything until the end of the path or a fixed string.
             *
             * the description are taken from the json swagger
             * definition file, during auto code generation in the
             * compilation.
             */
            struct path_description {
                //
                enum class url_component_type {
                    PARAM,                      // a normal path parameter (starts with / and end with / or end of path)
                    PARAM_UNTIL_END_OF_PATH,    // a parameter that contains all the path entil its end
                    FIXED_STRING,    // a fixed string inside the path, must be a full match and does not count
                                     // as a parameter
                };

                // path_part is either a parameter or a fixed string
                struct path_part {
                    sstring name;
                    url_component_type type = url_component_type::PARAM;
                };

                /**
                 * default empty constructor
                 */
                path_description() = default;

                /**
                 * constructor for path with parameters
                 * The constructor is used by
                 * @param path the url path
                 * @param method the http method
                 * @param nickname the nickname
                 * @param path_parameters path parameters and url parts of the path
                 * @param mandatory_params the names of the mandatory query parameters
                 */
                path_description(const sstring &path, operation_type method, const sstring &nickname,
                                 const std::vector<std::pair<sstring, bool>> &path_parameters,
                                 const std::vector<sstring> &mandatory_params);

                /**
                 * constructor for path with parameters
                 * The constructor is used by
                 * @param path the url path
                 * @param method the http method
                 * @param nickname the method nickname
                 * @param path_parameters path parameters and url parts of the path
                 * @param mandatory_params the names of the mandatory query parameters
                 */
                path_description(const sstring &path, operation_type method, const sstring &nickname,
                                 const std::initializer_list<path_part> &path_parameters,
                                 const std::vector<sstring> &mandatory_params);

                /**
                 * Add a parameter to the path definition
                 * for example, if the url should match /file/{path}
                 * The constructor would be followed by a call to
                 * pushparam("path")
                 *
                 * @param param the name of the parameters, this name will
                 * be used by the handler to identify the parameters.
                 * A name can appear at most once in a description
                 * @param all_path when set to true the parameter will assume to match
                 * until the end of the url.
                 * This is useful for situation like file path with
                 * a rule like /file/{path} and a url /file/etc/hosts.
                 * path should be equal to /ets/hosts and not only /etc
                 * @return the current path description
                 */
                path_description *pushparam(const sstring &param, bool all_path = false) {
                    params.push_back(
                        {param, (all_path) ? url_component_type::PARAM_UNTIL_END_OF_PATH : url_component_type::PARAM});
                    return this;
                }

                /*!
                 * \brief adds a fixed string as part of the path
                 * This will allow to combine fixed string URL parts and path parameters.
                 *
                 * For example to map a path like:
                 * /mypath/{param1}/morepath/{param2}
                 * path_description p("/mypath", operation_type::GET);
                 * p.pushparam("param1)->pushurl("morepath")->pushparam("param2");
                 */
                path_description *push_static_path_part(const sstring &url) {
                    params.push_back({url, url_component_type::FIXED_STRING});
                    return this;
                }
                /**
                 * adds a mandatory query parameter to the path
                 * this parameter will be check before calling a handler
                 * @param param the parameter to head
                 * @return a pointer to the current path description
                 */
                path_description *pushmandatory_param(const sstring &param) {
                    mandatory_queryparams.push_back(param);
                    return this;
                }

                std::vector<path_part> params;
                sstring path;
                json_operation operations;
                mutable routes::rule_cookie _cookie;

                std::vector<sstring> mandatory_queryparams;

                void set(routes &_routes, handler_base *handler) const;

                void set(routes &_routes, const json_request_function &f) const;

                void set(routes &_routes, const future_json_function &f) const;

                void unset(routes &_routes) const;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
