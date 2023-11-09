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

namespace nil {
    namespace actor {

        struct pollfn {
            virtual ~pollfn() {
            }
            // Returns true if work was done (false = idle)
            virtual bool poll() = 0;
            // Checks if work needs to be done, but without actually doing any
            // returns true if works needs to be done (false = idle)
            virtual bool pure_poll() = 0;
            // Tries to enter interrupt mode.
            //
            // If it returns true, then events from this poller will wake
            // a sleeping idle loop, and exit_interrupt_mode() must be called
            // to return to normal polling.
            //
            // If it returns false, the sleeping idle loop may not be entered.
            virtual bool try_enter_interrupt_mode() = 0;
            virtual void exit_interrupt_mode() = 0;
        };

        // The common case for poller -- do not make any difference between
        // poll() and pure_poll(), always/never agree to go to sleep and do
        // nothing on wakeup.
        template<bool Passive>
        struct simple_pollfn : public pollfn {
            virtual bool pure_poll() override final {
                return poll();
            }
            virtual bool try_enter_interrupt_mode() override final {
                return Passive;
            }
            virtual void exit_interrupt_mode() override final {
            }
        };

    }    // namespace actor
}    // namespace nil
