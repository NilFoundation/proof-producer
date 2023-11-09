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

#include <boost/filesystem.hpp>

#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {

        namespace fs = boost::filesystem;

        template<typename T>
        struct syscall_result {
            T result;
            int error;
            syscall_result(T result, int error) : result {std::move(result)}, error {error} {
            }
            void throw_if_error() const {
                if (long(result) == -1) {
                    throw std::system_error(ec());
                }
            }

            void throw_fs_exception(const sstring &reason, const boost::filesystem::path &path) const {
                throw boost::filesystem::filesystem_error(
                    reason, path, boost::system::error_code(error, boost::system::system_category()));
            }

            void throw_fs_exception(const sstring &reason, const boost::filesystem::path &path1,
                                    const boost::filesystem::path &path2) const {
                throw boost::filesystem::filesystem_error(
                    reason, path1, path2, boost::system::error_code(error, boost::system::system_category()));
            }

            void throw_fs_exception_if_error(const sstring &reason, const sstring &path) const {
                if (long(result) == -1) {
                    throw_fs_exception(reason, boost::filesystem::path(path));
                }
            }

            void throw_fs_exception_if_error(const sstring &reason, const sstring &path1, const sstring &path2) const {
                if (long(result) == -1) {
                    throw_fs_exception(reason, boost::filesystem::path(path1), boost::filesystem::path(path2));
                }
            }

        protected:
            std::error_code ec() const {
                return std::error_code(error, std::system_category());
            }
        };

        // Wrapper for a system call result containing the return value,
        // an output parameter that was returned from the syscall, and errno.
        template<typename Extra>
        struct syscall_result_extra : public syscall_result<int> {
            Extra extra;
            syscall_result_extra(int result, int error, Extra e) :
                syscall_result<int> {result, error}, extra {std::move(e)} {
            }
        };

        template<typename T>
        syscall_result<T> wrap_syscall(T result) {
            return syscall_result<T> {std::move(result), errno};
        }

        template<typename Extra>
        syscall_result_extra<Extra> wrap_syscall(int result, const Extra &extra) {
            return syscall_result_extra<Extra> {result, errno, extra};
        }
    }    // namespace actor
}    // namespace nil
