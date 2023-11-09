// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
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

#include <future>
#include <numeric>
#include <iostream>
#include <nil/actor/core/alien.hh>
#include <nil/actor/core/smp.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/posix.hh>
#include <nil/actor/core/reactor.hh>
#include <nil/actor/detail/later.hh>

using namespace nil::actor;

enum {
    ENGINE_READY = 24,
    ALIEN_DONE = 42,
};

int main(int argc, char **argv) {
    // we need a protocol that both actor and alien understand.
    // and on which, a actor future can wait.
    int engine_ready_fd = eventfd(0, 0);
    auto alien_done = file_desc::eventfd(0, 0);

    // use the raw fd, because actor engine want to take over the fds, if it
    // polls on them.
    auto zim = std::async([engine_ready_fd, alien_done = alien_done.get()] {
        eventfd_t result = 0;
        // wait until the actor engine is ready
        int r = ::eventfd_read(engine_ready_fd, &result);
        if (r < 0) {
            return -EINVAL;
        }
        if (result != ENGINE_READY) {
            return -EINVAL;
        }
        std::vector<std::future<int>> counts;
        for (auto i : boost::irange(0u, smp::count)) {
            // send messages from alien.
            counts.push_back(alien::submit_to(i, [i] { return nil::actor::make_ready_future<int>(i); }));
        }
        // std::future<void>
        alien::submit_to(0, [] { return nil::actor::make_ready_future<>(); }).wait();
        int total = 0;
        for (auto &count : counts) {
            total += count.get();
        }
        // i am done. dismiss the engine
        ::eventfd_write(alien_done, ALIEN_DONE);
        return total;
    });

    nil::actor::app_template app;
    eventfd_t result = 0;
    app.run(argc, argv, [&] {
        return nil::actor::now()
            .then([engine_ready_fd] {
                // engine ready!
                ::eventfd_write(engine_ready_fd, ENGINE_READY);
                return nil::actor::now();
            })
            .then([alien_done = std::move(alien_done), &result]() mutable {
                return do_with(nil::actor::pollable_fd(std::move(alien_done)), [&result](pollable_fd &alien_done_fds) {
                    // check if alien has dismissed me.
                    return alien_done_fds.readable().then([&result, &alien_done_fds] {
                        auto ret = alien_done_fds.get_file_desc().read(&result, sizeof(result));
                        return make_ready_future<size_t>(*ret);
                    });
                });
            })
            .then([&result](size_t n) {
                if (n != sizeof(result)) {
                    throw std::runtime_error("read from eventfd failed");
                }
                if (result != ALIEN_DONE) {
                    throw std::logic_error("alien failed to dismiss me");
                }
                return nil::actor::now();
            })
            .handle_exception([](auto ep) { std::cerr << "Error: " << ep << std::endl; })
            .finally([] { nil::actor::engine().exit(0); });
    });
    int total = zim.get();
    const auto shards = boost::irange(0u, smp::count);
    auto expected = std::accumulate(std::begin(shards), std::end(shards), 0);
    if (total != expected) {
        std::cerr << "Bad total: " << total << " != " << expected << std::endl;
        return 1;
    }
}
