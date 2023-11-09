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

#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string.hpp>

#include <nil/actor/core/sstring.hh>
#include <nil/actor/rpc/rpc_types.hh>

namespace nil {
    namespace actor {

        namespace rpc {

            // This is meta compressor factory. It gets an array of regular factories that
            // support one compression algorithm each and negotiates common compression algorithm
            // that is supported both by a client and a server. The order of algorithm preferences
            // is the order they appear in clien's list
            class multi_algo_compressor_factory : public rpc::compressor::factory {
                std::vector<const rpc::compressor::factory *> _factories;
                sstring _features;

            public:
                multi_algo_compressor_factory(std::vector<const rpc::compressor::factory *> factories) :
                    _factories(std::move(factories)) {
                    _features = boost::algorithm::join(
                        _factories | boost::adaptors::transformed(std::mem_fn(&rpc::compressor::factory::supported)),
                        sstring(","));
                }
                multi_algo_compressor_factory(std::initializer_list<const rpc::compressor::factory *> factories) :
                    multi_algo_compressor_factory(std::vector<const rpc::compressor::factory *>(std::move(factories))) {
                }
                multi_algo_compressor_factory(const rpc::compressor::factory *factory) :
                    multi_algo_compressor_factory({factory}) {
                }
                // return feature string that will be sent as part of protocol negotiation
                virtual const sstring &supported() const {
                    return _features;
                }
                // negotiate compress algorithm
                virtual std::unique_ptr<compressor> negotiate(sstring feature, bool is_server) const {
                    std::vector<sstring> names;
                    boost::split(names, feature, boost::is_any_of(","));
                    std::unique_ptr<compressor> c;
                    if (is_server) {
                        for (auto &&n : names) {
                            for (auto &&f : _factories) {
                                if ((c = f->negotiate(n, is_server))) {
                                    return c;
                                }
                            }
                        }
                    } else {
                        for (auto &&f : _factories) {
                            for (auto &&n : names) {
                                if ((c = f->negotiate(n, is_server))) {
                                    return c;
                                }
                            }
                        }
                    }
                    return nullptr;
                }
            };

        }    // namespace rpc

    }    // namespace actor
}    // namespace nil
