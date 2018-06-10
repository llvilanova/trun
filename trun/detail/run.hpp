/** trun/detail/run.hpp ---
 *
 * Copyright (C) 2015-2018 Lluís Vilanova
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

    template<trun::message msg, class C, class F, class... Args>
    static inline
    result<C>
    run(const parameters<C> & params, F&& func, Args&&... args)
    {
        if (std::is_same<C, ::trun::time::tsc_clock>::value) {
          // Use raw TSC cycles, and only convert to time at the end
          parameters<::trun::time::tsc_cycles> params_2 =
            params.template convert<::trun::time::tsc_cycles>();
          auto res = trun::detail::core::run<false, msg>(
              params_2, std::forward<F>(func), std::forward<Args>(args)...);
          return res.template convert<C>();

        } else {
          parameters<C> run_params = params;
          if (params.clock_time.count() == 0) {
              run_params = time::calibrate<C, msg>();
          }

          ::trun::time::detail::check(C());
          detail::parameters::check(run_params);
          detail::message<message::INFO, msg>("Executing benchmark...");
          auto res = trun::detail::core::run<false, msg, C, F, Args...>(
              run_params, std::forward<F>(func), std::forward<Args>(args)...);

          return res;
        }
    }

    template<class C, trun::message msg, class F, class... Args>
    static inline
    result<C>
    run(F&& func, Args&&... args)
    {
        auto params = time::calibrate<C, msg>();
        return run<false, msg, C, F>(
            std::forward<parameters<C>>(params),
            std::forward<F>(func),
            std::forward<Args>(args)...);
    }

}

#endif // TRUN__DETAIL__RUN_HPP
