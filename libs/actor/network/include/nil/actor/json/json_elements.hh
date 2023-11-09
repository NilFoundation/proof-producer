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

#include <string>
#include <vector>
#include <ctime>
#include <sstream>

#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/json/formatter.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/iostream.hh>

namespace nil {
    namespace actor {

        namespace json {

            /**
             * The base class for all json element.
             * Every json element has a name
             * An indication if it was set or not
             * And is this element is mandatory.
             * When a mandatory element is not set
             * this is not a valid object
             */
            class json_base_element {
            public:
                /**
                 * The constructors
                 */
                json_base_element() : _mandatory(false), _set(false) {
                }

                virtual ~json_base_element() = default;

                /**
                 * Check if it's a mandatory parameter
                 * and if it's set.
                 * @return true if this is not a mandatory parameter
                 * or if it is and it's value is set
                 */
                virtual bool is_verify() {
                    return !(_mandatory && !_set);
                }

                json_base_element &operator=(const json_base_element &o) {
                    // Names and mandatory are never changed after creation
                    _set = o._set;
                    return *this;
                }

                /**
                 * returns the internal value in a json format
                 * Each inherit class must implement this method
                 * @return formated internal value
                 */
                virtual std::string to_string() = 0;

                virtual future<> write(output_stream<char> &s) const = 0;
                std::string _name;
                bool _mandatory;
                bool _set;
            };

            /**
             * Basic json element instantiate
             * the json_element template.
             * it adds a value to the base definition
             * and the to_string implementation using the formatter
             */
            template<class T>
            class json_element : public json_base_element {
            public:
                /**
                 * the assignment operator also set
                 * the set value to true.
                 * @param new_value the new value
                 * @return the value itself
                 */
                json_element &operator=(const T &new_value) {
                    _value = new_value;
                    _set = true;
                    return *this;
                }
                /**
                 * the assignment operator also set
                 * the set value to true.
                 * @param new_value the new value
                 * @return the value itself
                 */
                template<class C>
                json_element &operator=(const C &new_value) {
                    _value = new_value;
                    _set = true;
                    return *this;
                }
                /**
                 * The brackets operator
                 * @return the value
                 */
                const T &operator()() const {
                    return _value;
                }

                /**
                 * The to_string return the value
                 * formated as a json value
                 * @return the value foramted for json
                 */
                virtual std::string to_string() override {
                    return formatter::to_json(_value);
                }

                virtual future<> write(output_stream<char> &s) const override {
                    return formatter::write(s, _value);
                }

            private:
                T _value;
            };

            /**
             * json_list is based on std vector implementation.
             *
             * When values are added with push it is set the "set" flag to true
             * hence will be included in the parsed object
             */
            template<class T>
            class json_list : public json_base_element {
            public:
                /**
                 * Add an element to the list.
                 * @param element a new element that will be added to the list
                 */
                void push(const T &element) {
                    _set = true;
                    _elements.push_back(element);
                }

                virtual std::string to_string() override {
                    return formatter::to_json(_elements);
                }

                /**
                 * Assignment can be done from any object that support const range
                 * iteration and that it's elements can be assigned to the list elements
                 */
                template<class C>
                json_list &operator=(const C &list) {
                    _elements.clear();
                    for (auto i : list) {
                        push(i);
                    }
                    return *this;
                }
                virtual future<> write(output_stream<char> &s) const override {
                    return formatter::write(s, _elements);
                }
                std::vector<T> _elements;
            };

            class jsonable {
            public:
                virtual ~jsonable() = default;
                /**
                 * create a foramted string of the object.
                 * @return the object formated.
                 */
                virtual std::string to_json() const = 0;

                /*!
                 * \brief write an object to the output stream
                 *
                 * The defult implementation uses the to_json
                 * Object implementation override it.
                 */
                virtual future<> write(output_stream<char> &s) const {
                    return s.write(to_json());
                }
            };

            /**
             * The base class for all json objects
             * It holds a list of all the element in it,
             * allowing it implement the to_json method.
             *
             * It also allows iterating over the element
             * in the object, even if not all the member
             * are known in advance and in practice mimic
             * reflection
             */
            struct json_base : public jsonable {

                virtual ~json_base() = default;

