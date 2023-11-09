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
#include <nil/actor/core/queue.hh>

#include <nil/actor/detail/std-compat.hh>

/// \defgroup fiber-module Fibers
///
/// \brief Fibers of execution
///
/// =nil; Actor continuations are normally short, but often chained to one
/// another, so that one continuation does a bit of work and then schedules
/// another continuation for later. Such chains can be long, and often even
/// involve loopings - see for example \ref repeat. We call such chains
/// "fibers" of execution.
///
/// These fibers are not threads - each is just a string of continuations -
/// but they share some common requirements with traditional threads.
/// For example, we want to avoid one fiber getting starved while a second
/// fiber continuously runs its continuations one after another.
/// As another example, fibers may want to communicate - e.g., one fiber
/// produces data that a second fiber consumes, and we wish to ensure that
/// both fibers get a chance to run, and that if one stops prematurely,
/// the other doesn't hang forever.
///
/// Consult the following table to see which APIs are useful for fiber tasks:
///
/// Task                                           | APIs
/// -----------------------------------------------|-------------------
/// Repeat a blocking task indefinitely            | \ref keep_doing()
/// Repeat a blocking task, then exit              | \ref repeat(), \ref do_until()
/// Provide mutual exclusion between two tasks     | \ref semaphore, \ref shared_mutex
/// Pass a stream of data between two fibers       | \ref nil::actor::pipe
/// Safely shut down a resource                    | \ref nil::actor::gate
/// Hold on to an object while a fiber is running  | \ref do_with()
///

/// =nil; Actor API namespace
namespace nil {
    namespace actor {

        /// \addtogroup fiber-module
        /// @{

        class broken_pipe_exception : public std::exception {
        public:
            virtual const char *what() const noexcept {
                return "Broken pipe";
            }
        };

        class unread_overflow_exception : public std::exception {
        public:
            virtual const char *what() const noexcept {
                return "pipe_reader::unread() overflow";
            }
        };

        /// \cond internal
        namespace detail {
            template<typename T>
            class pipe_buffer {
            private:
                queue<boost::optional<T>> _buf;
                bool _read_open = true;
                bool _write_open = true;

            public:
                pipe_buffer(size_t size) : _buf(size) {
                }
                future<boost::optional<T>> read() {
                    return _buf.pop_eventually();
                }
                future<> write(T &&data) {
                    return _buf.push_eventually(std::move(data));
                }
                bool readable() const {
                    return _write_open || !_buf.empty();
                }
                bool writeable() const {
                    return _read_open;
                }
                bool close_read() {
                    // If a writer blocking (on a full queue), need to stop it.
                    if (_buf.full()) {
                        _buf.abort(std::make_exception_ptr(broken_pipe_exception()));
                    }
                    _read_open = false;
                    return !_write_open;
                }
                bool close_write() {
                    // If the queue is empty, write the EOF (disengaged optional) to the
                    // queue to wake a blocked reader. If the queue is not empty, there is
                    // no need to write the EOF to the queue - the reader will return an
                    // EOF when it sees that _write_open == false.
                    if (_buf.empty()) {
                        _buf.push({});
                    }
                    _write_open = false;
                    return !_read_open;
                }
            };
        }    // namespace detail
        /// \endcond

        template<typename T>
        class pipe;

        /// \brief Read side of a \ref nil::actor::pipe
        ///
        /// The read side of a pipe, which allows only reading from the pipe.
        /// A pipe_reader object cannot be created separately, but only as part of a
        /// reader/writer pair through \ref nil::actor::pipe.
        template<typename T>
        class pipe_reader {
        private:
            detail::pipe_buffer<T> *_bufp;
            boost::optional<T> _unread;
            pipe_reader(detail::pipe_buffer<T> *bufp) : _bufp(bufp) {
            }
            friend class pipe<T>;

