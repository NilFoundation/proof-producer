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

#include <nil/actor/http/api_docs.hh>
#include <nil/actor/http/handlers.hh>
#include <nil/actor/json/formatter.hh>
#include <nil/actor/http/transformers.hh>
#include <nil/actor/core/fstream.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/http/transformers.hh>
#include <nil/actor/core/loop.hh>

using namespace std;

namespace nil {
    namespace actor {

        namespace httpd {

            const sstring api_registry_builder_base::DEFAULT_PATH = "/api-doc";
            const sstring api_registry_builder_base::DEFAULT_DIR = ".";

            doc_entry get_file_reader(sstring file_name) {
                return [file_name](output_stream<char> &os) {
                    return open_file_dma(file_name, open_flags::ro).then([&os](file f) mutable {
                        return do_with(
                            input_stream<char>(make_file_input_stream(std::move(f))),
                            [&os](input_stream<char> &is) { return copy(is, os).then([&is] { return is.close(); }); });
                    });
                };
            }

            future<> api_docs_20::write(output_stream<char> &&os, std::unique_ptr<request> req) {
                return do_with(output_stream<char>(_transform.transform(std::move(req), "", std::move(os))),
                               [this](output_stream<char> &os) {
                                   return do_for_each(_apis, [&os](doc_entry &api) { return api(os); })
                                       .then([&os] { return os.write("},\"definitions\": {"); })
                                       .then([this, &os] {
                                           return do_for_each(_definitions, [&os](doc_entry &api) { return api(os); });
                                       })
                                       .then([&os] { return os.write("}}"); })
                                       .then([&os] { return os.flush(); })
                                       .finally([&os] { return os.close(); });
                               });
            }

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
