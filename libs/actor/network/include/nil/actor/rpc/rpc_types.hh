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

#include <nil/actor/network/api.hh>
#include <stdexcept>
#include <string>
#include <boost/any.hpp>
#include <boost/type.hpp>
#include <nil/actor/detail/std-compat.hh>
#include <nil/actor/detail/variant_utils.hh>
#include <nil/actor/core/timer.hh>
#include <nil/actor/core/circular_buffer.hh>
#include <nil/actor/core/simple_stream.hh>
#include <nil/actor/core/lowres_clock.hh>
#include <boost/functional/hash.hpp>
#include <nil/actor/core/sharded.hh>

namespace nil {
    namespace actor {

        namespace rpc {

            using rpc_clock_type = lowres_clock;

            // used to tag a type for serializers
            template<typename T>
            using type = boost::type<T>;

            struct stats {
                using counter_type = uint64_t;
                counter_type replied = 0;
                counter_type pending = 0;
                counter_type exception_received = 0;
                counter_type sent_messages = 0;
                counter_type wait_reply = 0;
                counter_type timeout = 0;
            };

            struct client_info {
                socket_address addr;
                std::unordered_map<sstring, boost::any> user_data;
                template<typename T>
                void attach_auxiliary(const sstring &key, T &&object) {
                    user_data.emplace(key, boost::any(std::forward<T>(object)));
                }
                template<typename T>
                T &retrieve_auxiliary(const sstring &key) {
                    auto it = user_data.find(key);
                    assert(it != user_data.end());
                    return boost::any_cast<T &>(it->second);
                }
                template<typename T>
                typename std::add_const<T>::type &retrieve_auxiliary(const sstring &key) const {
                    return const_cast<client_info *>(this)->retrieve_auxiliary<typename std::add_const<T>::type>(key);
                }
            };

            class error : public std::runtime_error {
            public:
                error(const std::string &msg) : std::runtime_error(msg) {
                }
            };

            class closed_error : public error {
            public:
                closed_error() : error("connection is closed") {
                }
            };

            class timeout_error : public error {
            public:
                timeout_error() : error("rpc call timed out") {
                }
            };

            class unknown_verb_error : public error {
            public:
                uint64_t type;
                unknown_verb_error(uint64_t type_) : error("unknown verb"), type(type_) {
                }
            };

            class unknown_exception_error : public error {
            public:
                unknown_exception_error() : error("unknown exception") {
                }
            };

            class rpc_protocol_error : public error {
            public:
                rpc_protocol_error() : error("rpc protocol exception") {
                }
            };

            class canceled_error : public error {
            public:
                canceled_error() : error("rpc call was canceled") {
                }
            };

            class stream_closed : public error {
            public:
                stream_closed() : error("rpc stream was closed by peer") {
                }
            };

            struct no_wait_type { };

            // return this from a callback if client does not want to waiting for a reply
            extern no_wait_type no_wait;

            /// \addtogroup rpc
            /// @{

            template<typename T>
            class optional : public boost::optional<T> {
            public:
                using boost::optional<T>::optional;
            };

            class opt_time_point : public boost::optional<rpc_clock_type::time_point> {
            public:
                using boost::optional<rpc_clock_type::time_point>::optional;
                opt_time_point(boost::optional<rpc_clock_type::time_point> time_point) {
                    static_cast<boost::optional<rpc_clock_type::time_point> &>(*this) = time_point;
                }
            };

            /// @}

            struct cancellable {
                std::function<void()> cancel_send;
                std::function<void()> cancel_wait;
                cancellable **send_back_pointer = nullptr;
                cancellable **wait_back_pointer = nullptr;
                cancellable() = default;
                cancellable(cancellable &&x) :
                    cancel_send(std::move(x.cancel_send)), cancel_wait(std::move(x.cancel_wait)),
                    send_back_pointer(x.send_back_pointer), wait_back_pointer(x.wait_back_pointer) {
                    if (send_back_pointer) {
                        *send_back_pointer = this;
                        x.send_back_pointer = nullptr;
                    }
                    if (wait_back_pointer) {
                        *wait_back_pointer = this;
                        x.wait_back_pointer = nullptr;
                    }
                }
                cancellable &operator=(cancellable &&x) {
                    if (&x != this) {
                        this->~cancellable();
                        new (this) cancellable(std::move(x));
                    }
                    return *this;
                }
                void cancel() {
                    if (cancel_send) {
                        cancel_send();
                    }
                    if (cancel_wait) {
                        cancel_wait();
                    }
                }
                ~cancellable() {
                    cancel();
                }
            };

