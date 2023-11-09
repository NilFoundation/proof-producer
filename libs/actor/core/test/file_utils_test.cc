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

#include <cstdlib>

#include <nil/actor/testing/test_case.hh>
#include <nil/actor/testing/thread_test_case.hh>
#include <nil/actor/testing/test_runner.hh>

#include <nil/actor/core/file.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/loop.hh>

#include <nil/actor/detail/tmp_file.hh>
#include <nil/actor/detail/file.hh>

using namespace nil::actor;
namespace fs = boost::filesystem;

class expected_exception : std::runtime_error {
public:
    expected_exception() : runtime_error("expected") {
    }
};

ACTOR_TEST_CASE(test_make_tmp_file) {
    return make_tmp_file().then([](tmp_file tf) {
        return async([tf = std::move(tf)]() mutable {
            const sstring tmp_path = tf.get_path().native();
            BOOST_REQUIRE(file_exists(tmp_path).get0());
            tf.close().get();
            tf.remove().get();
            BOOST_REQUIRE(!file_exists(tmp_path).get0());
        });
    });
}

static temporary_buffer<char> get_init_buffer(file &f) {
    auto buf = temporary_buffer<char>::aligned(f.memory_dma_alignment(), f.memory_dma_alignment());
    memset(buf.get_write(), 0, buf.size());
    return buf;
}

ACTOR_THREAD_TEST_CASE(test_tmp_file) {
    size_t expected = ~0;
    size_t actual = 0;

    tmp_file::do_with([&](tmp_file &tf) mutable {
        auto &f = tf.get_file();
        auto buf = get_init_buffer(f);
        return do_with(std::move(buf), [&](auto &buf) mutable {
            expected = buf.size();
            return f.dma_write(0, buf.get(), buf.size()).then([&](size_t written) {
                actual = written;
                return make_ready_future<>();
            });
        });
    }).get();
    BOOST_REQUIRE_EQUAL(expected, actual);
}

ACTOR_THREAD_TEST_CASE(test_non_existing_TMPDIR) {
    auto old_tmpdir = getenv("TMPDIR");
    setenv("TMPDIR", "/tmp/non-existing-TMPDIR", true);
    BOOST_REQUIRE_EXCEPTION(tmp_file::do_with("/tmp/non-existing-TMPDIR", [](tmp_file &tf) {}).get(), std::system_error,
                            testing::exception_predicate::message_contains("No such file or directory"));
    if (old_tmpdir) {
        setenv("TMPDIR", old_tmpdir, true);
    } else {
        unsetenv("TMPDIR");
    }
}

static future<> touch_file(const sstring &filename, open_flags oflags = open_flags::rw | open_flags::create) noexcept {
    return open_file_dma(filename, oflags).then([](file f) { return f.close().finally([f] {}); });
}

ACTOR_THREAD_TEST_CASE(test_recursive_remove_directory) {
    struct test_dir {
        test_dir *parent;
        sstring name;
        std::list<sstring> sub_files = {};
        std::list<test_dir> sub_dirs = {};

        test_dir(test_dir *parent, sstring name) : parent(parent), name(std::move(name)) {
        }

        boost::filesystem::path path() const {
            if (!parent) {
                return boost::filesystem::path(name.c_str());
            }
            return parent->path() / name.c_str();
        }

        void fill_random_file(std::uniform_int_distribution<unsigned> &dist, std::default_random_engine &eng) {
            sub_files.emplace_back(format("file-{}", dist(eng)));
        }

        test_dir &fill_random_dir(std::uniform_int_distribution<unsigned> &dist, std::default_random_engine &eng) {
            sub_dirs.emplace_back(this, format("dir-{}", dist(eng)));
            return sub_dirs.back();
        }

        void random_fill(int level, int levels, std::uniform_int_distribution<unsigned> &dist,
                         std::default_random_engine &eng) {
            int num_files = dist(eng) % 10;
            int num_dirs = (level < levels - 1) ? (1 + dist(eng) % 3) : 0;

            for (int i = 0; i < num_files; i++) {
                fill_random_file(dist, eng);
            }

            if (num_dirs) {
                level++;
                for (int i = 0; i < num_dirs; i++) {
                    fill_random_dir(dist, eng).random_fill(level, levels, dist, eng);
                }
            }
        }

        future<> populate() {
            return touch_directory(path().native()).then([this] {
                return parallel_for_each(sub_files,
                                         [this](auto &name) { return touch_file((path() / name.c_str()).native()); })
                    .then([this] {
                        return parallel_for_each(sub_dirs, [](auto &sub_dir) { return sub_dir.populate(); });
                    });
            });
        }
    };

    auto &eng = testing::local_random_engine;
    auto dist = std::uniform_int_distribution<unsigned>();
    int levels = 1 + dist(eng) % 3;
    test_dir root = {nullptr, default_tmpdir().native()};
    test_dir base = {&root, format("base-{}", dist(eng))};
    base.random_fill(0, levels, dist, eng);
    base.populate().get();
    recursive_remove_directory(base.path()).get();
    BOOST_REQUIRE(!file_exists(base.path().native()).get0());
}

