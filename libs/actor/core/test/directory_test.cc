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
#include <nil/actor/core/app_template.hh>
#include <nil/actor/core/print.hh>
#include <nil/actor/core/shared_ptr.hh>

using namespace nil::actor;

const char *de_type_desc(directory_entry_type t) {
    switch (t) {
        case directory_entry_type::unknown:
            return "unknown";
        case directory_entry_type::block_device:
            return "block_device";
        case directory_entry_type::char_device:
            return "char_device";
        case directory_entry_type::directory:
            return "directory";
        case directory_entry_type::fifo:
            return "fifo";
        case directory_entry_type::link:
            return "link";
        case directory_entry_type::regular:
            return "regular";
        case directory_entry_type::socket:
            return "socket";
    }
    assert(0 && "should not get here");
    return nullptr;
}

int main(int ac, char **av) {
    class lister {
        file _f;
        subscription<directory_entry> _listing;

    public:
        lister(file f) :
            _f(std::move(f)), _listing(_f.list_directory([this](directory_entry de) { return report(de); })) {
        }
        future<> done() {
            return _listing.done();
        }

    private:
        future<> report(directory_entry de) {
            return file_stat(de.name, follow_symlink::no).then([de = std::move(de)](stat_data sd) {
                if (de.type) {
                    assert(*de.type == sd.type);
                } else {
                    assert(sd.type == directory_entry_type::unknown);
                }
                fmt::print("{} (type={})\n", de.name, de_type_desc(sd.type));
                return make_ready_future<>();
            });
        }
    };
    return app_template().run(ac, av, [] {
        return engine().open_directory(".").then([](file f) {
            return do_with(lister(std::move(f)), [](lister &l) { return l.done().then([] { return 0; }); });
        });
    });
}
