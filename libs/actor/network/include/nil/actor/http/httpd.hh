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

#include <nil/actor/http/request_parser.hh>
#include <nil/actor/http/request.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/sstring.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/circular_buffer.hh>
#include <nil/actor/core/distributed.hh>
#include <nil/actor/core/queue.hh>
#include <nil/actor/core/gate.hh>
#include <nil/actor/core/metrics_registration.hh>
#include <nil/actor/detail/std-compat.hh>

#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <bitset>
#include <limits>
#include <cctype>
#include <vector>

#include <boost/intrusive/list.hpp>

#include <nil/actor/http/routes.hh>
#include <nil/actor/network/tls.hh>
#include <nil/actor/core/shared_ptr.hh>

namespace nil {
    namespace actor {

        namespace httpd {

            class http_server;
            class http_stats;
            struct reply;

            using namespace std::chrono_literals;

            class http_stats {
                metrics::metric_groups _metric_groups;

            public:
                http_stats(http_server &server, const sstring &name);
            };

            class connection : public boost::intrusive::list_base_hook<> {
                http_server &_server;
                connected_socket _fd;
                input_stream<char> _read_buf;
                output_stream<char> _write_buf;
                static constexpr size_t limit = 4096;
                using tmp_buf = temporary_buffer<char>;
                http_request_parser _parser;
                std::unique_ptr<request> _req;
                std::unique_ptr<reply> _resp;
                // null element marks eof
                queue<std::unique_ptr<reply>> _replies {10};
                bool _done = false;

            public:
                connection(http_server &server, connected_socket &&fd, socket_address addr) :
                    _server(server), _fd(std::move(fd)), _read_buf(_fd.input()), _write_buf(_fd.output()) {
                    on_new_connection();
                }
                ~connection();
                void on_new_connection();

                future<> process();
                void shutdown();
                future<> read();
                future<> read_one();
                future<> respond();
                future<> do_response_loop();

                void set_headers(reply &resp);

                future<> start_response();
                future<> write_reply_headers(std::unordered_map<sstring, sstring>::iterator hi);

                static short hex_to_byte(char c);

                /**
                 * Convert a hex encoded 2 bytes substring to char
                 */
                static char hexstr_to_char(const std::string_view &in, size_t from);

                /**
                 * URL_decode a substring and place it in the given out sstring
                 */
                static bool url_decode(const std::string_view &in, sstring &out);

                /**
                 * Add a single query parameter to the parameter list
                 */
                static void add_param(request &req, const std::string_view &param);

                /**
                 * Set the query parameters in the request objects.
                 * query param appear after the question mark and are separated
                 * by the ampersand sign
                 */
                static sstring set_query_param(request &req);

                future<bool> generate_reply(std::unique_ptr<request> req);
                void generate_error_reply_and_close(std::unique_ptr<request> req, reply::status_type status,
                                                    const sstring &msg);

                future<> write_body();

                output_stream<char> &out();
            };

            class http_server_tester;

            class http_server {
                std::vector<server_socket> _listeners;
                http_stats _stats;
                uint64_t _total_connections = 0;
                uint64_t _current_connections = 0;
                uint64_t _requests_served = 0;
                uint64_t _read_errors = 0;
                uint64_t _respond_errors = 0;
                shared_ptr<nil::actor::tls::server_credentials> _credentials;
                sstring _date = http_date();
                timer<> _date_format_timer {[this] { _date = http_date(); }};
                size_t _content_length_limit = std::numeric_limits<size_t>::max();
                gate _task_gate;

            public:
                routes _routes;
                using connection = nil::actor::httpd::connection;
                explicit http_server(const sstring &name) : _stats(*this, name) {
                    _date_format_timer.arm_periodic(1s);
                }
                /*!
                 * \brief set tls credentials for the server
                 * Setting the tls credentials will set the http-server to work in https mode.
                 *
                 * To use the https, create server credentials and pass it to the server before it starts.
                 *
                 * Use case example using actor threads for clarity:

                    distributed<http_server> server; // typical server

                    nil::actor::shared_ptr<nil::actor::tls::credentials_builder> creds =
                 nil::actor::make_shared<nil::actor::tls::credentials_builder>(); sstring ms_cert = "MyCertificate.crt";
                 sstring ms_key = "MyKey.key";

                    creds->set_dh_level(nil::actor::tls::dh_params::level::MEDIUM);

                    creds->set_x509_key_file(ms_cert, ms_key, nil::actor::tls::x509_crt_format::PEM).get();
                    creds->set_system_trust().get();


                    server.invoke_on_all([creds](http_server& server) {
                        server.set_tls_credentials(creds->build_server_credentials());
                        return make_ready_future<>();
                    }).get();
                 *
                 */
                void set_tls_credentials(shared_ptr<nil::actor::tls::server_credentials> credentials);

                size_t get_content_length_limit() const;

                void set_content_length_limit(size_t limit);

                future<> listen(socket_address addr, listen_options lo);
                future<> listen(socket_address addr);
                future<> stop();

                future<> do_accepts(int which);

                uint64_t total_connections() const;
                uint64_t current_connections() const;
                uint64_t requests_served() const;
                uint64_t read_errors() const;
                uint64_t reply_errors() const;
                // Write the current date in the specific "preferred format" defined in
                // RFC 7231, Section 7.1.1.1.
                static sstring http_date();

            private:
                future<> do_accept_one(int which);
                boost::intrusive::list<connection> _connections;
                friend class nil::actor::httpd::connection;
                friend class http_server_tester;
            };

            class http_server_tester {
            public:
                static std::vector<server_socket> &listeners(http_server &server) {
                    return server._listeners;
                }
            };

            /*
             * A helper class to start, set and listen an http server
             * typical use would be:
             *
             * auto server = new http_server_control();
             *                 server->start().then([server] {
             *                 server->set_routes(set_routes);
             *              }).then([server, port] {
             *                  server->listen(port);
             *              }).then([port] {
             *                  std::cout << "Actor HTTP server listening on port " << port << " ...\n";
             *              });
             */
            class http_server_control {
                std::unique_ptr<distributed<http_server>> _server_dist;

            private:
                static sstring generate_server_name();

            public:
                http_server_control() : _server_dist(new distributed<http_server>) {
                }

                future<> start(const sstring &name = generate_server_name());
                future<> stop();
                future<> set_routes(std::function<void(routes &r)> fun);
                future<> listen(socket_address addr);
                future<> listen(socket_address addr, listen_options lo);
                distributed<http_server> &server();
            };

        }    // namespace httpd

    }    // namespace actor
}    // namespace nil
