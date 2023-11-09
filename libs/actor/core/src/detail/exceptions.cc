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

#include <nil/actor/detail/exceptions.hh>

namespace nil {
    namespace actor {

        boost::filesystem::filesystem_error make_filesystem_error(const std::string &what, boost::filesystem::path path,
                                                                  int error) {
            return boost::filesystem::filesystem_error(
                what, std::move(path), boost::system::error_code(error, boost::system::system_category()));
        }

        boost::filesystem::filesystem_error make_filesystem_error(const std::string &what,
                                                                  boost::filesystem::path path1,
                                                                  boost::filesystem::path path2, int error) {
            return boost::filesystem::filesystem_error(
                what, std::move(path1), std::move(path1),
                boost::system::error_code(error, boost::system::system_category()));
        }

    }    // namespace actor
}    // namespace nil
