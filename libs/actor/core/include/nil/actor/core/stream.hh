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

#include <nil/actor/core/future.hh>
#include <exception>
#include <functional>
#include <cassert>

namespace nil {
    namespace actor {

        // A stream/subscription pair is similar to a promise/future pair,
        // but apply to a sequence of values instead of a single value.
        //
        // A stream<> is the producer side.  It may call produce() as long
        // as the future<> returned from the previous invocation is ready.
        // To signify no more data is available, call close().
        //
        // A subscription<> is the consumer side.  It is created by a call
        // to stream::listen().  Calling subscription::start(),
        // which registers the data processing callback, starts processing
        // events.  It may register for end-of-stream notifications by
        // chaining the when_done() future, which also delivers error
        // events (as exceptions).
        //
        // The consumer can pause generation of new data by returning
        // a non-ready future; when the future becomes ready, the producer
        // will resume processing.

        template<typename... T>
        class stream;

        template<typename... T>
        class subscription;

        template<typename... T>
        class stream {
        public:
            using next_fn = noncopyable_function<future<>(T...)>;

        private:
            promise<> _done;
            promise<> _ready;
            next_fn _next;

            /// \brief Start receiving events from the stream.
            ///
            /// \param next Callback to call for each event
            void start(next_fn next) {
                _next = std::move(next);
                _ready.set_value();
            }

        public:
            stream() = default;
            stream(const stream &) = delete;
            stream(stream &&) = delete;
            void operator=(const stream &) = delete;
            void operator=(stream &&) = delete;

            // Returns a subscription that reads value from this
            // stream.
            subscription<T...> listen() {
                return subscription<T...>(this);
            }

            // Returns a subscription that reads value from this
            // stream, and also sets up the listen function.
            subscription<T...> listen(next_fn next) {
                start(std::move(next));
                return subscription<T...>(this);
            }

            // Becomes ready when the listener is ready to accept
            // values.  Call only once, when beginning to produce
            // values.
            future<> started() {
                return _ready.get_future();
            }

            // Produce a value.  Call only after started(), and after
            // a previous produce() is ready.
            future<> produce(T... data);

            // End the stream.   Call only after started(), and after
            // a previous produce() is ready.  No functions may be called
            // after this.
            void close() {
                _done.set_value();
            }

            // Signal an error.   Call only after started(), and after
            // a previous produce() is ready.  No functions may be called
            // after this.
            template<typename E>
            void set_exception(E ex) {
                _done.set_exception(ex);
            }

            friend class subscription<T...>;
        };

        template<typename... T>
        class subscription {
            stream<T...> *_stream;
            future<> _done;
            explicit subscription(stream<T...> *s) : _stream(s), _done(s->_done.get_future()) {
            }

        public:
            using next_fn = typename stream<T...>::next_fn;
            subscription(subscription &&x) : _stream(x._stream), _done(std::move(x._done)) {
                x._stream = nullptr;
            }

            /// \brief Start receiving events from the stream.
            ///
            /// \param next Callback to call for each event
            void start(next_fn next) {
                return _stream->start(std::move(next));
            }

            // Becomes ready when the stream is empty, or when an error
            // happens (in that case, an exception is held).
            future<> done() {
                return std::move(_done);
            }

            friend class stream<T...>;
        };

        template<typename... T>
        inline future<> stream<T...>::produce(T... data) {
            auto ret = futurize_invoke(_next, std::move(data)...);
            if (ret.available() && !ret.failed()) {
                // Native network stack depends on stream::produce() returning
                // a ready future to push packets along without dropping.  As
                // a temporary workaround, special case a ready, unfailed future
                // and return it immediately, so that then_wrapped(), below,
                // doesn't convert a ready future to an unready one.
                return ret;
            }
            return ret.then_wrapped([this](auto &&f) {
                try {
                    f.get();
                } catch (...) {
                    _done.set_exception(std::current_exception());
                    // FIXME: tell the producer to stop producing
                    throw;
                }
            });
        }
    }    // namespace actor
}    // namespace nil
