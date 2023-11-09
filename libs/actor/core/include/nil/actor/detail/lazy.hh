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

#include <ostream>

/// \addtogroup logging
/// @{

namespace nil {
    namespace actor {

        /// \brief This class is a wrapper for a lazy evaluation of a value.
        ///
        /// The value is evaluated by a functor that gets no parameters, which is
        /// provided to a lazy_value constructor.
        ///
        /// The instance may be created only using nil::actor::value_of helper function.
        ///
        /// The evaluation is triggered by operator().
        template<typename Func>
        class lazy_eval {
        private:
            Func _func;

        private:
            lazy_eval(Func &&f) : _func(std::forward<Func>(f)) {
            }

        public:
            /// \brief Evaluate a value.
            ///
            /// \return the evaluated value
            auto operator()() {
                return _func();
            }

            /// \brief Evaluate a value (const version).
            ///
            /// \return the evaluated value
            auto operator()() const {
                return _func();
            }

            template<typename F>
            friend lazy_eval<F> value_of(F &&func);
        };

        /// Create a nil::actor::lazy_eval object that will use a given functor for
        /// evaluating a value when the evaluation is triggered.
        ///
        /// The actual evaluation is triggered by applying a () operator on a
        /// returned object.
        ///
        /// \param func a functor to evaluate the value
        ///
        /// \return a lazy_eval object that may be used for evaluating a value
        template<typename Func>
        inline lazy_eval<Func> value_of(Func &&func) {
            return lazy_eval<Func>(std::forward<Func>(func));
        }

        /// \brief This struct is a wrapper for lazy dereferencing a pointer.
        ///
        /// In particular this is to be used in situations where the value of a
        /// pointer has to be converted to string in a lazy manner. Since
        /// pointers can be null adding a check at the point of calling the
        /// log function for example, will introduce an unnecessary branch in
        /// potentially useless code. Using lazy_deref this check can be
        /// deferred to the point where the code is actually evaluated.
        template<typename T>
        struct lazy_deref_wrapper {
            const T &p;

            constexpr lazy_deref_wrapper(const T &p) : p(p) {
            }
        };

        /// Create a nil::actor::lazy_deref_wrapper object.
        ///
        /// The actual dereferencing will happen when the object is inserted
        /// into a stream. The pointer is not copied, only a reference is saved
        /// to it. Smart pointers are supported as well.
        ///
        /// \param p a raw pointer or a smart pointer
        ///
        /// \return a lazy_deref_wrapper object
        template<typename T>
        lazy_deref_wrapper<T> lazy_deref(const T &p) {
            return lazy_deref_wrapper<T>(p);
        }

    }    // namespace actor
}    // namespace nil

namespace std {
    /// Output operator for a nil::actor::lazy_eval<Func>
    /// This would allow printing a nil::actor::lazy_eval<Func> as if it's a regular
    /// value.
    ///
    /// For example:
    ///
    /// `logger.debug("heavy eval result:{}", nil::actor::value_of([&] { return <heavy evaluation>; }));`
    ///
    /// (If a logging level is lower than "debug" the evaluation will not take place.)
    ///
    /// \tparam Func a functor type
    /// \param os ostream to print to
    /// \param lf a reference to a lazy_eval<Func> to be printed
    ///
    /// \return os
    template<typename Func>
    ostream &operator<<(ostream &os, const nil::actor::lazy_eval<Func> &lf) {
        return os << lf();
    }

    template<typename Func>
    ostream &operator<<(ostream &os, nil::actor::lazy_eval<Func> &lf) {
        return os << lf();
    }

    template<typename Func>
    ostream &operator<<(ostream &os, nil::actor::lazy_eval<Func> &&lf) {
        return os << lf();
    }

    template<typename T>
    ostream &operator<<(ostream &os, nil::actor::lazy_deref_wrapper<T> ld) {
        if (ld.p) {
            return os << *ld.p;
        }

        return os << "null";
    }
}    // namespace std
     /// @}
