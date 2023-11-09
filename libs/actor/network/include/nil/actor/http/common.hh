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

#include <unordered_map>

#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            class parameters {
                std::unordered_map<sstring, sstring> params;

            public:
                const sstring &path(const sstring &key) const {
                    return params.at(key);
                }

                sstring operator[](const sstring &key) const {
                    return params.at(key).substr(1);
                }

                const sstring &at(const sstring &key) const {
                    return path(key);
                }

                bool exists(const sstring &key) const {
                    return params.find(key) != params.end();
                }

                void set(const sstring &key, const sstring &value) {
                    params[key] = value;
                }

                void clear() {
                    params.clear();
                }
            };

            enum operation_type { GET, POST, PUT, DELETE, HEAD, OPTIONS, TRACE, CONNECT, NUM_OPERATION };

            /**
             * Translate the string command to operation type
             * @param type the string "GET" or "POST"
             * @return the operation_type
             */
            operation_type str2type(const sstring &type);

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
