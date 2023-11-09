//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
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

#include <nil/actor/core/sharded.hh>
#include <nil/actor/core/loop.hh>

#include <boost/iterator/counting_iterator.hpp>

namespace nil {
    namespace actor {
        namespace detail {

            future<> sharded_parallel_for_each(unsigned nr_shards, on_each_shard_func on_each_shard) noexcept(
                std::is_nothrow_move_constructible<on_each_shard_func>::value) {
                return parallel_for_each(boost::irange<unsigned>(0, nr_shards), std::move(on_each_shard));
            }
        }    // namespace detail
    }        // namespace actor
}    // namespace nil
