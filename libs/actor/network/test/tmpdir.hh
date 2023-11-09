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

#include <nil/actor/detail/tmp_file.hh>

namespace nil {
    namespace actor {

        /**
         * Temp dir helper for RAII usage when doing tests
         * in actor threads. Will not work in "normal" mode.
         * Just use tmp_dir::do_with for that.
         */
        class tmpdir {
            nil::actor::tmp_dir _tmp;

        public:
            tmpdir(tmpdir &&) = default;
            tmpdir(const tmpdir &) = delete;

            tmpdir(const sstring &name = sstring(nil::actor::default_tmpdir().string()) + "/testXXXX") {
                _tmp.create(boost::filesystem::path(name)).get();
            }
            ~tmpdir() {
                _tmp.remove().get();
            }
            auto path() const {
                return _tmp.get_path();
            }
        };
    }    // namespace actor
}    // namespace nil
