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

#include <nil/actor/http/matcher.hh>

#include <iostream>

namespace nil {
    namespace actor {

        namespace httpd {

            using namespace std;

            /**
             * Search for the end of the url parameter.
             * @param url the url to search
             * @param ind the position in the url
             * @param entire_path when set to true, take all the reminaing url
             * when set to false, search for the next slash
             * @return the position in the url of the end of the parameter
             */
            static size_t find_end_param(const sstring &url, size_t ind, bool entire_path) {
                size_t pos = (entire_path) ? url.length() : url.find('/', ind + 1);
                if (pos == sstring::npos) {
                    return url.length();
                }
                return pos;
            }

            size_t param_matcher::match(const sstring &url, size_t ind, parameters &param) {
                size_t last = find_end_param(url, ind, _entire_path);
                if (last == ind) {
                    /*
                     * empty parameter allows only for the case of entire_path
                     */
                    if (_entire_path) {
                        param.set(_name, "");
                        return ind;
                    }
                    return sstring::npos;
                }
                param.set(_name, url.substr(ind, last - ind));
                return last;
            }

            size_t str_matcher::match(const sstring &url, size_t ind, parameters &param) {
                if (url.length() >= _len + ind && (url.find(_cmp, ind) == ind) &&
                    (url.length() == _len + ind || url.at(_len + ind) == '/')) {
                    return _len + ind;
                }
                return sstring::npos;
            }

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
