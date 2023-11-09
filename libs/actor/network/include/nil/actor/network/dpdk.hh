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

#ifdef ACTOR_HAVE_DPDK

#include <memory>

#include <nil/actor/network/config.hh>
#include <nil/actor/network/net.hh>
#include <nil/actor/core/sstring.hh>

namespace nil {
    namespace actor {

        std::unique_ptr<net::device> create_dpdk_net_device(uint16_t port_idx = 0,
                                                            uint16_t num_queues = 1,
                                                            bool use_lro = true,
                                                            bool enable_fc = true);

        std::unique_ptr<net::device> create_dpdk_net_device(const net::hw_config &hw_cfg);

        boost::program_options::options_description get_dpdk_net_options_description();

        namespace dpdk {
            /**
             * @return Number of bytes needed for mempool objects of each QP.
             */
            uint32_t qp_mempool_obj_size(bool hugetlbfs_membackend);
        }    // namespace dpdk

    }    // namespace actor
}    // namespace nil

#endif    // ACTOR_HAVE_DPDK
