//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2022 Aleksei Moskvin <alalmoskvin@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#ifndef SOLANA_NETWORK_STACK_REGISTRY_CC
#define SOLANA_NETWORK_STACK_REGISTRY_CC

#include <boost/program_options.hpp>
#include <nil/actor/network/api.hh>
#include <nil/actor/core/network_stack_registry.hh>

namespace nil {
    namespace actor {

        void network_stack_registry::register_stack(
            const sstring &name, const boost::program_options::options_description &opts,
            noncopyable_function<future<std::unique_ptr<network_stack>>(options opts)> create, bool make_default) {
            if (_map().count(name)) {
                return;
            }
            _map()[name] = std::move(create);
            options_description().add(opts);
            if (make_default) {
                _default() = name;
            }
        }

        sstring network_stack_registry::default_stack() {
            return _default();
        }

        std::vector<sstring> network_stack_registry::list() {
            std::vector<sstring> ret;
            for (auto &&ns : _map()) {
                ret.push_back(ns.first);
            }
            return ret;
        }

        future<std::unique_ptr<network_stack>> network_stack_registry::create(options opts) {
            return create(_default(), std::move(opts));
        }

        future<std::unique_ptr<network_stack>> network_stack_registry::create(const sstring &name, options opts) {
            if (!_map().count(name)) {
                throw std::runtime_error(format("network stack {} not registered", name));
            }
            return _map()[name](std::move(opts));
        }
    }
}

#endif    // SOLANA_NETWORK_STACK_REGISTRY_CC