                json_base() = default;

                json_base(const json_base &) = delete;

                json_base operator=(const json_base &) = delete;

                /**
                 * create a foramted string of the object.
                 * @return the object formated.
                 */
                virtual std::string to_json() const;

                /*!
                 * \brief write to an output stream
                 */
                virtual future<> write(output_stream<char> &) const;

                /**
                 * Check that all mandatory elements are set
                 * @return true if all mandatory parameters are set
                 */
                virtual bool is_verify() const;

                /**
                 * Register an element in an object
                 * @param element the element to be added
                 * @param name the element name
                 * @param mandatory is this element mandatory.
                 */
                virtual void add(json_base_element *element, std::string name, bool mandatory = false);

                std::vector<json_base_element *> _elements;
            };

            /**
             * There are cases where a json request needs to return a successful
             * empty reply.
             * The json_void class will be used to mark that the reply should be empty.
             *
             */
            struct json_void : public jsonable {
                virtual std::string to_json() const {
                    return "";
                }

                /*!
                 * \brief write to an output stream
                 */
                virtual future<> write(output_stream<char> &s) const {
                    return s.close();
                }
            };

            /**
             * The json return type, is a helper class to return a json
             * formatted string.
             * It uses autoboxing in its constructor so when a function return
             * type is json_return_type, it could return a type that would be converted
             * ie.
             * json_return_type foo() {
             *     return "hello";
             * }
             *
             * would return a json formatted string: "hello" (rather then hello)
             */
            struct json_return_type {
                sstring _res;
                std::function<future<>(output_stream<char> &&)> _body_writer;
                json_return_type(std::function<future<>(output_stream<char> &&)> &&body_writer) :
                    _body_writer(std::move(body_writer)) {
                }
                template<class T>
                json_return_type(const T &res) {
                    _res = formatter::to_json(res);
                }

                json_return_type(json_return_type &&o) noexcept :
                    _res(std::move(o._res)), _body_writer(std::move(o._body_writer)) {
                }
                json_return_type &operator=(json_return_type &&o) noexcept {
                    _res = std::move(o._res);
                    _body_writer = std::move(o._body_writer);
                    return *this;
                }
            };

            /*!
             * \brief capture a range and return a serialize function for it as a json array.
             *
             * To use it, pass a range and a mapping function.
             * For example, if res is a map:
             *
             * return make_ready_future<json::json_return_type>(stream_range_as_array(res, [](const auto&i) {return
             * i.first}));
             */
            template<typename Container, typename Func>
#ifdef BOOST_HAS_CONCEPTS
            requires requires(Container c, Func aa, output_stream<char> s) {
                { formatter::write(s, aa(*c.begin())) }
                ->std::same_as<future<>>;
            }
#endif
            std::function<future<>(output_stream<char> &&)> stream_range_as_array(Container val, Func fun) {
                return [val = std::move(val), fun = std::move(fun)](output_stream<char> &&s) {
                    return do_with(
                        output_stream<char>(std::move(s)), Container(std::move(val)), Func(std::move(fun)), true,
                        [](output_stream<char> &s, const Container &val, const Func &f, bool &first) {
                            return s.write("[")
                                .then([&val, &s, &first, &f]() {
                                    return do_for_each(val, [&s, &first, &f](const typename Container::value_type &v) {
                                        auto fut = first ? make_ready_future<>() : s.write(", ");
                                        first = false;
                                        return fut.then([&s, &f, &v]() { return formatter::write(s, f(v)); });
                                    });
                                })
                                .then([&s]() { return s.write("]").then([&s] { return s.close(); }); });
                        });
                };
            }

            /*!
             * \brief capture an object and return a serialize function for it.
             *
             * To use it:
             * return make_ready_future<json::json_return_type>(stream_object(res));
             */
            template<class T>
            std::function<future<>(output_stream<char> &&)> stream_object(T val) {
                return [val = std::move(val)](output_stream<char> &&s) {
                    return do_with(output_stream<char>(std::move(s)), T(std::move(val)),
                                   [](output_stream<char> &s, const T &val) {
                                       return formatter::write(s, val).then([&s] { return s.close(); });
                                   });
                };
            }

        }    // namespace json

    }    // namespace actor
}    // namespace nil
