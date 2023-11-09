//---------------------------------------------------------------------------//
// Copyright (c) 2022 Ilia Shirobokov <i.shirobokov@nil.foundation>
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

#ifndef ACTOR_BLUEPRINT_MC_PLONK_KIMCHI_DETAIL_CONSTRAINTS_INDEX_TERM_TYPE_HPP
#define ACTOR_BLUEPRINT_MC_PLONK_KIMCHI_DETAIL_CONSTRAINTS_INDEX_TERM_TYPE_HPP

#include <nil/actor_blueprint_mc/blueprint/plonk.hpp>
#include <nil/actor_blueprint_mc/component.hpp>
#include <nil/actor_blueprint_mc/components/systems/snark/plonk/kimchi/types/column_type.hpp>

namespace nil {
    namespace actor_blueprint_mc {
            namespace components {
                struct index_term_type {
                    const column_type type;
                    const std::size_t index;
                    const char *str_repr;
                    const std::size_t rows_amount;
                };
            }    // namespace components
    }            // namespace actor_blueprint_mc
}    // namespace nil

#endif    // ACTOR_BLUEPRINT_MC_PLONK_KIMCHI_DETAIL_CONSTRAINTS_INDEX_TERM_TYPE_HPP