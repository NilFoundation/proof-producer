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

#ifndef CONVERSIONS_CC_
#define CONVERSIONS_CC_

#include <nil/actor/detail/conversions.hh>
#include <nil/actor/core/print.hh>

#include <boost/lexical_cast.hpp>

#include <cctype>

namespace nil {
    namespace actor {

        size_t parse_memory_size(std::string s) {
            size_t factor = 1;
            if (s.size()) {
                auto c = s[s.size() - 1];
                if (!isdigit(c)) {
                    static std::string suffixes = "kMGT";
                    auto pos = suffixes.find(c);
                    if (pos == suffixes.npos) {
                        throw std::runtime_error(format("Cannot parse memory size '{}'", s));
                    }
                    factor <<= (pos + 1) * 10;
                    s = s.substr(0, s.size() - 1);
                }
            }
            return boost::lexical_cast<size_t>(s) * factor;
        }

    }    // namespace actor
}    // namespace nil

#endif /* CONVERSIONS_CC_ */
