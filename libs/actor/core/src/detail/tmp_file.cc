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

#include <iostream>
#include <random>

#include <nil/actor/core/core.hh>

#include <nil/actor/detail/file.hh>
#include <nil/actor/detail/exceptions.hh>
#include <nil/actor/detail/tmp_file.hh>

namespace nil {
    namespace actor {

        namespace fs = boost::filesystem;

        static constexpr const char *default_tmp_name_template = "XXXXXX.tmp";

        static boost::filesystem::path generate_tmp_name(const boost::filesystem::path &path_template) {
            boost::filesystem::path parent = path_template.parent_path();
            std::string filename = path_template.filename().native();
            if (parent.empty()) {
                parent = ".";
            }
            auto pos = filename.find("XX");
            if (pos == std::string::npos) {
                parent = path_template;
                filename = default_tmp_name_template;
                pos = filename.find("XX");
                assert(pos != std::string::npos);
            }
            auto end = filename.size();
            static constexpr char charset[] = "0123456789abcdef";
            static thread_local std::default_random_engine engine(std::random_device {}());
            static thread_local std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
            while (pos < end && filename[pos] == 'X') {
                filename[pos++] = charset[dist(engine)];
            }
            parent /= filename;
            return parent;
        }

        static boost::filesystem::path default_tmpdir_path;

        const boost::filesystem::path &default_tmpdir() {
            if (default_tmpdir_path.empty()) {
                auto TMPDIR = getenv("TMPDIR");
                default_tmpdir_path = TMPDIR ? TMPDIR : "/tmp";
            }
            return default_tmpdir_path;
        }

        void set_default_tmpdir(boost::filesystem::path path) {
            default_tmpdir_path = std::move(path);
        }

        tmp_file::tmp_file(tmp_file &&x) noexcept : _path(std::move(x._path)), _file(std::move(x._file)) {
            std::swap(_is_open, x._is_open);
        }

        tmp_file::~tmp_file() {
            assert(!has_path());
            assert(!is_open());
        }

        future<> tmp_file::open(boost::filesystem::path path_template,
                                open_flags oflags,
                                file_open_options options) noexcept {
            assert(!has_path());
            assert(!is_open());
            oflags |= open_flags::create | open_flags::exclusive;
            boost::filesystem::path path;
            try {
                path = generate_tmp_name(std::move(path_template));
            } catch (...) {
                return current_exception_as_future();
            }
            return open_file_dma(path.native(), oflags, std::move(options))
                .then([this, path = std::move(path)](file f) mutable {
                    _path = std::move(path);
                    _file = std::move(f);
                    _is_open = true;
                    return make_ready_future<>();
                });
        }

        future<> tmp_file::close() noexcept {
            if (!is_open()) {
                return make_ready_future<>();
            }
            return _file.close().then([this] { _is_open = false; });
        }

        future<> tmp_file::remove() noexcept {
            if (!has_path()) {
                return make_ready_future<>();
            }
            return remove_file(get_path().native()).then([this] { _path.clear(); });
        }

        future<tmp_file> make_tmp_file(boost::filesystem::path path_template,
                                       open_flags oflags,
                                       file_open_options options) noexcept {
            return do_with(
                tmp_file(),
                [path_template = std::move(path_template), oflags, options = std::move(options)](tmp_file &t) mutable {
                    return t.open(std::move(path_template), oflags, std::move(options)).then([&t] {
                        return make_ready_future<tmp_file>(std::move(t));
                    });
                });
        }

        tmp_dir::~tmp_dir() {
            assert(!has_path());
        }

        future<> tmp_dir::create(boost::filesystem::path path_template, file_permissions create_permissions) noexcept {
            assert(!has_path());
            boost::filesystem::path path;
            try {
                path = generate_tmp_name(std::move(path_template));
            } catch (...) {
                return current_exception_as_future();
            }
            return touch_directory(path.native(), create_permissions).then([this, path = std::move(path)]() mutable {
                _path = std::move(path);
                return make_ready_future<>();
            });
        }

        future<> tmp_dir::remove() noexcept {
            if (!has_path()) {
                return make_ready_future<>();
            }
            return recursive_remove_directory(std::move(_path));
        }

        future<tmp_dir> make_tmp_dir(boost::filesystem::path path_template,
                                     file_permissions create_permissions) noexcept {
            return do_with(tmp_dir(),
                           [path_template = std::move(path_template), create_permissions](tmp_dir &t) mutable {
                               return t.create(std::move(path_template), create_permissions).then([&t]() mutable {
                                   return make_ready_future<tmp_dir>(std::move(t));
                               });
                           });
        }

    }    // namespace actor
}    // namespace nil