ACTOR_TEST_CASE(test_make_tmp_dir) {
    return make_tmp_dir().then([](tmp_dir td) {
        return async([td = std::move(td)]() mutable {
            const sstring tmp_path = td.get_path().native();
            BOOST_REQUIRE(file_exists(tmp_path).get0());
            td.remove().get();
            BOOST_REQUIRE(!file_exists(tmp_path).get0());
        });
    });
}

ACTOR_THREAD_TEST_CASE(test_tmp_dir) {
    size_t expected;
    size_t actual;
    tmp_dir::do_with([&](tmp_dir &td) {
        return tmp_file::do_with(td.get_path(), [&](tmp_file &tf) {
            auto &f = tf.get_file();
            auto buf = get_init_buffer(f);
            return do_with(std::move(buf), [&](auto &buf) mutable {
                expected = buf.size();
                return f.dma_write(0, buf.get(), buf.size()).then([&](size_t written) {
                    actual = written;
                    return make_ready_future<>();
                });
            });
        });
    }).get();
    BOOST_REQUIRE_EQUAL(expected, actual);
}

ACTOR_THREAD_TEST_CASE(test_tmp_dir_with_path) {
    size_t expected;
    size_t actual;
    tmp_dir::do_with(".", [&](tmp_dir &td) {
        return tmp_file::do_with(td.get_path(), [&](tmp_file &tf) {
            auto &f = tf.get_file();
            auto buf = get_init_buffer(f);
            return do_with(std::move(buf), [&](auto &buf) mutable {
                expected = buf.size();
                return tf.get_file().dma_write(0, buf.get(), buf.size()).then([&](size_t written) {
                    actual = written;
                    return make_ready_future<>();
                });
            });
        });
    }).get();
    BOOST_REQUIRE_EQUAL(expected, actual);
}

ACTOR_THREAD_TEST_CASE(test_tmp_dir_with_non_existing_path) {
    BOOST_REQUIRE_EXCEPTION(tmp_dir::do_with("/tmp/this_name_should_not_exist", [](tmp_dir &) {}).get(),
                            std::system_error,
                            testing::exception_predicate::message_contains("No such file or directory"));
}

ACTOR_TEST_CASE(tmp_dir_with_thread_test) {
    return tmp_dir::do_with_thread([](tmp_dir &td) {
        tmp_file tf = make_tmp_file(td.get_path()).get0();
        auto &f = tf.get_file();
        auto buf = get_init_buffer(f);
        auto expected = buf.size();
        auto actual = f.dma_write(0, buf.get(), buf.size()).get0();
        BOOST_REQUIRE_EQUAL(expected, actual);
        tf.close().get();
        tf.remove().get();
    });
}

ACTOR_TEST_CASE(tmp_dir_with_leftovers_test) {
    return tmp_dir::do_with_thread([](tmp_dir &td) {
        boost::filesystem::path path = td.get_path() / "testfile.tmp";
        touch_file(path.native()).get();
        BOOST_REQUIRE(file_exists(path.native()).get0());
    });
}

ACTOR_TEST_CASE(tmp_dir_do_with_fail_func_test) {
    return tmp_dir::do_with_thread([](tmp_dir &outer) {
        BOOST_REQUIRE_THROW(tmp_dir::do_with([](tmp_dir &inner) mutable {
                                return make_exception_future<>(expected_exception());
                            }).get(),
                            expected_exception);
    });
}

ACTOR_TEST_CASE(tmp_dir_do_with_fail_remove_test) {
    return tmp_dir::do_with_thread([](tmp_dir &outer) {
        auto saved_default_tmpdir = default_tmpdir();
        sstring outer_path = outer.get_path().native();
        sstring inner_path;
        set_default_tmpdir(outer_path.c_str());
        BOOST_REQUIRE_THROW(tmp_dir::do_with([outer_path, &inner_path](tmp_dir &inner) mutable {
                                inner_path = inner.get_path().native();
                                return chmod(outer_path, file_permissions::user_read | file_permissions::user_execute);
                            }).get(),
                            std::system_error);
        BOOST_REQUIRE(file_exists(inner_path).get0());
        chmod(outer_path, file_permissions::default_dir_permissions).get();
        set_default_tmpdir(saved_default_tmpdir.c_str());
    });
}

ACTOR_TEST_CASE(tmp_dir_do_with_thread_fail_func_test) {
    return tmp_dir::do_with_thread([](tmp_dir &outer) {
        BOOST_REQUIRE_THROW(tmp_dir::do_with_thread([](tmp_dir &inner) mutable { throw expected_exception(); }).get(),
                            expected_exception);
    });
}

ACTOR_TEST_CASE(tmp_dir_do_with_thread_fail_remove_test) {
    return tmp_dir::do_with_thread([](tmp_dir &outer) {
        auto saved_default_tmpdir = default_tmpdir();
        sstring outer_path = outer.get_path().native();
        sstring inner_path;
        set_default_tmpdir(outer_path.c_str());
        BOOST_REQUIRE_THROW(tmp_dir::do_with_thread([outer_path, &inner_path](tmp_dir &inner) mutable {
                                inner_path = inner.get_path().native();
                                chmod(outer_path, file_permissions::user_read | file_permissions::user_execute).get();
                            }).get(),
                            std::system_error);
        BOOST_REQUIRE(file_exists(inner_path).get0());
        chmod(outer_path, file_permissions::default_dir_permissions).get();
        set_default_tmpdir(saved_default_tmpdir.c_str());
    });
}
