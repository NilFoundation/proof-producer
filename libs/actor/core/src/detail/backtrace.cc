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

#include <boost/predef.h>

#include <nil/actor/detail/backtrace.hh>

#if BOOST_LIB_STD_GNU && BOOST_OS_LINUX
#include <link.h>
#endif

#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include <nil/actor/core/print.hh>
#include <nil/actor/core/thread.hh>
#include <nil/actor/core/reactor.hh>

namespace nil {
    namespace actor {
#if BOOST_LIB_STD_GNU && BOOST_OS_LINUX
        static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t size, void *data) {
            std::size_t total_size {0};
            for (int i = 0; i < info->dlpi_phnum; i++) {
                const auto hdr = info->dlpi_phdr[i];

                // Only account loadable, executable (text) segments
                if (hdr.p_type == PT_LOAD && (hdr.p_flags & PF_X) == PF_X) {
                    total_size += hdr.p_memsz;
                }
            }

            reinterpret_cast<std::vector<shared_object> *>(data)->push_back(
                {info->dlpi_name, info->dlpi_addr, info->dlpi_addr + total_size});

            return 0;
        }
#endif

        static std::vector<shared_object> enumerate_shared_objects() {
            std::vector<shared_object> shared_objects;
#if BOOST_LIB_STD_GNU && BOOST_OS_LINUX
            dl_iterate_phdr(dl_iterate_phdr_callback, &shared_objects);
#endif

            return shared_objects;
        }

        static const std::vector<shared_object> shared_objects {enumerate_shared_objects()};
        static const shared_object uknown_shared_object {"", 0, std::numeric_limits<uintptr_t>::max()};

        bool operator==(const frame &a, const frame &b) {
            return a.so == b.so && a.addr == b.addr;
        }

        frame decorate(uintptr_t addr) {
            // If the shared-objects are not enumerated yet, or the enumeration
            // failed return the addr as-is with a dummy shared-object.
            if (shared_objects.empty()) {
                return {&uknown_shared_object, addr};
            }

            auto it = std::find_if(shared_objects.begin(), shared_objects.end(),
                                   [&](const shared_object &so) { return addr >= so.begin && addr < so.end; });

            // Unidentified addresses are assumed to originate from the executable.
            auto &so = it == shared_objects.end() ? shared_objects.front() : *it;
            return {&so, addr - so.begin};
        }

        simple_backtrace current_backtrace_tasklocal() noexcept {
            simple_backtrace::vector_type v;
            backtrace([&](frame f) {
                if (v.size() < v.capacity()) {
                    v.emplace_back(std::move(f));
                }
            });
            return simple_backtrace(std::move(v));
        }

        size_t simple_backtrace::calculate_hash() const {
            size_t h = 0;
            for (auto f : _frames) {
                h = ((h << 5) - h) ^ (f.so->begin + f.addr);
            }
            return h;
        }

        std::ostream &operator<<(std::ostream &out, const frame &f) {
            if (!f.so->name.empty()) {
                out << f.so->name << "+";
            }
            out << format("0x{:x}", f.addr);
            return out;
        }

        std::ostream &operator<<(std::ostream &out, const simple_backtrace &b) {
            for (auto f : b._frames) {
                out << "   " << f << "\n";
            }
            return out;
        }

        std::ostream &operator<<(std::ostream &out, const tasktrace &b) {
            out << b._main;
            for (auto &&e : b._prev) {
                out << "   --------\n";
                std::visit(make_visitor([&](const shared_backtrace &sb) { out << sb; },
                                        [&](const task_entry &f) { out << "   " << f << "\n"; }),
                           e);
            }
            return out;
        }

        std::ostream &operator<<(std::ostream &out, const task_entry &e) {
            return out << nil::actor::pretty_type_name(*e._task_type);
        }

        tasktrace current_tasktrace() noexcept {
            auto main = current_backtrace_tasklocal();

            tasktrace::vector_type prev;
            size_t hash = 0;
            if (local_engine && g_current_context) {
                task *tsk = nullptr;

                thread_context *thread = thread_impl::get();
                if (thread) {
                    tsk = thread->waiting_task();
                } else {
                    tsk = local_engine->current_task();
                }

                while (tsk && prev.size() < prev.max_size()) {
                    shared_backtrace bt = tsk->get_backtrace();
                    hash *= 31;
                    if (bt) {
                        hash ^= bt->hash();
                        prev.push_back(bt);
                    } else {
                        const std::type_info &ti = typeid(*tsk);
                        prev.push_back(task_entry(ti));
                        hash ^= ti.hash_code();
                    }
                    tsk = tsk->waiting_task();
                }
            }

            return tasktrace(std::move(main), std::move(prev), hash, current_scheduling_group());
        }

        saved_backtrace current_backtrace() noexcept {
            return current_tasktrace();
        }

        tasktrace::tasktrace(simple_backtrace main, tasktrace::vector_type prev, size_t prev_hash,
                             scheduling_group sg) :
            _main(std::move(main)),
            _prev(std::move(prev)), _sg(sg), _hash(_main.hash() * 31 ^ prev_hash) {
        }

        bool tasktrace::operator==(const tasktrace &o) const {
            return _hash == o._hash && _main == o._main && _prev == o._prev;
        }

        tasktrace::~tasktrace() {
        }

    }    // namespace actor
}    // namespace nil
