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

#include <nil/actor/json/formatter.hh>
#include <nil/actor/json/json_elements.hh>

#include <cmath>

namespace nil {
    namespace actor {

        using namespace std;

        namespace json {

            sstring formatter::begin(state s) {
                switch (s) {
                    case state::array:
                        return "[";
                    case state::map:
                        return "{";
                    default:
                        return {};
                }
            }

            sstring formatter::end(state s) {
                switch (s) {
                    case state::array:
                        return "]";
                    case state::map:
                        return "}";
                    default:
                        return {};
                }
            }

            sstring formatter::to_json(const sstring &str) {
                return to_json(str.c_str());
            }

            sstring formatter::to_json(const char *str) {
                sstring res = "\"";
                res += str;
                res += "\"";
                return res;
            }

            sstring formatter::to_json(int n) {
                return to_string(n);
            }

            sstring formatter::to_json(unsigned n) {
                return to_string(n);
            }

            sstring formatter::to_json(long n) {
                return to_string(n);
            }

            sstring formatter::to_json(float f) {
                if (std::isinf(f)) {
                    throw out_of_range("Infinite float value is not supported");
                } else if (std::isnan(f)) {
                    throw invalid_argument("Invalid float value");
                }
                return to_sstring(f);
            }

            sstring formatter::to_json(double d) {
                if (std::isinf(d)) {
                    throw out_of_range("Infinite double value is not supported");
                } else if (std::isnan(d)) {
                    throw invalid_argument("Invalid double value");
                }
                return to_sstring(d);
            }

            sstring formatter::to_json(bool b) {
                return (b) ? "true" : "false";
            }

            sstring formatter::to_json(const date_time &d) {
                // use RFC3339/RFC8601 "internet format"
                // which is stipulated as mandatory for swagger
                // dates
                // Note that this assumes dates are in UTC timezone
                static constexpr const char *TIME_FORMAT = "%FT%TZ";

                char buff[50];
                sstring res = "\"";
                strftime(buff, 50, TIME_FORMAT, &d);
                res += buff;
                return res + "\"";
            }

            sstring formatter::to_json(const jsonable &obj) {
                return obj.to_json();
            }

            sstring formatter::to_json(unsigned long l) {
                return to_string(l);
            }

        }    // namespace json

    }    // namespace actor
}    // namespace nil
