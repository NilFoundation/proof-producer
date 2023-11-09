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

#include <nil/actor/core/smp.hh>
#include <nil/actor/core/loop.hh>
#include <nil/actor/core/semaphore.hh>
#include <nil/actor/core/print.hh>

#include <boost/range/algorithm/find_if.hpp>

#include <vector>

namespace nil {
    namespace actor {

        void smp_message_queue::work_item::process() {
            schedule(this);
        }

        struct smp_service_group_impl {
            std::vector<smp_service_group_semaphore> clients;    // one client per server shard
        };

        static smp_service_group_semaphore smp_service_group_management_sem {
            1, named_semaphore_exception_factory {"smp_service_group_management_sem"}};
        static thread_local std::vector<smp_service_group_impl> smp_service_groups;

        static named_semaphore_exception_factory
            make_service_group_semaphore_exception_factory(unsigned id, shard_id client_cpu, shard_id this_cpu,
                                                           boost::optional<sstring> smp_group_name) {
            if (smp_group_name) {
                return named_semaphore_exception_factory {
                    format("smp_service_group:'{}' (#{}) {}->{} semaphore", *smp_group_name, id, client_cpu, this_cpu)};
            } else {
                return named_semaphore_exception_factory {
                    format("smp_service_group#{} {}->{} semaphore", id, client_cpu, this_cpu)};
            }
        }

        static_assert(std::is_nothrow_copy_constructible<smp_service_group>::value);
        static_assert(std::is_nothrow_move_constructible<smp_service_group>::value);

        static_assert(std::is_nothrow_default_constructible<smp_submit_to_options>::value);
        static_assert(std::is_nothrow_copy_constructible<smp_submit_to_options>::value);
        static_assert(std::is_nothrow_move_constructible<smp_submit_to_options>::value);

        future<smp_service_group> create_smp_service_group(smp_service_group_config ssgc) noexcept {
            ssgc.max_nonlocal_requests = std::max(ssgc.max_nonlocal_requests, smp::count - 1);
            return smp::submit_to(0, [ssgc] {
                return with_semaphore(smp_service_group_management_sem, 1, [ssgc] {
                    auto it = boost::range::find_if(smp_service_groups,
                                                    [&](smp_service_group_impl &ssgi) { return ssgi.clients.empty(); });
                    size_t id = it - smp_service_groups.begin();
                    return parallel_for_each(smp::all_cpus(),
                                             [ssgc, id](unsigned cpu) {
                                                 return smp::submit_to(cpu, [ssgc, id, cpu] {
                                                     if (id >= smp_service_groups.size()) {
                                                         smp_service_groups.resize(id + 1);    // may throw
                                                     }
                                                     smp_service_groups[id].clients.reserve(smp::count);    // may throw
                                                     auto per_client = smp::count > 1 ? ssgc.max_nonlocal_requests /
                                                                                            (smp::count - 1) :
                                                                                        0u;
                                                     for (unsigned i = 0; i != smp::count; ++i) {
                                                         smp_service_groups[id].clients.emplace_back(
                                                             per_client, make_service_group_semaphore_exception_factory(
                                                                             id, i, cpu, ssgc.group_name));
                                                     }
                                                 });
                                             })
                        .handle_exception([id](std::exception_ptr e) {
                            // rollback
                            return smp::invoke_on_all([id] {
                                       if (smp_service_groups.size() > id) {
                                           smp_service_groups[id].clients.clear();
                                       }
                                   })
                                .then([e = std::move(e)]() mutable { std::rethrow_exception(std::move(e)); });
                        })
                        .then([id] { return smp_service_group(id); });
                });
            });
        }

        future<> destroy_smp_service_group(smp_service_group ssg) noexcept {
            return smp::submit_to(0, [ssg] {
                return with_semaphore(smp_service_group_management_sem, 1, [ssg] {
                    auto id = detail::smp_service_group_id(ssg);
                    return smp::invoke_on_all([id] { smp_service_groups[id].clients.clear(); });
                });
            });
        }

        void init_default_smp_service_group(shard_id cpu) {
            smp_service_groups.emplace_back();
            auto &ssg0 = smp_service_groups.back();
            ssg0.clients.reserve(smp::count);
            for (unsigned i = 0; i != smp::count; ++i) {
                ssg0.clients.emplace_back(smp_service_group_semaphore::max_counter(),
                                          make_service_group_semaphore_exception_factory(
                                              0, i, cpu, boost::make_optional<sstring>("default")));
            }
        }

        smp_service_group_semaphore &get_smp_service_groups_semaphore(unsigned ssg_id, shard_id t) noexcept {
            return smp_service_groups[ssg_id].clients[t];
        }
    }    // namespace actor
}    // namespace nil
