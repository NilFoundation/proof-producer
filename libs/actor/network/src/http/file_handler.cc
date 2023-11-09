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

#include <algorithm>
#include <iostream>

#include <nil/actor/http/file_handler.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/fstream.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/http/exception.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            directory_handler::directory_handler(const sstring &doc_root, file_transformer *transformer) :
                file_interaction_handler(transformer), doc_root(doc_root) {
            }

            future<std::unique_ptr<reply>> directory_handler::handle(const sstring &path, std::unique_ptr<request> req,
                                                                     std::unique_ptr<reply> rep) {
                sstring full_path = doc_root + req->param["path"];
                auto h = this;
                return engine().file_type(full_path).then(
                    [h, full_path, req = std::move(req), rep = std::move(rep)](auto val) mutable {
                        if (val) {
                            if (val.value() == directory_entry_type::directory) {
                                if (h->redirect_if_needed(*req.get(), *rep.get())) {
                                    return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                                }
                                full_path += "/index.html";
                            }
                            return h->read(full_path, std::move(req), std::move(rep));
                        }
                        rep->set_status(reply::status_type::not_found).done();
                        return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                    });
            }

            file_interaction_handler::~file_interaction_handler() {
                delete transformer;
            }

            sstring file_interaction_handler::get_extension(const sstring &file) {
                size_t last_slash_pos = file.find_last_of('/');
                size_t last_dot_pos = file.find_last_of('.');
                sstring extension;
                if (last_dot_pos != sstring::npos && last_dot_pos > last_slash_pos) {
                    extension = file.substr(last_dot_pos + 1);
                }
                return extension;
            }

            output_stream<char> file_interaction_handler::get_stream(std::unique_ptr<request> req,
                                                                     const sstring &extension,
                                                                     output_stream<char> &&s) {
                if (transformer) {
                    return transformer->transform(std::move(req), extension, std::move(s));
                }
                return std::move(s);
            }

            future<std::unique_ptr<reply>> file_interaction_handler::read(sstring file_name,
                                                                          std::unique_ptr<request> req,
                                                                          std::unique_ptr<reply> rep) {
                sstring extension = get_extension(file_name);
                rep->write_body(extension, [req = std::move(req), extension, file_name,
                                            this](output_stream<char> &&s) mutable {
                    return do_with(output_stream<char>(get_stream(std::move(req), extension, std::move(s))),
                                   [file_name](output_stream<char> &os) {
                                       return open_file_dma(file_name, open_flags::ro).then([&os](file f) {
                                           return do_with(
                                               input_stream<char>(make_file_input_stream(std::move(f))),
                                               [&os](input_stream<char> &is) {
                                                   return copy(is, os).then([&os] { return os.close(); }).then([&is] {
                                                       return is.close();
                                                   });
                                               });
                                       });
                                   });
                });
                return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
            }

            bool file_interaction_handler::redirect_if_needed(const request &req, reply &rep) const {
                if (req._url.length() == 0 || req._url.back() != '/') {
                    rep.set_status(reply::status_type::moved_permanently);
                    rep._headers["Location"] = req.get_url() + "/";
                    rep.done();
                    return true;
                }
                return false;
            }

            future<std::unique_ptr<reply>> file_handler::handle(const sstring &path, std::unique_ptr<request> req,
                                                                std::unique_ptr<reply> rep) {
                if (force_path && redirect_if_needed(*req.get(), *rep.get())) {
                    return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
                }
                return read(file, std::move(req), std::move(rep));
            }

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
