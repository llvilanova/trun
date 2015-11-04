/** trun-detail.hpp ---
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

#ifndef TRUN_DETAIL_HPP
#define TRUN_DETAIL_HPP 1

#include <chrono>
#include <cmath>
#include <cpuid.h>
#include <cstdio>
#include <err.h>
#include <limits>
#include <sys/time.h>
#include <unistd.h>
#include <utility>

#include <trun/detail/common.hpp>
#include <trun/detail/core-impl.hpp>
#include <trun/detail/parameters.hpp>
#include <trun/detail/time.hpp>


//////////////////////////////////////////////////////////////////////
// * Experiment run

namespace trun {

    template<class C, class F, class... A>
    result<C> && run(const parameters<C> & params, F&& func, A&&... args)
    {
        result<C> res;
        if (params.clock_time.count() == 0) {
            auto new_params = time::calibrate(params);
            INFO("Executing benchmark...");
            trun::detail::core::run<false>(res, new_params,
                                           std::forward<F>(func), std::forward<A>(args)...);
        } else {
            detail::parameters::check(params);
            INFO("Executing benchmark...");
            trun::detail::core::run<false>(res, params,
                                           std::forward<F>(func), std::forward<A>(args)...);
        }
        res.mean = time::detail::clock_units<C>(res.mean);
        res.sigma = time::detail::clock_units<C>(res.sigma);
        return std::move(res);
    }

}

#endif // TRUN_DETAIL_HPP
