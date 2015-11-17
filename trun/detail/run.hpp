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

    template<class C, bool show_info, bool show_debug, class F>
    static inline
    result<C> run(const parameters<C> & params, F&& func,
                  std::function<void(size_t, size_t, size_t)> && func_iter_start,
                  std::function<void(size_t, size_t, size_t)> && func_batch_start,
                  std::function<void(size_t, size_t, size_t)> && func_batch_stop,
                  std::function<void(size_t, size_t, size_t)> && func_iter_stop,
                  std::function<void(size_t, size_t, size_t)> && func_batch_select,
                  std::function<void(size_t, size_t, size_t)> && func_iter_select)
    {
        result<C> res;
        parameters<C> run_params = params;
        if (params.clock_time.count() == 0) {
            run_params = time::calibrate<C, show_info, show_debug>(params);
        }

        detail::parameters::check(run_params);
        detail::info<show_info>("Executing benchmark...");
        trun::detail::core::run<false, show_info, show_debug>(
            res, run_params, std::forward<F>(func),
            std::forward<decltype(func_iter_start)>(func_iter_start),
            std::forward<decltype(func_batch_start)>(func_batch_start),
            std::forward<decltype(func_batch_stop)>(func_batch_stop),
            std::forward<decltype(func_iter_stop)>(func_iter_stop),
            std::forward<decltype(func_batch_select)>(func_batch_select),
            std::forward<decltype(func_iter_select)>(func_iter_select));

        res.mean = time::detail::clock_units<C>(res.mean);
        res.sigma = time::detail::clock_units<C>(res.sigma);
        return res;
    }

    template<class C, bool show_info, bool show_debug, class F>
    static inline
    result<C> run(F&& func,
                  std::function<void(size_t, size_t, size_t)> && func_iter_start,
                  std::function<void(size_t, size_t, size_t)> && func_batch_start,
                  std::function<void(size_t, size_t, size_t)> && func_batch_stop,
                  std::function<void(size_t, size_t, size_t)> && func_iter_stop,
                  std::function<void(size_t, size_t, size_t)> && func_batch_select,
                  std::function<void(size_t, size_t, size_t)> && func_iter_select)
    {
        auto params = time::calibrate<C, show_info, show_debug>();
        return run<C, show_info, show_debug>(
            std::forward<parameters<C>>(params),
            std::forward<F>(func),
            std::forward<decltype(func_iter_start)>(func_iter_start),
            std::forward<decltype(func_batch_start)>(func_batch_start),
            std::forward<decltype(func_batch_stop)>(func_batch_stop),
            std::forward<decltype(func_iter_stop)>(func_iter_stop),
            std::forward<decltype(func_batch_select)>(func_batch_select),
            std::forward<decltype(func_iter_select)>(func_iter_select));
    }

}

#endif // TRUN__DETAIL__RUN_HPP
