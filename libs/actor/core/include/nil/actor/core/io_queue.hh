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
#include <nil/actor/core/fair_queue.hh>
#include <nil/actor/core/metrics_registration.hh>
#include <nil/actor/core/future.hh>
#include <nil/actor/core/detail/io_request.hh>

#include <mutex>
#include <array>

namespace nil {
    namespace actor {

        class io_priority_class;

        /// Renames an io priority class
        ///
        /// Renames an `io_priority_class` previously created with register_one_priority_class().
        ///
        /// The operation is global and affects all shards.
        /// The operation affects the exported statistics labels.
        ///
        /// \param pc The io priority class to be renamed
        /// \param new_name The new name for the io priority class
        /// \return a future that is ready when the io priority class have been renamed
        future<> rename_priority_class(io_priority_class pc, sstring new_name);

        class io_intent;

        namespace detail {
            class io_sink;
            namespace linux_abi {

                struct io_event;
                struct iocb;

            }    // namespace linux_abi
        }        // namespace detail

        using shard_id = unsigned;

        class io_priority_class;
        class io_queue;
        class io_desc_read_write;
        class queued_io_request;

        class io_group {
        public:
            struct config {
                unsigned max_req_count = std::numeric_limits<int>::max();
                unsigned max_bytes_count = std::numeric_limits<int>::max();
            };
            explicit io_group(config cfg) noexcept;

        private:
            friend class io_queue;
            fair_group _fg;
            const unsigned _maximum_request_size;

            static fair_group::config make_fair_group_config(config cfg) noexcept;
        };

        using io_group_ptr = std::shared_ptr<io_group>;
        struct priority_class_data;

        class io_queue {
        private:
            std::vector<std::vector<std::unique_ptr<priority_class_data>>> _priority_classes;
            io_group_ptr _group;
            fair_queue _fq;
            detail::io_sink &_sink;

            static constexpr unsigned _max_classes = 2048;
            static std::mutex _register_lock;
            static std::array<uint32_t, _max_classes> _registered_shares;
            static std::array<sstring, _max_classes> _registered_names;

        public:
            static io_priority_class register_one_priority_class(sstring name, uint32_t shares);
            static bool rename_one_priority_class(io_priority_class pc, sstring name);

        private:
            priority_class_data &find_or_create_class(const io_priority_class &pc, shard_id owner);

            // The fields below are going away, they are just here so we can implement deprecated
            // functions that used to be provided by the fair_queue and are going away (from both
            // the fair_queue and the io_queue). Double-accounting for now will allow for easier
            // decoupling and is temporary
            size_t _queued_requests = 0;
            size_t _requests_executing = 0;

        public:
            // We want to represent the fact that write requests are (maybe) more expensive
            // than read requests. To avoid dealing with floating point math we will scale one
            // read request to be counted by this amount.
            //
            // A write request that is 30% more expensive than a read will be accounted as
            // (read_request_base_count * 130) / 100.
            // It is also technically possible for reads to be the expensive ones, in which case
            // writes will have an integer value lower than read_request_base_count.
            static constexpr unsigned read_request_base_count = 128;
            static constexpr unsigned request_ticket_size_shift = 9;
            static constexpr unsigned minimal_request_size = 512;

            struct config {
                dev_t devid;
                unsigned capacity = std::numeric_limits<unsigned>::max();
                unsigned disk_req_write_to_read_multiplier = read_request_base_count;
                unsigned disk_bytes_write_to_read_multiplier = read_request_base_count;
                float disk_us_per_request = 0;
                float disk_us_per_byte = 0;
                sstring mountpoint = "undefined";
            };

            io_queue(io_group_ptr group, detail::io_sink &sink, config cfg);
            ~io_queue();

            fair_queue_ticket request_fq_ticket(const detail::io_request &req, size_t len) const;

            future<size_t> queue_request(const io_priority_class &pc, size_t len, detail::io_request req,
                                         io_intent *intent) noexcept;
            void submit_request(io_desc_read_write *desc, detail::io_request req, priority_class_data &pclass) noexcept;
            void cancel_request(queued_io_request &req, priority_class_data &pclass) noexcept;
            void complete_cancelled_request(queued_io_request &req) noexcept;

            [[deprecated("modern I/O queues should use a property file")]] size_t capacity() const {
                return _config.capacity;
            }

            [[deprecated(
                "I/O queue users should not track individual requests, but resources (weight, size) passing through "
                "the "
                "queue")]] size_t
                queued_requests() const {
                return _queued_requests;
            }

            // How many requests are sent to disk but not yet returned.
            [[deprecated(
                "I/O queue users should not track individual requests, but resources (weight, size) passing through "
                "the "
                "queue")]] size_t
                requests_currently_executing() const {
                return _requests_executing;
            }

            void notify_requests_finished(fair_queue_ticket &desc) noexcept;

            // Dispatch requests that are pending in the I/O queue
            void poll_io_queue();

            std::chrono::steady_clock::time_point next_pending_aio() const noexcept {
                return _fq.next_pending_aio();
            }

            sstring mountpoint() const {
                return _config.mountpoint;
            }

            dev_t dev_id() const noexcept {
                return _config.devid;
            }

            future<> update_shares_for_class(io_priority_class pc, size_t new_shares);
            void rename_priority_class(io_priority_class pc, sstring new_name);

        private:
            config _config;
            static fair_queue::config make_fair_queue_config(config cfg);
        };

    }    // namespace actor
}    // namespace nil
