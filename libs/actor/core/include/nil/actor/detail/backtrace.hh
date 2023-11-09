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

#include <execinfo.h>
#include <iosfwd>
#include <variant>

#include <boost/container/static_vector.hpp>

#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/scheduling.hh>
#include <nil/actor/core/shared_ptr.hh>

namespace nil {
    namespace actor {

        struct shared_object {
            sstring name;
            uintptr_t begin;
            uintptr_t end;    // C++-style, last addr + 1
        };

        struct frame {
            const shared_object *so;
            uintptr_t addr;
        };

        bool operator==(const frame &a, const frame &b);

        // If addr doesn't seem to belong to any of the provided shared objects, it
        // will be considered as part of the executable.
        frame decorate(uintptr_t addr);

        // Invokes func for each frame passing it as argument.
        template<typename Func>
        void backtrace(Func &&func) noexcept(noexcept(func(frame()))) {
            constexpr size_t max_backtrace = 100;
            void *buffer[max_backtrace];
            int n = ::backtrace(buffer, max_backtrace);
            for (int i = 0; i < n; ++i) {
                auto ip = reinterpret_cast<uintptr_t>(buffer[i]);
                func(decorate(ip - 1));
            }
        }

        // Represents a call stack of a single thread.
        class simple_backtrace {
        public:
            using vector_type = boost::container::static_vector<frame, 64>;

        private:
            vector_type _frames;
            size_t _hash;

        private:
            size_t calculate_hash() const;

        public:
            simple_backtrace() = default;
            simple_backtrace(vector_type f) : _frames(std::move(f)) {
            }
            size_t hash() const {
                return _hash;
            }

            friend std::ostream &operator<<(std::ostream &out, const simple_backtrace &);

            bool operator==(const simple_backtrace &o) const {
                return _hash == o._hash && _frames == o._frames;
            }

            bool operator!=(const simple_backtrace &o) const {
                return !(*this == o);
            }
        };

        using shared_backtrace = nil::actor::lw_shared_ptr<simple_backtrace>;

        // Represents a task object inside a tasktrace.
        class task_entry {
            const std::type_info *_task_type;

        public:
            task_entry(const std::type_info &ti) : _task_type(&ti) {
            }

            friend std::ostream &operator<<(std::ostream &out, const task_entry &);

            bool operator==(const task_entry &o) const {
                return *_task_type == *o._task_type;
            }

            bool operator!=(const task_entry &o) const {
                return !(*this == o);
            }

            size_t hash() const {
                return _task_type->hash_code();
            }
        };

        // Extended backtrace which consists of a backtrace of the currently running task
        // and information about the chain of tasks waiting for the current operation to complete.
        class tasktrace {
        public:
            using entry = std::variant<shared_backtrace, task_entry>;
            using vector_type = boost::container::static_vector<entry, 16>;

        private:
            simple_backtrace _main;
            vector_type _prev;
            scheduling_group _sg;
            size_t _hash;

        public:
            tasktrace() = default;
            tasktrace(simple_backtrace main, vector_type prev, size_t prev_hash, scheduling_group sg);
            ~tasktrace();

            size_t hash() const {
                return _hash;
            }

            friend std::ostream &operator<<(std::ostream &out, const tasktrace &);

            bool operator==(const tasktrace &o) const;

            bool operator!=(const tasktrace &o) const {
                return !(*this == o);
            }
        };

    }    // namespace actor
}    // namespace nil

namespace std {

    template<>
    struct hash<nil::actor::simple_backtrace> {
        size_t operator()(const nil::actor::simple_backtrace &b) const {
            return b.hash();
        }
    };

    template<>
    struct hash<nil::actor::tasktrace> {
        size_t operator()(const nil::actor::tasktrace &b) const {
            return b.hash();
        }
    };

}    // namespace std

namespace nil {
    namespace actor {

        using saved_backtrace = tasktrace;

        saved_backtrace current_backtrace() noexcept;

        tasktrace current_tasktrace() noexcept;

        // Collects backtrace only within the currently executing task.
        simple_backtrace current_backtrace_tasklocal() noexcept;

        std::ostream &operator<<(std::ostream &out, const tasktrace &b);

        namespace detail {

            template<class Exc>
            class backtraced : public Exc {
                std::shared_ptr<sstring> _backtrace;

            public:
                template<typename... Args>
                backtraced(Args &&...args) :
                    Exc(std::forward<Args>(args)...),
                    _backtrace(
                        std::make_shared<sstring>(format("{} Backtrace: {}", Exc::what(), current_backtrace()))) {
                }

                /**
                 * Returns the original exception message with a backtrace appended to it
                 *
                 * @return original exception message followed by a backtrace
                 */
                virtual const char *what() const noexcept override {
                    assert(_backtrace);
                    return _backtrace->c_str();
                }
            };

        }    // namespace detail

        /// Create an exception pointer of unspecified type that is derived from Exc type
        /// with a backtrace attached to its message.
        ///
        /// \tparam Exc exception type to be caught at the receiving side
        /// \tparam Args types of arguments forwarded to the constructor of Exc
        /// \param args arguments forwarded to the constructor of Exc
        /// \return std::exception_ptr containing the exception with the backtrace.
        template<class Exc, typename... Args>
        std::exception_ptr make_backtraced_exception_ptr(Args &&...args) {
            using exc_type = std::decay_t<Exc>;
            static_assert(std::is_base_of<std::exception, exc_type>::value,
                          "throw_with_backtrace only works with exception types");
            return std::make_exception_ptr<detail::backtraced<exc_type>>(Exc(std::forward<Args>(args)...));
        }

        /**
         * Throws an exception of unspecified type that is derived from the Exc type
         * with a backtrace attached to its message
         *
         * @tparam Exc exception type to be caught at the receiving side
         * @tparam Args types of arguments forwarded to the constructor of Exc
         * @param args arguments forwarded to the constructor of Exc
         * @return never returns (throws an exception)
         */
        template<class Exc, typename... Args>
        [[noreturn]] void throw_with_backtrace(Args &&...args) {
            std::rethrow_exception(make_backtraced_exception_ptr<Exc>(std::forward<Args>(args)...));
        };

    }    // namespace actor
}    // namespace nil
