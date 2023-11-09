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

#include <boost/config.hpp>

#include <nil/actor/core/circular_buffer.hh>
#include <nil/actor/core/detail/io_request.hh>

namespace nil {
    namespace actor {

        class io_completion;

        namespace detail {

            class io_sink;

            class pending_io_request : private detail::io_request {
                friend class io_sink;
                io_completion *_completion;

            public:
                pending_io_request(detail::io_request req, io_completion *desc) noexcept :
                    io_request(std::move(req)), _completion(desc) {
                }
            };

            class io_sink {
                circular_buffer<pending_io_request> _pending_io;

            public:
                void submit(io_completion *desc, detail::io_request req) noexcept;

#ifdef BOOST_HAS_CONCEPTS
                // Fn should return whether the request was consumed and
                // draining should try to drain more
                template<typename Fn>
                requires std::is_invocable_r<bool, Fn, detail::io_request &, io_completion *>::value size_t
                    drain(Fn &&consume) {
#else
                // Fn should return whether the request was consumed and
                // draining should try to drain more
                template<typename Fn>
                size_t drain(Fn &&consume) {

#endif

                    size_t pending = _pending_io.size();
                    size_t drained = 0;

                    while (pending > drained) {
                        pending_io_request &req = _pending_io[drained];

                        if (!consume(req, req._completion)) {
                            break;
                        }
                        drained++;
                    }

                    _pending_io.erase(_pending_io.begin(), _pending_io.begin() + drained);
                    return drained;
                }
            };    // namespace detail

        }    // namespace detail

    }    // namespace actor
}    // namespace nil
