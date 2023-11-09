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

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/container/small_vector.hpp>

#include <nil/actor/core/detail/io_intent.hh>
#include <nil/actor/core/io_priority_class.hh>

namespace bi = boost::intrusive;

namespace nil {
    namespace actor {

        /// \example file_demo.cc
        /// A handle confirming the caller's intent to do the IO
        ///
        /// When a pointer to an intent is passed to the \ref io_queue
        /// "io_queue"::queue_request() method, the issued request is pinned
        /// to the intent and is only processed as long as the intent object
        /// is alive and the **cancel()** method is not called.
        ///
        /// If no intent is provided, then the request is processed till its
        /// completion be it success or error
        class io_intent {
            struct intents_for_queue {
                dev_t dev;
                io_priority_class_id qid;
                detail::cancellable_queue cq;

                intents_for_queue(dev_t dev_, io_priority_class_id qid_) noexcept : dev(dev_), qid(qid_), cq() {
                }

                intents_for_queue(intents_for_queue &&) noexcept = default;
                intents_for_queue &operator=(intents_for_queue &&) noexcept = default;
            };

            struct references {
                detail::intent_reference::container_type list;

                references(references &&) noexcept = default;

                references() noexcept = default;
                ~references() {
                    clear();
                }

                void clear() {
                    list.clear_and_dispose([](detail::intent_reference *r) { r->on_cancel(); });
                }

                void bind(detail::intent_reference &iref) noexcept {
                    list.push_back(iref);
                }
            };

            boost::container::small_vector<intents_for_queue, 1> _intents;
            references _refs;
            friend detail::intent_reference::intent_reference(io_intent *) noexcept;

        public:
            io_intent() = default;
            ~io_intent() = default;

            io_intent(const io_intent &) = delete;
            io_intent &operator=(const io_intent &) = delete;
            io_intent &operator=(io_intent &&) = delete;
            io_intent(io_intent &&o) noexcept : _intents(std::move(o._intents)), _refs(std::move(o._refs)) {
                for (auto &&r : _refs.list) {
                    r._intent = this;
                }
            }

            /// Explicitly cancels all the requests attached to this intent
            /// so far. The respective futures are resolved into the \ref
            /// cancelled_error "cancelled_error"
            void cancel() noexcept {
                _refs.clear();
                _intents.clear();
            }

            /// @private
            detail::cancellable_queue &find_or_create_cancellable_queue(dev_t dev, io_priority_class_id qid) {
                for (auto &&i : _intents) {
                    if (i.dev == dev && i.qid == qid) {
                        return i.cq;
                    }
                }

                _intents.emplace_back(dev, qid);
                return _intents.back().cq;
            }
        };
    }    // namespace actor
}    // namespace nil
