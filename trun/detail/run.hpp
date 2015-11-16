/** trun/detail/run.hpp ---
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

#ifndef TRUN__DETAIL__RUN_HPP
#define TRUN__DETAIL__RUN_HPP 1

#include <trun/detail/common.hpp>
#include <trun/detail/core.hpp>
#include <trun/detail/parameters.hpp>
#include <trun/detail/time.hpp>


namespace trun {

    template<class C, class F>
    result<C> run(const parameters<C> & params, F&& func)
    {
        result<C> res;
        if (params.clock_time.count() == 0) {
            auto new_params = time::calibrate(params);
            INFO("Executing benchmark...");
            trun::detail::core::run<false>(res, new_params, std::forward<F>(func));
        } else {
            detail::parameters::check(params);
            INFO("Executing benchmark...");
            trun::detail::core::run<false>(res, params, std::forward<F>(func));
        }
        res.mean = time::detail::clock_units<C>(res.mean);
        res.sigma = time::detail::clock_units<C>(res.sigma);
        return res;
    }

    template<class C, class F>
    result<C> run(F&& func)
    {
        auto params = time::calibrate<C>();
        return run(std::forward<parameters<C>>(params), std::forward<F>(func));
    }

}

#endif // TRUN__DETAIL__RUN_HPP
