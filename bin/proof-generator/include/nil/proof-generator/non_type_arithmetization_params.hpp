//---------------------------------------------------------------------------//
// Copyright (c) 2024 Iosif (x-mass) <x-mass@nil.foundation>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//---------------------------------------------------------------------------//

#ifndef PROOF_GENERATOR_NON_TYPE_ARITHMETIZATION_PARAMS_HPP
#define PROOF_GENERATOR_NON_TYPE_ARITHMETIZATION_PARAMS_HPP

#include <cstddef>

namespace nil {
    namespace proof_generator {

        struct ColumnsParams {
            std::size_t witness_columns;
            std::size_t public_input_columns;
            std::size_t component_constant_columns;
            std::size_t component_selector_columns;
            std::size_t lookup_constant_columns;
            std::size_t lookup_selector_columns;

            bool operator==(const ColumnsParams& other) const {
                return witness_columns == other.witness_columns && public_input_columns == other.public_input_columns
                    && component_constant_columns == other.component_constant_columns
                    && component_selector_columns == other.component_selector_columns
                    && lookup_constant_columns == other.lookup_constant_columns
                    && lookup_selector_columns == other.lookup_selector_columns;
            }
        };

        // Need this class to be derived into actual params, so we could overload
        // read/write operators for parsing.
        class SizeTParam {
        public:
            SizeTParam() = default; // lexical_cast wants it
            constexpr SizeTParam(std::size_t value)
                : value_(value) {
            }

            constexpr operator std::size_t() const {
                return value_;
            }

            bool operator==(const SizeTParam& other) const {
                return value_ == other.value_;
            }

        private:
            std::size_t value_;
        };

        class LambdaParam : public SizeTParam {
        public:
            using SizeTParam::SizeTParam;
        };
        class GrindParam : public SizeTParam {
        public:
            using SizeTParam::SizeTParam;
        };

    } // namespace proof_generator
} // namespace nil

#endif // PROOF_GENERATOR_NON_TYPE_ARITHMETIZATION_PARAMS_HPP
