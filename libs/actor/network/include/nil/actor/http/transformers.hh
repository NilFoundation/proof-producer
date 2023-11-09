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
#include <nil/actor/http/file_handler.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            /**
             * content_replace replaces variable in a file with a dynamic value.
             * It would take the host from request and will replace the variable
             * in a file
             *
             * The replacement can be restricted to an extension.
             *
             * We are currently support only one file type for replacement.
             * It could be extend if we will need it
             *
             */
            class content_replace : public file_transformer {
            public:
                virtual output_stream<char> transform(std::unique_ptr<request> req, const sstring &extension,
                                                      output_stream<char> &&s);
                /**
                 * the constructor get the file extension the replace would work on.
                 * @param extension file extension, when not set all files extension
                 */
                explicit content_replace(const sstring &extension = "") : extension(extension) {
                }

            private:
                sstring extension;
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
