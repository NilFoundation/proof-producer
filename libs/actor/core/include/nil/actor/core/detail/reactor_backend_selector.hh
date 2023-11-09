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

#include <boost/predef.h>

#include <nil/actor/core/detail/reactor_backend_epoll.hh>
#include <nil/actor/core/detail/reactor_backend_osv.hh>

#if BOOST_OS_LINUX
#include <nil/actor/core/detail/reactor_backend_aio.hh>
#endif

namespace nil {
    namespace actor {
        class reactor_backend_selector {
            std::string _name;

        private:
            static bool has_enough_aio_nr();
            explicit reactor_backend_selector(std::string name) : _name(std::move(name)) {
            }

        public:
            std::unique_ptr<reactor_backend> create(reactor &r);
            static reactor_backend_selector default_backend();
            static std::vector<reactor_backend_selector> available();
            friend std::ostream &operator<<(std::ostream &os, const reactor_backend_selector &rbs) {
                return os << rbs._name;
            }
            friend void validate(boost::any &v, const std::vector<std::string> &values, reactor_backend_selector *rbs,
                                 int) {
                boost::program_options::validators::check_first_occurrence(v);
                const auto& s = boost::program_options::validators::get_single_string(values);
                for (auto &&x : available()) {
                    if (s == x._name) {
                        v = std::move(x);
                        return;
                    }
                }
                throw boost::program_options::validation_error(
                    boost::program_options::validation_error::invalid_option_value);
            }
        };
    }    // namespace actor
}    // namespace nil
