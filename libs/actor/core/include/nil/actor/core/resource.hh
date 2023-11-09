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

#include <cassert>
#include <cstdlib>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <unordered_map>

#include <sched.h>

#include <boost/any.hpp>
#include <boost/optional.hpp>

#include <nil/actor/detail/spinlock.hh>
#include <nil/actor/detail/thread_affinity.hh>

namespace nil {
    namespace actor {

        cpu_set_t cpuid_to_cpuset(unsigned cpuid);
        class io_queue;
        class io_group;

        namespace resource {

            using boost::optional;

            using cpuset = std::set<unsigned>;

            struct configuration {
                optional<size_t> total_memory;
                optional<size_t> reserve_memory;    // if total_memory not specified
                optional<size_t> cpus;
                optional<cpuset> cpu_set;
                size_t shard0scale;  // The ratio of how much the zero shard memory is larger than the rest
                bool assign_orphan_cpus = false;
                std::vector<dev_t> devices;
                unsigned num_io_groups;
            };

            struct memory {
                size_t bytes;
                unsigned nodeid;
            };

            // Since this is static information, we will keep a copy at each CPU.
            // This will allow us to easily find who is the IO coordinator for a given
            // node without a trip to a remote CPU.
            struct io_queue_topology {
                unsigned nr_queues;
                std::vector<unsigned> shard_to_group;
                unsigned nr_groups;
            };

            struct cpu {
                unsigned cpu_id;
                std::vector<memory> mem;
            };

            struct resources {
                std::vector<cpu> cpus;
                std::unordered_map<dev_t, io_queue_topology> ioq_topology;
            };

            struct device_io_topology {
                std::vector<io_queue *> queues;
                struct group {
                    std::shared_ptr<io_group> g;
                    unsigned attached = 0;
                };
                util::spinlock lock;
                std::vector<group> groups;

                device_io_topology() noexcept = default;
                device_io_topology(const io_queue_topology &iot) noexcept :
                    queues(iot.nr_queues), groups(iot.nr_groups) {
                }
            };

            resources allocate(configuration c);
            unsigned nr_processing_units();
        }    // namespace resource

        // We need a wrapper class, because boost::program_options wants validate()
        // (below) to be in the same namespace as the type it is validating.
        struct cpuset_bpo_wrapper {
            resource::cpuset value;
        };

        // Overload for boost program options parsing/validation
        extern void validate(boost::any &v, const std::vector<std::string> &values, cpuset_bpo_wrapper *target_type,
                             int);

    }    // namespace actor
}    // namespace nil
