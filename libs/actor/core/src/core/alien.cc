// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
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

#include <nil/actor/core/alien.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/metrics.hh>
#include <nil/actor/core/prefetch.hh>

namespace nil {
    namespace actor {
        namespace alien {

            message_queue::message_queue(reactor *to) : _pending(to) {
            }

            void message_queue::stop() {
                _metrics.clear();
            }

            void message_queue::lf_queue::maybe_wakeup() {
                // see also smp_message_queue::lf_queue::maybe_wakeup()
                std::atomic_signal_fence(std::memory_order_seq_cst);
                if (remote->_sleeping.load(std::memory_order_relaxed)) {
                    remote->_sleeping.store(false, std::memory_order_relaxed);
                    remote->wakeup();
                }
            }

            void message_queue::submit_item(std::unique_ptr<message_queue::work_item> item) {
                if (!_pending.push(item.get())) {
                    throw std::bad_alloc();
                }
                item.release();
                _pending.maybe_wakeup();
                ++_sent.value;
            }

            bool message_queue::pure_poll_rx() const {
                return !_pending.empty();
            }

            template<typename Func>
            size_t message_queue::process_queue(lf_queue &q, Func process) {
                // copy batch to local memory in order to minimize
                // time in which cross-cpu data is accessed
                work_item *wi;
                if (!q.pop(wi)) {
                    return 0;
                }
                work_item *items[batch_size + prefetch_cnt];
                // start prefetching first item before popping the rest to overlap memory
                // access with potential cache miss the second pop may cause
                prefetch<2>(wi);
                size_t nr = 0;
                while (nr < batch_size && q.pop(items[nr])) {
                    ++nr;
                }
                std::fill(std::begin(items) + nr, std::begin(items) + nr + prefetch_cnt, nr ? items[nr - 1] : wi);
                unsigned i = 0;
                do {
                    prefetch_n<2>(std::begin(items) + i, std::begin(items) + i + prefetch_cnt);
                    process(wi);
                    wi = items[i++];
                } while (i <= nr);

                return nr + 1;
            }

            size_t message_queue::process_incoming() {
                if (_pending.empty()) {
                    return 0;
                }
                auto nr = process_queue(_pending, [](work_item *wi) {
                    wi->process();
                    delete wi;
                });
                _received += nr;
                _last_rcv_batch = nr;
                return nr;
            }

            void message_queue::start() {
                namespace sm = nil::actor::metrics;
                char instance[10];
                std::snprintf(instance, sizeof(instance), "%u", this_shard_id());
                _metrics.add_group("alien",
                                   {
                                       // Absolute value of num packets in last tx batch.
                                       sm::make_queue_length("receive_batch_queue_length", _last_rcv_batch,
                                                             sm::description("Current receive batch queue length")),
                                       // total_operations value:DERIVE:0:U
                                       sm::make_derive("total_received_messages", _received,
                                                       sm::description("Total number of received messages")),
                                       // total_operations value:DERIVE:0:U
                                       sm::make_derive(
                                           "total_sent_messages", [this] { return _sent.value.load(); },
                                           sm::description("Total number of sent messages")),
                                   });
            }

            void smp::qs_deleter::operator()(alien::message_queue *qs) const {
                for (unsigned i = 0; i < count; i++) {
                    qs[i].~message_queue();
                }
                ::operator delete[](qs);
            }

            smp::qs smp::_qs;

            smp::qs smp::create_qs(const std::vector<reactor *> &reactors) {
                auto queues = reinterpret_cast<alien::message_queue *>(operator new[](sizeof(alien::message_queue) *
                                                                                      reactors.size()));
                for (unsigned i = 0; i < reactors.size(); i++) {
                    new (&queues[i]) alien::message_queue(reactors[i]);
                }
                return qs {queues, smp::qs_deleter {static_cast<unsigned>(reactors.size())}};
            }

            bool smp::poll_queues() {
                auto &queue = _qs[this_shard_id()];
                return queue.process_incoming() != 0;
            }

            bool smp::pure_poll_queues() {
                auto &queue = _qs[this_shard_id()];
                return queue.pure_poll_rx();
            }
        }    // namespace alien
    }        // namespace actor
}    // namespace nil
