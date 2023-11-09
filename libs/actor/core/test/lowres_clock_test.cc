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

#include <nil/actor/testing/test_case.hh>

#include <nil/actor/core/do_with.hh>
#include <nil/actor/core/lowres_clock.hh>
#include <nil/actor/core/sleep.hh>
#include <nil/actor/core/loop.hh>

#include <ctime>

#include <algorithm>
#include <array>
#include <chrono>

using namespace nil::actor;

//
// Sanity check the accuracy of the steady low-resolution clock.
//
ACTOR_TEST_CASE(steady_clock_sanity) {
    return do_with(lowres_clock::now(), [](auto &&t1) {
        static constexpr auto sleep_duration = std::chrono::milliseconds(100);

        return ::nil::actor::sleep(sleep_duration).then([&t1] {
            auto const elapsed = lowres_clock::now() - t1;
            auto const minimum_elapsed = 0.9 * sleep_duration;

            BOOST_REQUIRE(elapsed >= minimum_elapsed);

            return make_ready_future<>();
        });
    });
}

//
// At the very least, we can verify that the low-resolution system clock is within a second of the
// high-resolution system clock.
//
ACTOR_TEST_CASE(system_clock_sanity) {
    static const auto check_matching = [] {
        auto const system_time = std::chrono::system_clock::now();
        auto const lowres_time = lowres_system_clock::now();

        auto const t1 = std::chrono::system_clock::to_time_t(system_time);
        auto const t2 = lowres_system_clock::to_time_t(lowres_time);

        std::tm *lt1 = std::localtime(&t1);
        std::tm *lt2 = std::localtime(&t2);

        return (lt1->tm_isdst == lt2->tm_isdst) && (lt1->tm_year == lt2->tm_year) && (lt1->tm_mon == lt2->tm_mon) &&
               (lt1->tm_yday == lt2->tm_yday) && (lt1->tm_mday == lt2->tm_mday) && (lt1->tm_wday == lt2->tm_wday) &&
               (lt1->tm_hour == lt2->tm_hour) && (lt1->tm_min == lt2->tm_min) && (lt1->tm_sec == lt2->tm_sec);
    };

    //
    // Check two out of three samples in order to account for the possibility that the high-resolution clock backing
    // the low-resoltuion clock was captured in the range of the 990th to 999th millisecond of the second. This would
    // make the low-resolution clock and the high-resolution clock disagree on the current second.
    //

    return do_with(0ul, 0ul, [](std::size_t &index, std::size_t &success_count) {
        return repeat([&index, &success_count] {
            if (index >= 3) {
                BOOST_REQUIRE_GE(success_count, 2u);
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }

            return ::nil::actor::sleep(std::chrono::milliseconds(10)).then([&index, &success_count] {
                if (check_matching()) {
                    ++success_count;
                }

                ++index;
                return stop_iteration::no;
            });
        });
    });
}

//
// Verify that the low-resolution clock updates its reported time point over time.
//
ACTOR_TEST_CASE(system_clock_dynamic) {
    return do_with(lowres_system_clock::now(), [](auto &&t1) {
        return nil::actor::sleep(std::chrono::milliseconds(100)).then([&t1] {
            auto const t2 = lowres_system_clock::now();
            BOOST_REQUIRE_NE(t1.time_since_epoch().count(), t2.time_since_epoch().count());

            return make_ready_future<>();
        });
    });
}
