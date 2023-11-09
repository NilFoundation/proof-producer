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

// For IDEs that don't see ACTOR_API_LEVEL, generate a nice default
#ifndef ACTOR_API_LEVEL
#define ACTOR_API_LEVEL 3
#endif

#if ACTOR_API_LEVEL == 6
#define ACTOR_INCLUDE_API_V6 inline
#else
#define ACTOR_INCLUDE_API_V6
#endif

#if ACTOR_API_LEVEL == 5
#define ACTOR_INCLUDE_API_V5 inline
#else
#define ACTOR_INCLUDE_API_V5
#endif

#if ACTOR_API_LEVEL == 4
#define ACTOR_INCLUDE_API_V4 inline
#else
#define ACTOR_INCLUDE_API_V4
#endif

#if ACTOR_API_LEVEL == 3
#define ACTOR_INCLUDE_API_V3 inline
#else
#define ACTOR_INCLUDE_API_V3
#endif

// Declare them here so we don't have to use the macros everywhere
namespace nil {
    namespace actor {
        ACTOR_INCLUDE_API_V3 namespace api_v3 {
            inline namespace and_newer { }
        }
        ACTOR_INCLUDE_API_V4 namespace api_v4 {
            inline namespace and_newer {
                using namespace api_v3::and_newer;
            }
        }
        ACTOR_INCLUDE_API_V5 namespace api_v5 {
            inline namespace and_newer {
                using namespace api_v4::and_newer;
            }
        }
        ACTOR_INCLUDE_API_V6 namespace api_v6 {
            inline namespace and_newer {
                using namespace api_v5::and_newer;
            }
        }
    }    // namespace actor
}    // namespace nil
