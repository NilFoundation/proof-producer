//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#ifdef ACTOR_HAVE_DPDK

#include <cinttypes>
#include <nil/actor/network/dpdk.hh>
#include <nil/actor/core/dpdk_rte.hh>
#include <nil/actor/detail/conversions.hh>
#include <nil/actor/detail/std-compat.hh>
#include <rte_pci.h>

namespace nil {
    namespace actor {

        namespace dpdk {

            bool eal::initialized = false;

            void eal::init(cpuset cpus, boost::program_options::variables_map opts) {
                if (initialized) {
                    return;
                }

                std::stringstream mask;
                cpuset nibble = 0xF;
                while (cpus.any()) {
                    mask << std::hex << (cpus & nibble).to_ulong();
                    cpus >>= 4;
                }

                std::string mask_str = mask.str();
                std::reverse(mask_str.begin(), mask_str.end());

                // TODO: Inherit these from the app parameters - "opts"
                std::vector<std::vector<char>> args {string2vector(opts["argv0"].as<std::string>()),
                                                     string2vector("-c"), string2vector(mask_str), string2vector("-n"),
                                                     string2vector("1")};

                boost::optional<std::string> hugepages_path;
                if (opts.count("hugepages")) {
                    hugepages_path = opts["hugepages"].as<std::string>();
                }

                // If "hugepages" is not provided and DPDK PMD drivers mode is requested -
                // use the default DPDK huge tables configuration.
                if (hugepages_path) {
                    args.push_back(string2vector("--huge-dir"));
                    args.push_back(string2vector(hugepages_path.value()));

                    //
                    // We don't know what is going to be our networking configuration so we
                    // assume there is going to be a queue per-CPU. Plus we'll give a DPDK
                    // 64MB for "other stuff".
                    //
                    size_t size_MB = mem_size(cpus.count()) >> 20;
                    std::stringstream size_MB_str;
                    size_MB_str << size_MB;

                    args.push_back(string2vector("-m"));
                    args.push_back(string2vector(size_MB_str.str()));
                } else if (!opts.count("dpdk-pmd")) {
                    args.push_back(string2vector("--no-huge"));
                }
#ifdef HAVE_OSV
                args.push_back(string2vector("--no-shconf"));
#endif

                std::vector<char *> cargs;

                for (auto &&a : args) {
                    cargs.push_back(a.data());
                }
                /* initialise the EAL for all */
                int ret = rte_eal_init(cargs.size(), cargs.data());
                if (ret < 0) {
                    rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
                }

                initialized = true;
            }

            uint32_t __attribute__((weak)) qp_mempool_obj_size(bool hugetlbfs_membackend) {
                return 0;
            }

            size_t eal::mem_size(int num_cpus, bool hugetlbfs_membackend) {
                size_t memsize = 0;
                //
                // PMD mempool memory:
                //
                // We don't know what is going to be our networking configuration so we
                // assume there is going to be a queue per-CPU.
                //
                memsize += num_cpus * qp_mempool_obj_size(hugetlbfs_membackend);

                // Plus we'll give a DPDK 64MB for "other stuff".
                memsize += (64UL << 20);

                return memsize;
            }

        }    // namespace dpdk

    }    // namespace actor
}    // namespace nil

#endif    // ACTOR_HAVE_DPDK
