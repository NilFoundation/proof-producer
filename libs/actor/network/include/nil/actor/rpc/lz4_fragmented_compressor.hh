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

#include <nil/actor/core/sstring.hh>
#include <nil/actor/rpc/rpc_types.hh>

namespace nil {
    namespace actor {
        namespace rpc {

            class lz4_fragmented_compressor final : public compressor {
            public:
                class factory final : public rpc::compressor::factory {
                public:
                    virtual const sstring &supported() const override;
                    virtual std::unique_ptr<rpc::compressor> negotiate(sstring feature, bool is_server) const override;
                };

            public:
                virtual snd_buf compress(size_t head_space, snd_buf data) override;
                virtual rcv_buf decompress(rcv_buf data) override;
                sstring name() const override;
            };

        }    // namespace rpc
    }        // namespace actor
}    // namespace nil
