// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
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

#include <atomic>
#include <deque>
#include <future>
#include <memory>

#include <boost/lockfree/queue.hpp>

#include <nil/actor/core/future.hh>
#include <nil/actor/core/cacheline.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/metrics_registration.hh>

/// \file

namespace nil {
    namespace actor {

        class reactor;

        /// \brief Integration with non-actor applications.
        namespace alien {

            class message_queue {
                static constexpr size_t batch_size = 128;
                static constexpr size_t prefetch_cnt = 2;
                struct work_item;
                struct lf_queue_remote {
                    reactor *remote;
                };
                using lf_queue_base = boost::lockfree::queue<work_item *>;
                // use inheritence to control placement order
                struct lf_queue : lf_queue_remote, lf_queue_base {
                    lf_queue(reactor *remote) : lf_queue_remote {remote}, lf_queue_base {batch_size} {
                    }
                    void maybe_wakeup();
                } _pending;
                struct alignas(nil::actor::cache_line_size) {
                    std::atomic<size_t> value {0};
                } _sent;
                // keep this between two structures with statistics
                // this makes sure that they have at least one cache line
                // between them, so hw prefetcher will not accidentally prefetch
                // cache line used by another cpu.
                metrics::metric_groups _metrics;
                struct alignas(nil::actor::cache_line_size) {
                    size_t _received = 0;
                    size_t _last_rcv_batch = 0;
                };
                struct work_item {
                    virtual ~work_item() = default;
                    virtual void process() = 0;
                };
                template<typename Func>
                struct async_work_item : work_item {
                    Func _func;
                    async_work_item(Func &&func) : _func(std::move(func)) {
                    }
                    void process() override {
                        _func();
                    }
                };
                template<typename Func>
                size_t process_queue(lf_queue &q, Func process);
                void submit_item(std::unique_ptr<work_item> wi);

            public:
                message_queue(reactor *to);
                void start();
                void stop();
                template<typename Func>
                void submit(Func &&func) {
                    auto wi = std::make_unique<async_work_item<Func>>(std::forward<Func>(func));
                    submit_item(std::move(wi));
                }
                size_t process_incoming();
                bool pure_poll_rx() const;
            };

            class smp {
                struct qs_deleter {
                    unsigned count;
                    qs_deleter(unsigned n = 0) : count(n) {
                    }
                    qs_deleter(const qs_deleter &d) : count(d.count) {
                    }
                    void operator()(message_queue *qs) const;
                };
                using qs = std::unique_ptr<message_queue[], qs_deleter>;

            public:
                static qs create_qs(const std::vector<reactor *> &reactors);
                static qs _qs;
                static bool poll_queues();
                static bool pure_poll_queues();
            };

            /// Runs a function on a remote shard from an alien thread where engine() is not available.
            ///
            /// \param shard designates the shard to run the function on
            /// \param func a callable to run on shard \c t.  If \c func is a temporary object,
            ///          its lifetime will be extended by moving it.  If \c func is a reference,
            ///          the caller must guarantee that it will survive the call.
            /// \note the func must not throw and should return \c void. as we cannot identify the
            ///          alien thread, hence we are not able to post the fulfilled promise to the
            ///          message queue managed by the shard executing the alien thread which is
            ///          interested to the return value. Please use \c submit_to() instead, if
            ///          \c func throws.
            template<typename Func>
            void run_on(unsigned shard, Func func) {
                smp::_qs[shard].submit(std::move(func));
            }

            namespace detail {
                template<typename Func>
                using return_value_t = typename futurize<typename std::invoke_result<Func>::type>::value_type;

                template<typename Func, bool = std::is_empty_v<return_value_t<Func>>>
                struct return_type_of {
                    using type = void;
                    static void set(std::promise<void> &p, return_value_t<Func> &&) {
                        p.set_value();
                    }
                };
                template<typename Func>
                struct return_type_of<Func, false> {
                    using return_tuple_t = typename futurize<typename std::invoke_result<Func>::type>::tuple_type;
                    using type = std::tuple_element_t<0, return_tuple_t>;
                    static void set(std::promise<type> &p, return_value_t<Func> &&t) {
#if ACTOR_API_LEVEL < 5
                        p.set_value(std::get<0>(std::move(t)));
#else
                        p.set_value(std::move(t));
#endif
                    }
                };
                template<typename Func>
                using return_type_t = typename return_type_of<Func>::type;
            }    // namespace detail

            /// Runs a function on a remote shard from an alien thread where engine() is not available.
            ///
            /// \param shard designates the shard to run the function on
            /// \param func a callable to run on \c shard.  If \c func is a temporary object,
            ///          its lifetime will be extended by moving it.  If \c func is a reference,
            ///          the caller must guarantee that it will survive the call.
            /// \return whatever \c func returns, as a \c std::future<>
            /// \note the caller must keep the returned future alive until \c func returns
            template<typename Func, typename T = detail::return_type_t<Func>>
            std::future<T> submit_to(unsigned shard, Func func) {
                std::promise<T> pr;
                auto fut = pr.get_future();
                run_on(shard, [pr = std::move(pr), func = std::move(func)]() mutable {
                    // std::future returned via std::promise above.
                    (void)func().then_wrapped([pr = std::move(pr)](auto &&result) mutable {
                        try {
                            detail::return_type_of<Func>::set(pr, result.get());
                        } catch (...) {
                            pr.set_exception(std::current_exception());
                        }
                    });
                });
                return fut;
            }

        }    // namespace alien
    }        // namespace actor
}    // namespace nil