        public:
            /// \brief Read next item from the pipe
            ///
            /// Returns a future value, which is fulfilled when the pipe's buffer
            /// becomes non-empty, or the write side is closed. The value returned
            /// is an optional<T>, which is disengaged to mark and end of file
            /// (i.e., the write side was closed, and we've read everything it sent).
            future<boost::optional<T>> read() {
                if (_unread) {
                    auto ret = std::move(*_unread);
                    _unread = {};
                    return make_ready_future<boost::optional<T>>(std::move(ret));
                }
                if (_bufp->readable()) {
                    return _bufp->read();
                } else {
                    return make_ready_future<boost::optional<T>>();
                }
            }
            /// \brief Return an item to the front of the pipe
            ///
            /// Pushes the given item to the front of the pipe, so it will be
            /// returned by the next read() call. The typical use case is to
            /// unread() the last item returned by read().
            /// More generally, it is legal to unread() any item, not just one
            /// previously returned by read(), but note that the unread() is limited
            /// to just one item - two calls to unread() without an intervening call
            /// to read() will cause an exception.
            void unread(T &&item) {
                if (_unread) {
                    throw unread_overflow_exception();
                }
                _unread = std::move(item);
            }
            ~pipe_reader() {
                if (_bufp && _bufp->close_read()) {
                    delete _bufp;
                }
            }
            // Allow move, but not copy, of pipe_reader
            pipe_reader(pipe_reader &&other) : _bufp(other._bufp) {
                other._bufp = nullptr;
            }
            pipe_reader &operator=(pipe_reader &&other) {
                std::swap(_bufp, other._bufp);
            }
        };

        /// \brief Write side of a \ref nil::actor::pipe
        ///
        /// The write side of a pipe, which allows only writing to the pipe.
        /// A pipe_writer object cannot be created separately, but only as part of a
        /// reader/writer pair through \ref nil::actor::pipe.
        template<typename T>
        class pipe_writer {
        private:
            detail::pipe_buffer<T> *_bufp;
            pipe_writer(detail::pipe_buffer<T> *bufp) : _bufp(bufp) {
            }
            friend class pipe<T>;

        public:
            /// \brief Write an item to the pipe
            ///
            /// Returns a future value, which is fulfilled when the data was written
            /// to the buffer (when it become non-full). If the data could not be
            /// written because the read side was closed, an exception
            /// \ref broken_pipe_exception is returned in the future.
            future<> write(T &&data) {
                if (_bufp->writeable()) {
                    return _bufp->write(std::move(data));
                } else {
                    return make_exception_future<>(broken_pipe_exception());
                }
            }
            ~pipe_writer() {
                if (_bufp && _bufp->close_write()) {
                    delete _bufp;
                }
            }
            // Allow move, but not copy, of pipe_writer
            pipe_writer(pipe_writer &&other) : _bufp(other._bufp) {
                other._bufp = nullptr;
            }
            pipe_writer &operator=(pipe_writer &&other) {
                std::swap(_bufp, other._bufp);
            }
        };

        /// \brief A fixed-size pipe for communicating between two fibers.
        ///
        /// A pipe<T> is a mechanism to transfer data between two fibers, one
        /// producing data, and the other consuming it. The fixed-size buffer also
        /// ensures a balanced execution of the two fibers, because the producer
        /// fiber blocks when it writes to a full pipe, until the consumer fiber gets
        /// to run and read from the pipe.
        ///
        /// A pipe<T> resembles a Unix pipe, in that it has a read side, a write side,
        /// and a fixed-sized buffer between them, and supports either end to be closed
        /// independently (and EOF or broken pipe when using the other side).
        /// A pipe<T> object holds the reader and write sides of the pipe as two
        /// separate objects. These objects can be moved into two different fibers.
        /// Importantly, if one of the pipe ends is destroyed (i.e., the continuations
        /// capturing it end), the other end of the pipe will stop blocking, so the
        /// other fiber will not hang.
        ///
        /// The pipe's read and write interfaces are future-based blocking. I.e., the
        /// write() and read() methods return a future which is fulfilled when the
        /// operation is complete. The pipe is single-reader single-writer, meaning
        /// that until the future returned by read() is fulfilled, read() must not be
        /// called again (and same for write).
        ///
        /// Note: The pipe reader and writer are movable, but *not* copyable. It is
        /// often convenient to wrap each end in a shared pointer, so it can be
        /// copied (e.g., used in an std::function which needs to be copyable) or
        /// easily captured into multiple continuations.
        template<typename T>
        class pipe {
        public:
            pipe_reader<T> reader;
            pipe_writer<T> writer;
            explicit pipe(size_t size) : pipe(new detail::pipe_buffer<T>(size)) {
            }

        private:
            pipe(detail::pipe_buffer<T> *bufp) : reader(bufp), writer(bufp) {
            }
        };

        /// @}

    }    // namespace actor
}    // namespace nil
