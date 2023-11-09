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
//
#pragma once

#ifdef ACTOR_HAVE_DPDK

#include <bitset>
#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_version.h>
#include <boost/program_options.hpp>

/*********************** Compat section ***************************************/
// We currently support only versions 2.0 and above.
#if (RTE_VERSION < RTE_VERSION_NUM(2, 0, 0, 0))
#error "DPDK version above 2.0.0 is required"
#endif

#if defined(RTE_MBUF_REFCNT_ATOMIC)
#warning \
    "CONFIG_RTE_MBUF_REFCNT_ATOMIC should be disabled in DPDK's " \
         "config/common_linuxapp"
#endif
/******************************************************************************/

namespace nil {
    namespace actor {

        namespace dpdk {

            // DPDK Environment Abstraction Layer
            class eal {
            public:
                using cpuset = std::bitset<RTE_MAX_LCORE>;

                static void init(cpuset cpus, boost::program_options::variables_map opts);
                /**
                 * Returns the amount of memory needed for DPDK
                 * @param num_cpus Number of CPUs the application is going to use
                 *
                 * @return
                 */
                static size_t mem_size(int num_cpus, bool hugetlbfs_membackend = true);
                static bool initialized;
            };

        }    // namespace dpdk

    }    // namespace actor
}    // namespace nil

#endif    // ACTOR_HAVE_DPDK