            struct rcv_buf {
                uint32_t size = 0;
                boost::optional<semaphore_units<>> su;
                std::variant<std::vector<temporary_buffer<char>>, temporary_buffer<char>> bufs;
                using iterator = std::vector<temporary_buffer<char>>::iterator;
                rcv_buf() {
                }
                explicit rcv_buf(size_t size_) : size(size_) {
                }
                explicit rcv_buf(temporary_buffer<char> b) : size(b.size()), bufs(std::move(b)) {};
                explicit rcv_buf(std::vector<temporary_buffer<char>> bufs, size_t size) :
                    size(size), bufs(std::move(bufs)) {};
            };

            struct snd_buf {
                // Preferred, but not required, chunk size.
                static constexpr size_t chunk_size = 128 * 1024;
                uint32_t size = 0;
                std::variant<std::vector<temporary_buffer<char>>, temporary_buffer<char>> bufs;
                using iterator = std::vector<temporary_buffer<char>>::iterator;
                snd_buf() {
                }
                snd_buf(snd_buf &&) noexcept;
                snd_buf &operator=(snd_buf &&) noexcept;
                explicit snd_buf(size_t size_);
                explicit snd_buf(temporary_buffer<char> b) : size(b.size()), bufs(std::move(b)) {};

                explicit snd_buf(std::vector<temporary_buffer<char>> bufs, size_t size) :
                    size(size), bufs(std::move(bufs)) {};

                temporary_buffer<char> &front();
            };

            static inline memory_input_stream<rcv_buf::iterator> make_deserializer_stream(rcv_buf &input) {
                auto *b = std::get_if<temporary_buffer<char>>(&input.bufs);
                if (b) {
                    return memory_input_stream<rcv_buf::iterator>(
                        memory_input_stream<rcv_buf::iterator>::simple(b->begin(), b->size()));
                } else {
                    auto &ar = std::get<std::vector<temporary_buffer<char>>>(input.bufs);
                    return memory_input_stream<rcv_buf::iterator>(
                        memory_input_stream<rcv_buf::iterator>::fragmented(ar.begin(), input.size));
                }
            }

            class compressor {
            public:
                virtual ~compressor() {
                }
                // compress data and leave head_space bytes at the beginning of returned buffer
                virtual snd_buf compress(size_t head_space, snd_buf data) = 0;
                // decompress data
                virtual rcv_buf decompress(rcv_buf data) = 0;
                virtual sstring name() const = 0;

                // factory to create compressor for a connection
                class factory {
                public:
                    virtual ~factory() {
                    }
                    // return feature string that will be sent as part of protocol negotiation
                    virtual const sstring &supported() const = 0;
                    // negotiate compress algorithm
                    virtual std::unique_ptr<compressor> negotiate(sstring feature, bool is_server) const = 0;
                };
            };

            class connection;

            struct connection_id {
                uint64_t id;
                bool operator==(const connection_id &o) const {
                    return id == o.id;
                }
                operator bool() const {
                    return shard() != 0xffff;
                }
                size_t shard() const {
                    return size_t(id & 0xffff);
                }
                constexpr static connection_id make_invalid_id(uint64_t id = 0) {
                    return make_id(id, 0xffff);
                }
                constexpr static connection_id make_id(uint64_t id, uint16_t shard) {
                    return {id << 16 | shard};
                }
            };

            constexpr connection_id invalid_connection_id = connection_id::make_invalid_id();

            std::ostream &operator<<(std::ostream &, const connection_id &);

