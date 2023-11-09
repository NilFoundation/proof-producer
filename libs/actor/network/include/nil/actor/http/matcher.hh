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

#include <nil/actor/http/common.hh>

#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * a base class for the url matching.
             * Each implementation check if the given url matches a criteria
             */
            class matcher {
            public:
                virtual ~matcher() = default;

                /**
                 * check if the given url matches the rule
                 * @param url the url to check
                 * @param ind the position to start from
                 * @param param fill the parameters hash
                 * @return the end of of the matched part, or sstring::npos if not matched
                 */
                virtual size_t match(const sstring &url, size_t ind, parameters &param) = 0;
            };

            /**
             * Check if the url match a parameter and fill the parameters object
             *
             * Note that a non empty url will always return true with the parameters
             * object filled
             *
             * Assume that the rule is /file/{path}/ and the param_matcher identify
             * the /{path}
             *
             * For all non empty values, match will return true.
             * If the entire url is /file/etc/hosts, and the part that is passed to
             * param_matcher is /etc/hosts, if entire_path is true, the match will be
             * '/etc/hosts' If entire_path is false, the match will be '/etc'
             */
            class param_matcher : public matcher {
            public:
                /**
                 * Constructor
                 * @param name the name of the parameter, will be used as the key
                 * in the parameters object
                 * @param entire_path when set to true, the matched parameters will
                 * include all the remaining url until the end of it.
                 * when set to false the match will terminate at the next slash
                 */
                explicit param_matcher(const sstring &name, bool entire_path = false) :
                    _name(name), _entire_path(entire_path) {
                }

                virtual size_t match(const sstring &url, size_t ind, parameters &param) override;

            private:
                sstring _name;
                bool _entire_path;
            };

            /**
             * Check if the url match a predefine string.
             *
             * When parsing a match rule such as '/file/{path}' the str_match would parse
             * the '/file' part
             */
            class str_matcher : public matcher {
            public:
                /**
                 * Constructor
                 * @param cmp the string to match
                 */
                explicit str_matcher(const sstring &cmp) : _cmp(cmp), _len(cmp.size()) {
                }

                virtual size_t match(const sstring &url, size_t ind, parameters &param) override;

            private:
                sstring _cmp;
                unsigned _len;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
