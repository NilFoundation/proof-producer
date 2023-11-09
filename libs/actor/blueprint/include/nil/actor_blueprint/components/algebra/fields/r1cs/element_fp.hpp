//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020-2021 Nikita Kaskov <nbering@nil.foundation>
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
// @file Declaration of interfaces for Fp2 components.
//
// The components verify field arithmetic in Fp2 = Fp[U]/(U^2-non_residue),
// where non_residue is in Fp.
//---------------------------------------------------------------------------//

#ifndef ACTOR_BLUEPRINT_COMPONENTS_FP_COMPONENTS_HPP
#define ACTOR_BLUEPRINT_COMPONENTS_FP_COMPONENTS_HPP

#include <nil/actor/zk/blueprint/r1cs.hpp>

namespace nil {
    namespace actor {
        namespace actor_blueprint {
            namespace components {

                /******************************** element_fp ************************************/

                /**
                 * Component that represents an element_fp.
                 */
                template<typename FieldType>
                using element_fp = detail::blueprint_linear_combination<FieldType>;
            }    // namespace components
        }        // namespace blueprint
    }            // namespace actor
}    // namespace nil

#endif    // ACTOR_BLUEPRINT_COMPONENTS_FP_COMPONENTS_HPP
