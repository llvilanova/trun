/** trun/detail/time.hpp ---
 *
 * Copyright (C) 2015 Lluís Vilanova
 *
 * Author: Lluís Vilanova <vilanova@ac.upc.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef TRUN__DETAIL__TIME_HPP
#define TRUN__DETAIL__TIME_HPP 1

#include <trun/detail/core.hpp>


namespace trun {
    namespace time {

        namespace detail {

            template<class Clock>
            static inline
            void check(const Clock &clock)
            {
            }

        }

        template<class C, trun::message msg>
        static inline
        parameters<C> calibrate(const parameters<C> & params)
        {
            // initialize parameters
            parameters<C> res_params = params;
            res_params.clock_time = typename C::duration(typename C::rep(0));
            trun::detail::parameters::check(res_params);

            trun::detail::message<trun::message::INFO, msg>("Calibrating clock overheads...");
            auto time = []() {
                auto t1 = C::now();
                (void)t1;
                auto t2 = C::now();
                (void)t2;
            };
            result<C> res;
            trun::detail::core::run<true, msg, false>(res, params, time);
            if (!res.converged) {
                errx(1, "[trun] clock calibration did not converge");
            }

            res_params.clock_time = res.mean;

            return std::move(res_params);
        }

        template<class C, trun::message msg>
        static inline
        parameters<C> calibrate()
        {
            parameters<C> res_params;

            auto clock_params = res_params;
            clock_params.confidence_sigma = 3; // 99.73%
            clock_params.warmup_batch_size = 1000;
            clock_params.batch_size = 10000;
            clock_params.experiment_timeout = 60;
            clock_params = calibrate<C, msg>(clock_params);

            res_params.clock_time = clock_params.clock_time;
            return std::move(res_params);
        }

        template<class Ratio, class Clock>
        static inline
        std::string units(const Clock &clock)
        {
            std::string units = "?";
            if (std::ratio_equal<std::femto, Ratio>::value) {
                units = "fs";
            } else if (std::ratio_equal<std::pico, Ratio>::value) {
                units = "ps";
            } else if (std::ratio_equal<std::nano, Ratio>::value) {
                units = "ns";
            } else if (std::ratio_equal<std::micro, Ratio>::value) {
                units = "us";
            } else if (std::ratio_equal<std::milli, Ratio>::value) {
                units = "ms";
            } else if (std::ratio_equal<std::ratio<1>, Ratio>::value) {
                units = "sec";
            } else if (std::ratio_equal<std::ratio<60>, Ratio>::value) {
                units = "min";
            } else if (std::ratio_equal<std::ratio<60*60>, Ratio>::value) {
                units = "hours";
            } else if (std::ratio_equal<std::ratio<60*60*24>, Ratio>::value) {
                units = "days";
            }
            return units;
        }

    }
}

#include <trun/detail/time-cycles.hpp>

#endif // TRUN__DETAIL__TIME_HPP
