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

#include <memory>
#include <nil/actor/core/task.hh>
#include <nil/actor/core/future.hh>

namespace nil {
    namespace actor {

        template<typename Func>
        class lambda_task final : public task {
            Func _func;
            using futurator = futurize<std::result_of_t<Func()>>;
            typename futurator::promise_type _result;

        public:
            lambda_task(scheduling_group sg, const Func &func) : task(sg), _func(func) {
            }
            lambda_task(scheduling_group sg, Func &&func) : task(sg), _func(std::move(func)) {
            }
            typename futurator::type get_future() noexcept {
                return _result.get_future();
            }
            virtual void run_and_dispose() noexcept override {
                futurator::invoke(_func).forward_to(std::move(_result));
                delete this;
            }
            virtual task *waiting_task() noexcept override {
                return _result.waiting_task();
            }
        };

        template<typename Func>
        inline lambda_task<Func> *make_task(Func &&func) noexcept {
            return new lambda_task<Func>(current_scheduling_group(), std::forward<Func>(func));
        }

        template<typename Func>
        inline lambda_task<Func> *make_task(scheduling_group sg, Func &&func) noexcept {
            return new lambda_task<Func>(sg, std::forward<Func>(func));
        }

    }    // namespace actor
}    // namespace nil
