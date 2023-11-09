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

#include <nil/actor/core/posix.hh>

#include <nil/actor/detail/read_first_line.hh>

namespace nil {
    namespace actor {
        sstring read_first_line(boost::filesystem::path sys_file) {
            auto file = file_desc::open(sys_file.string(), O_RDONLY | O_CLOEXEC);
            sstring buf;
            size_t n = 0;
            do {
                // try to avoid allocations
                sstring tmp = uninitialized_string(8);
                auto ret = file.read(tmp.data(), 8ul);
                if (!ret) {    // EAGAIN
                    continue;
                }
                n = *ret;
                if (n > 0) {
                    buf += tmp;
                }
            } while (n != 0);
            auto end = buf.find('\n');
            auto value = buf.substr(0, end);
            file.close();
            return value;
        }

    }    // namespace actor
}    // namespace nil
