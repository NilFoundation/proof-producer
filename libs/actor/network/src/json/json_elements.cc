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

#include <nil/actor/core/loop.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/json/json_elements.hh>

#include <cstring>
#include <string>
#include <vector>
#include <sstream>

namespace nil {
    namespace actor {

        using namespace std;

        namespace json {

            class json_stream_builder;
            /**
             * The json builder is a helper class
             * To help create a json object
             *
             */
            class json_builder {
            public:
                json_builder() : first(true) {
                    result << OPEN;
                }

                /**
                 * add a name value to an object
                 * @param name the name of the element
                 * @param str the value already formated
                 */
                void add(const string &name, const string &str) {
                    if (first) {
                        first = false;
                    } else {
                        result << ", ";
                    }
                    result << '"' << name << "\": " << str;
                }

                /**
                 * add a json element to the an object
                 * @param element
                 */
                void add(json_base_element *element) {
                    if (element == nullptr || element->_set == false) {
                        return;
                    }
                    try {
                        add(element->_name, element->to_string());
                    } catch (...) {
                        std::throw_with_nested(
                            std::runtime_error(format("Json generation failed for field: {}", element->_name)));
                    }
                }

                /**
                 * Get the string representation of the object
                 * @return a string of accumulative object
                 */
                string as_json() {
                    result << CLOSE;
                    return result.str();
                }

            private:
                static const string OPEN;
                static const string CLOSE;
                friend class json_stream_builder;
                stringstream result;
                bool first;
            };

            /**
             * The json builder is a helper class
             * To help create a json object
             *
             */
            class json_stream_builder {
            public:
                json_stream_builder(output_stream<char> &s) : first(true), open(false), _s(s) {
                }

                /**
                 * add a name value to an object
                 * @param name the name of the element
                 * @param str the value already formated
                 */
                future<> add(const string &name, const json_base_element &element) {
                    if (!open) {
                        open = true;
                        return _s.write(json_builder::OPEN).then([this, &name, &element] {
                            return add(name, element);
                        });
                    }
                    return _s.write(((first) ? '"' + name : ",\"" + name) + "\":").then([this, &element] {
                        first = false;
                        return element.write(_s);
                    });
                }

                /**
                 * add a json element to the an object
                 * @param element
                 */
                future<> add(json_base_element *element) {
                    if (element == nullptr || element->_set == false) {
                        return make_ready_future<>();
                    }
                    return add(element->_name, *element);
                }

                /**
                 * Get the string representation of the object
                 * @return a string of accumulative object
                 */
                future<> done() {
                    return _s.write(json_builder::CLOSE);
                }

            private:
                bool first;
                bool open;
                output_stream<char> &_s;
            };

            const string json_builder::OPEN("{");
            const string json_builder::CLOSE("}");

            void json_base::add(json_base_element *element, string name, bool mandatory) {
                element->_mandatory = mandatory;
                element->_name = name;
                _elements.push_back(element);
            }

            string json_base::to_json() const {
                json_builder res;
                for (auto i : _elements) {
                    res.add(i);
                }
                return res.as_json();
            }

            future<> json_base::write(output_stream<char> &s) const {
                return do_with(json_stream_builder(s), [this](json_stream_builder &builder) {
                    return do_for_each(_elements, [&builder](auto m) { return builder.add(m); }).then([&builder] {
                        return builder.done();
                    });
                });
            }

            bool json_base::is_verify() const {
                for (auto i : _elements) {
                    if (!i->is_verify()) {
                        return false;
                    }
                }
                return true;
            }

        }    // namespace json

    }    // namespace actor
}    // namespace nil
