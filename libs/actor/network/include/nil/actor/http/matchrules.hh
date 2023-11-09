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

#include <nil/actor/http/handlers.hh>
#include <nil/actor/http/matcher.hh>
#include <nil/actor/http/common.hh>

#include <nil/actor/core/sstring.hh>

#include <vector>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * match_rule check if a url matches criteria, that can contains
             * parameters.
             * the routes object would call the get method with a url and if
             * it matches, the method will return a handler
             * during the matching process, the method fill the parameters object.
             */
            class match_rule {
            public:
                /**
                 * The destructor deletes matchers.
                 */
                ~match_rule() {
                    for (auto m : _match_list) {
                        delete m;
                    }
                    delete _handler;
                }

                /**
                 * Constructor with a handler
                 * @param handler the handler to return when this match rule is met
                 */
                explicit match_rule(handler_base *handler) : _handler(handler) {
                }

                /**
                 * Check if url match the rule and return a handler if it does
                 * @param url a url to compare against the rule
                 * @param params the parameters object, matches parameters will fill
                 * the object during the matching process
                 * @return a handler if there is a full match or nullptr if not
                 */
                handler_base *get(const sstring &url, parameters &params) {
                    size_t ind = 0;
                    if (_match_list.empty()) {
                        return _handler;
                    }
                    for (unsigned int i = 0; i < _match_list.size(); i++) {
                        ind = _match_list.at(i)->match(url, ind, params);
                        if (ind == sstring::npos) {
                            return nullptr;
                        }
                    }
                    return (ind + 1 >= url.length()) ? _handler : nullptr;
                }

                /**
                 * Add a matcher to the rule
                 * @param match the matcher to add
                 * @return this
                 */
                match_rule &add_matcher(matcher *match) {
                    _match_list.push_back(match);
                    return *this;
                }

                /**
                 * Add a static url matcher
                 * @param str the string to search for
                 * @return this
                 */
                match_rule &add_str(const sstring &str) {
                    add_matcher(new str_matcher(str));
                    return *this;
                }

                /**
                 * add a parameter matcher to the rule
                 * @param str the parameter name
                 * @param fullpath when set to true, parameter will included all the
                 * remaining url until its end
                 * @return this
                 */
                match_rule &add_param(const sstring &str, bool fullpath = false) {
                    add_matcher(new param_matcher(str, fullpath));
                    return *this;
                }

            private:
                std::vector<matcher *> _match_list;
                handler_base *_handler;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
