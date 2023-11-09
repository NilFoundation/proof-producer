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

#include <nil/actor/http/json_path.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            using namespace std;

            void path_description::set(routes &_routes, handler_base *handler) const {
                for (auto &i : mandatory_queryparams) {
                    handler->mandatory(i);
                }

                if (params.size() == 0)
                    _routes.put(operations.method, path, handler);
                else {
                    match_rule *rule = new match_rule(handler);
                    rule->add_str(path);
                    for (auto &&i : params) {
                        if (i.type == url_component_type::FIXED_STRING) {
                            rule->add_str(i.name);
                        } else {
                            rule->add_param(i.name, i.type == url_component_type::PARAM_UNTIL_END_OF_PATH);
                        }
                    }
                    _cookie = _routes.add_cookie(rule, operations.method);
                }
            }

            void path_description::set(routes &_routes, const json_request_function &f) const {
                set(_routes, new function_handler(f));
            }

            void path_description::set(routes &_routes, const future_json_function &f) const {
                set(_routes, new function_handler(f));
            }

            void path_description::unset(routes &_routes) const {
                if (params.size() == 0) {
                    _routes.drop(operations.method, path);
                } else {
                    auto rule = _routes.del_cookie(_cookie, operations.method);
                    delete rule;
                }
            }

            path_description::path_description(const sstring &path, operation_type method, const sstring &nickname,
                                               const std::vector<std::pair<sstring, bool>> &path_parameters,
                                               const std::vector<sstring> &mandatory_params) :
                path(path),
                operations(method, nickname) {

                for (auto man : mandatory_params) {
                    pushmandatory_param(man);
                }
                for (auto param : path_parameters) {
                    pushparam(param.first, param.second);
                }
            }

            path_description::path_description(const sstring &path, operation_type method, const sstring &nickname,
                                               const std::initializer_list<path_part> &path_parameters,
                                               const std::vector<sstring> &mandatory_params) :
                path(path),
                operations(method, nickname) {

                for (auto man : mandatory_params) {
                    pushmandatory_param(man);
                }
                for (auto param : path_parameters) {
                    params.push_back(param);
                }
            }

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
