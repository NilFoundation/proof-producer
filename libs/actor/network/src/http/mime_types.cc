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

#include <nil/actor/http/mime_types.hh>

namespace nil {
    namespace actor {

        namespace httpd {
            namespace mime_types {

                struct mapping {
                    const char *extension;
                    const char *mime_type;
                } mappings[] = {
                    {"json", "application/json"},
                    {"gif", "image/gif"},
                    {"htm", "text/html"},
                    {"css", "text/css"},
                    {"js", "text/javascript"},
                    {"html", "text/html"},
                    {"jpg", "image/jpeg"},
                    {"png", "image/png"},
                    {"txt", "text/plain"},
                    {"ico", "image/x-icon"},
                    {"bin", "application/octet-stream"},
                    {"proto",
                     "application/vnd.google.protobuf; proto=io.prometheus.client.MetricFamily; encoding=delimited"},
                };

                const char *extension_to_type(const sstring &extension) {
                    for (mapping m : mappings) {
                        if (extension == m.extension) {
                            return m.mime_type;
                        }
                    }
                    return "text/plain";
                }

            }    // namespace mime_types

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
