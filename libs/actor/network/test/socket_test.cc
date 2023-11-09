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

#include <nil/actor/core/reactor.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/memory.hh>
#include <nil/actor/detail/std-compat.hh>

#include <nil/actor/network/posix-stack.hh>

using namespace nil::actor;

future<> handle_connection(connected_socket s) {
    auto in = s.input();
    auto out = s.output();
    return do_with(std::move(in), std::move(out), [](auto &in, auto &out) {
        return do_until([&in]() { return in.eof(); },
                        [&in, &out] {
                            return in.read().then([&out](auto buf) {
                                return out.write(std::move(buf)).then([&out]() { return out.close(); });
                            });
                        });
    });
}

future<> echo_server_loop() {
    return do_with(server_socket(listen(make_ipv4_address({1234}), listen_options {.reuse_address = true})),
                   [](auto &listener) {
                       // Connect asynchronously in background.
                       (void)connect(make_ipv4_address({"127.0.0.1", 1234})).then([](connected_socket &&socket) {
                           socket.shutdown_output();
                       });
                       return listener.accept()
                           .then([](accept_result ar) {
                               connected_socket s = std::move(ar.connection);
                               return handle_connection(std::move(s));
                           })
                           .then([l = std::move(listener)]() mutable { return l.abort_accept(); });
                   });
}

class my_malloc_allocator : public boost::container::pmr::memory_resource {
public:
    int allocs;
    int frees;
    void *do_allocate(std::size_t bytes, std::size_t alignment) override {
        allocs++;
        return malloc(bytes);
    }
    void do_deallocate(void *ptr, std::size_t bytes, std::size_t alignment) override {
        frees++;
        return free(ptr);
    }
    virtual bool do_is_equal(const boost::container::pmr::memory_resource &__other) const noexcept override {
        abort();
    }
};

my_malloc_allocator malloc_allocator;
boost::container::pmr::polymorphic_allocator<char> allocator {&malloc_allocator};

int main(int ac, char **av) {
    register_network_stack(
        "posix", boost::program_options::options_description(),
        [](boost::program_options::variables_map ops) {
            return smp::main_thread() ? net::posix_network_stack::create(ops, &allocator) :
                                        net::posix_ap_network_stack::create(ops);
        },
        true);
    return app_template().run_deprecated(ac, av, [] {
        return echo_server_loop().finally(
            []() { engine().exit((malloc_allocator.allocs == malloc_allocator.frees) ? 0 : 1); });
    });
}
