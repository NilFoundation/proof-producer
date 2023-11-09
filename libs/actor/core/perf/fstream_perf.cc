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

#include <nil/actor/core/fstream.hh>
#include <nil/actor/core/core.hh>
#include <nil/actor/core/file.hh>
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/loop.hh>
#include <fmt/printf.h>

using namespace nil::actor;
using namespace std::chrono_literals;

int main(int ac, char **av) {
    app_template at;
    namespace bpo = boost::program_options;
    at.add_options()("concurrency", bpo::value<unsigned>()->default_value(1), "Write operations to issue in parallel")(
        "buffer-size", bpo::value<size_t>()->default_value(4096),
        "Write buffer size")("total-ops", bpo::value<unsigned>()->default_value(100000),
                             "Total write operations to issue")("sloppy-size", bpo::value<bool>()->default_value(false),
                                                                "Enable the sloppy-size optimization");
    return at.run(ac, av, [&at] {
        auto concurrency = at.configuration()["concurrency"].as<unsigned>();
        auto buffer_size = at.configuration()["buffer-size"].as<size_t>();
        auto total_ops = at.configuration()["total-ops"].as<unsigned>();
        auto sloppy_size = at.configuration()["sloppy-size"].as<bool>();
        file_open_options foo;
        foo.sloppy_size = sloppy_size;
        return open_file_dma("testfile.tmp", open_flags::wo | open_flags::create | open_flags::exclusive, foo)
            .then([=](file f) {
                file_output_stream_options foso;
                foso.buffer_size = buffer_size;
                foso.preallocation_size = 32 << 20;
                foso.write_behind = concurrency;
                return make_file_output_stream(f, foso).then([=](output_stream<char> &&os) {
                    return do_with(
                        std::move(os), std::move(f), unsigned(0),
                        [=](output_stream<char> &os, file &f, unsigned &completed) {
                            auto start = std::chrono::steady_clock::now();
                            return repeat([=, &os, &completed] {
                                       if (completed == total_ops) {
                                           return make_ready_future<stop_iteration>(stop_iteration::yes);
                                       }
                                       char buf[buffer_size];
                                       memset(buf, 0, buffer_size);
                                       return os.write(buf, buffer_size).then([&completed] {
                                           ++completed;
                                           return stop_iteration::no;
                                       });
                                   })
                                .then([=, &os] {
                                    auto end = std::chrono::steady_clock::now();
                                    using fseconds = std::chrono::duration<float, std::ratio<1, 1>>;
                                    auto iops = total_ops / std::chrono::duration_cast<fseconds>(end - start).count();
                                    fmt::print("{:10} {:10} {:10} {:12}\n", "bufsize", "ops", "iodepth", "IOPS");
                                    fmt::print("{:10d} {:10d} {:10d} {:12.0f}\n", buffer_size, total_ops, concurrency,
                                               iops);
                                    return os.flush();
                                })
                                .then([&os] { return os.close(); });
                        });
                });
            });
    });
}