            using xshard_connection_ptr = lw_shared_ptr<foreign_ptr<shared_ptr<connection>>>;
            constexpr size_t max_queued_stream_buffers = 50;
            constexpr size_t max_stream_buffers_memory = 100 * 1024;

            /// \addtogroup rpc
            /// @{

            // send data Out...
            template<typename... Out>
            class sink {
            public:
                class impl {
                protected:
                    xshard_connection_ptr _con;
                    semaphore _sem;
                    std::exception_ptr _ex;
                    impl(xshard_connection_ptr con) : _con(std::move(con)), _sem(max_stream_buffers_memory) {
                    }

                public:
                    virtual ~impl() {};
                    virtual future<> operator()(const Out &...args) = 0;
                    virtual future<> close() = 0;
                    virtual future<> flush() = 0;
                    friend sink;
                };

            private:
                shared_ptr<impl> _impl;

            public:
                sink(shared_ptr<impl> impl) : _impl(std::move(impl)) {
                }
                future<> operator()(const Out &...args) {
                    return _impl->operator()(args...);
                }
                future<> close() {
                    return _impl->close();
                }
                // Calling this function makes sure that any data buffered
                // by the stream sink will be flushed to the network.
                // It does not mean the data was received by the corresponding
                // source.
                future<> flush() {
                    return _impl->flush();
                }
                connection_id get_id() const;
            };

            // receive data In...
            template<typename... In>
            class source {
            public:
                class impl {
                protected:
                    xshard_connection_ptr _con;
                    circular_buffer<foreign_ptr<std::unique_ptr<rcv_buf>>> _bufs;
                    impl(xshard_connection_ptr con) : _con(std::move(con)) {
                        _bufs.reserve(max_queued_stream_buffers);
                    }

                public:
                    virtual ~impl() {
                    }
                    virtual future<boost::optional<std::tuple<In...>>> operator()() = 0;
                    friend source;
                };

            private:
                shared_ptr<impl> _impl;

            public:
                source(shared_ptr<impl> impl) : _impl(std::move(impl)) {
                }
                future<boost::optional<std::tuple<In...>>> operator()() {
                    return _impl->operator()();
                };
                connection_id get_id() const;
                template<typename Serializer, typename... Out>
                sink<Out...> make_sink();
            };

            /// Used to return multiple values in rpc without variadic futures
            ///
            /// If you wish to return multiple values from an rpc procedure, use a
            /// signature `future<rpc::tuple<return type list> (argument list)>>`. This
            /// will be marshalled by rpc, so you do not need to have your Serializer
            /// serialize/deserialize this tuple type. The serialization format is
            /// compatible with the deprecated variadic future support, and is compatible
            /// with adding new return types in a backwards compatible way provided new
            /// parameters are appended only, and wrapped with rpc::optional:
            /// `future<rpc::tuple<existing return type list, rpc::optional<new_return_type>>> (argument list)`
            ///
            /// You may also use another tuple type, such as std::tuple. In this case,
            /// your Serializer type must recognize your tuple type and provide serialization
            /// and deserialization for it.
            template<typename... T>
            class tuple : public std::tuple<T...> {
            public:
                using std::tuple<T...>::tuple;
                tuple(std::tuple<T...> &&x) : std::tuple<T...>(std::move(x)) {
                }
            };

            /// @}

            template<typename... T>
            tuple(T &&...) -> tuple<T...>;

        }    // namespace rpc
    }        // namespace actor
}    // namespace nil

namespace std {
    template<>
    struct hash<nil::actor::rpc::connection_id> {
        size_t operator()(const nil::actor::rpc::connection_id &id) const {
            size_t h = 0;
            boost::hash_combine(h, std::hash<uint64_t> {}(id.id));
            return h;
        }
    };

    template<typename... T>
    struct tuple_size<nil::actor::rpc::tuple<T...>> : tuple_size<tuple<T...>> { };

    template<size_t I, typename... T>
    struct tuple_element<I, nil::actor::rpc::tuple<T...>> : tuple_element<I, tuple<T...>> { };

}    // namespace std
