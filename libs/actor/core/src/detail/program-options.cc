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

#include <nil/actor/detail/program-options.hh>

#include <regex>

namespace bpo = boost::program_options;

namespace nil {
    namespace actor {

        namespace program_options {

            sstring get_or_default(const string_map &ss, const sstring &key, const sstring &def) {
                const auto iter = ss.find(key);
                if (iter != ss.end()) {
                    return iter->second;
                }

                return def;
            }

            static void parse_map_associations(const std::string &v, string_map &ss) {
                static const std::regex colon(":");

                std::sregex_token_iterator s(v.begin(), v.end(), colon, -1);
                const std::sregex_token_iterator e;
                while (s != e) {
                    const sstring p = std::string(*s++);

                    const auto i = p.find('=');
                    if (i == sstring::npos) {
                        throw bpo::invalid_option_value(p);
                    }

                    auto k = p.substr(0, i);
                    auto v = p.substr(i + 1, p.size());
                    ss[std::move(k)] = std::move(v);
                };
            }

            void validate(boost::any &out, const std::vector<std::string> &in, string_map *, int) {
                if (out.empty()) {
                    out = boost::any(string_map());
                }

                auto *ss = boost::any_cast<string_map>(&out);

                for (const auto &s : in) {
                    parse_map_associations(s, *ss);
                }
            }

            std::ostream &operator<<(std::ostream &os, const string_map &ss) {
                int n = 0;

                for (const auto &e : ss) {
                    if (n > 0) {
                        os << ":";
                    }

                    os << e.first << "=" << e.second;
                    ++n;
                }

                return os;
            }

            std::istream &operator>>(std::istream &is, string_map &ss) {
                std::string str;
                is >> str;

                parse_map_associations(str, ss);
                return is;
            }
        }    // namespace program_options
    }        // namespace actor
}    // namespace nil
