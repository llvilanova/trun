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

            template<class C>
            void check()
            {
            }

            template<class C>
            typename result<C>::duration clock_units(const typename result<C>::duration & time)
            {
                return time;
            }

        }

        template<class C>
        parameters<C>
        calibrate(const parameters<C> & params)
        {
            detail::check<C>();

            // initialize parameters
            parameters<C> res_params = params;
            res_params.clock_time = typename C::duration(typename C::rep(0));
            if (res_params.sigma_outlier_perc == 0.0) {
                res_params.sigma_outlier_perc = 3.0;
            }
            trun::detail::parameters::check(res_params);

            // clock measurements are going to be fast
            parameters<C> clock_params(res_params);
            clock_params.warmup_batch_size = 1000;
            clock_params.batch_size = 10000;
            clock_params.max_experiments = 1000000000;

            INFO("Calibrating clock overheads...");
            result<C> res;
            trun::detail::core::run<true>(res, clock_params, C::now);
            if (!res.converged) {
                errx(1, "clock calibration did not converge");
            }

            res_params.clock_time = detail::clock_units<C>(res.mean);

            return std::move(res_params);
        }

    }
}

#endif // TRUN__DETAIL__TIME_HPP
