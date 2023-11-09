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

namespace bi = boost::intrusive;

namespace nil {
    namespace actor {

        class io_intent;

        namespace detail {

            /*
             * The tracker of cancellable sub-queue of requests.
             *
             * This queue is stuffed with requests that sit in the
             * same IO queue for dispatching (there can be other requests
             * as well) and ties them together for cancellation.
             * This IO queue is the fair_queue's priority_class's one.
             * Beware, if requests from different IO queues happen
             * in the same cancellable queue the whole thing blows up.
             */
            class cancellable_queue {
            public:
                class link {
                    friend class cancellable_queue;

                    union {
                        cancellable_queue *_ref;
                        boost::intrusive::slist_member_hook<> _hook;
                    };

                public:
                    link() noexcept : _ref(nullptr) {
                    }
                    ~link() {
                        assert(_ref == nullptr);
                    }

                    void enqueue(cancellable_queue *cq) noexcept {
                        if (cq != nullptr) {
                            cq->push_back(*this);
                        }
                    }

                    void maybe_dequeue() noexcept {
                        if (_ref != nullptr) {
                            _ref->pop_front();
                        }
                    }
                };

            private:
                static_assert(sizeof(link) == sizeof(void *), "cancellable_queue::link size is too big");
                using list_of_links_t = boost::intrusive::slist<link, boost::intrusive::constant_time_size<false>, boost::intrusive::cache_last<true>,
                                                  boost::intrusive::member_hook<link, boost::intrusive::slist_member_hook<>, &link::_hook>>;

                link *_first;
                list_of_links_t _rest;

                void push_back(link &il) noexcept;
                void pop_front() noexcept;

            public:
                cancellable_queue() noexcept : _first(nullptr) {
                }
                cancellable_queue(const cancellable_queue &) = delete;
                cancellable_queue(cancellable_queue &&o) noexcept;
                cancellable_queue &operator=(cancellable_queue &&o) noexcept;
                ~cancellable_queue();
            };

            /*
             * A "safe" reference on a intent. Safe here means that the original
             * intent can be destroyed at any time and this reference will be
             * updated not to point at it any longer.
             * The retrieve() method brings the original intent back or throws
             * and exception if the intent was cancelled.
             */
            class intent_reference : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
                friend class nil::actor::io_intent;
                using container_type = boost::intrusive::list<intent_reference, boost::intrusive::constant_time_size<false>>;
                static constexpr uintptr_t _cancelled_intent = 1;
                io_intent *_intent;

                void on_cancel() noexcept {
                    _intent = reinterpret_cast<io_intent *>(_cancelled_intent);
                }
                bool is_cancelled() const noexcept {
                    return _intent == reinterpret_cast<io_intent *>(_cancelled_intent);
                }

            public:
                intent_reference(io_intent *intent) noexcept;

                intent_reference(intent_reference &&o) noexcept : _intent(std::exchange(o._intent, nullptr)) {
                    container_type::node_algorithms::swap_nodes(o.this_ptr(), this_ptr());
                }

                intent_reference &operator=(intent_reference &&o) noexcept {
                    if (this != &o) {
                        _intent = std::exchange(o._intent, nullptr);
                        unlink();
                        container_type::node_algorithms::swap_nodes(o.this_ptr(), this_ptr());
                    }
                    return *this;
                }

                io_intent *retrieve() const;
            };

        }    // namespace detail

    }    // namespace actor
}    // namespace nil
