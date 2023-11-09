//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
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

#ifndef SOLANA_NETWORK_STACK_REGISTRY_HH
#define SOLANA_NETWORK_STACK_REGISTRY_HH

#include <boost/program_options.hpp>
#include <nil/actor/network/api.hh>

namespace nil {
    namespace actor {
        class network_stack_registry {
        public:
            using options = boost::program_options::variables_map;

        private:
            static std::unordered_map<sstring, noncopyable_function<future<std::unique_ptr<network_stack>>(options opts)>> &
                _map() {
                static std::unordered_map<sstring, noncopyable_function<future<std::unique_ptr<network_stack>>(options opts)>>
                    map;
                return map;
            }
            static sstring &_default() {
                static sstring def;
                return def;
            }

        public:
            static boost::program_options::options_description &options_description() {
                static boost::program_options::options_description opts;
                return opts;
            }
            static void register_stack(const sstring &name, const boost::program_options::options_description &opts,
                                       noncopyable_function<future<std::unique_ptr<network_stack>>(options opts)> create,
                                       bool make_default);
            static sstring default_stack();
            static std::vector<sstring> list();
            static future<std::unique_ptr<network_stack>> create(options opts);
            static future<std::unique_ptr<network_stack>> create(const sstring &name, options opts);
        };
    }
}

#endif    // SOLANA_NETWORK_STACK_REGISTRY_HH
