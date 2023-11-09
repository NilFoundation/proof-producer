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

namespace nil {
    namespace actor {

        /// \brief make a filesystem_error for system calls with a single file operand.
        ///
        /// \param what - describes the action that failed
        /// \param path - path of the file that hit the error
        /// \param error - the system error number (see errno(3))
        ///
        boost::filesystem::filesystem_error make_filesystem_error(const std::string &what, boost::filesystem::path path,
                                                                  int error);

        /// \brief make a filesystem_error for system calls with two file operands.
        ///
        /// \param what - describes the action that failed
        /// \param path1, path2 - paths of the files that hit the error
        /// \param error - the system error number (see errno(3))
        ///
        boost::filesystem::filesystem_error make_filesystem_error(const std::string &what,
                                                                  boost::filesystem::path path1,
                                                                  boost::filesystem::path path2, int error);

    }    // namespace actor
}    // namespace nil
