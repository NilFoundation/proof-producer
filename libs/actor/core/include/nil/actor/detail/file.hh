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

#include <nil/actor/core/future.hh>

namespace nil {
    namespace actor {

        /// Recursively removes a directory and all of its contents.
        ///
        /// \param path path of the directory to recursively remove
        ///
        /// \note
        /// Unlike `rm -rf` path has to be a directory and may not refer to a regular file.
        ///
        /// The function flushes the parent directory of the removed path and so guaranteeing that
        /// the remove is stable on disk.
        ///
        /// The function bails out on first error. In that case, some files and/or sub-directories
        /// (and their contents) may be left behind at the level in which the error was detected.
        ///
        future<> recursive_remove_directory(boost::filesystem::path path) noexcept;

    }    // namespace actor
}    // namespace nil
