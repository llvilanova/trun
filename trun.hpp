/** trun.hpp ---
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


#ifndef TRUN_HPP
#define TRUN_HPP 1

#include <chrono>
#include <cstddef>
#include <utility>
#include <vector>


#ifndef TRUN_DEBUG
#define TRUN_DEBUG 0
#endif

#ifndef TRUN_INFO
#define TRUN_INFO 0
#endif


namespace trun {

    // Experiment run results.
    template<class Clock>
    class result
    {
    public:
        using duration = std::chrono::duration<double, typename Clock::period>;

        duration min, min_nooutliers;
        duration max, max_nooutliers;
        duration mean;
        duration sigma;
        size_t run_size;
        size_t batch_size;
        bool converged;
    };

#define TRUN_CLOCK_OVERHEAD_PERC 0.001

    // Run parameters.
    //
    // @Clock: the clock type (any provided by std::chrono).
    // @clock_time: the time it takes to take a measurement with #Clock.
    //     (default: auto-calibrated)
    // @clock_overhead_perc: target clock overhead percentage
    //     (default: TRUN_CLOCK_OVERHEAD_PERC)
    // @mean_err_perc: keep running until
    //     standard error <= mean * mean_err_perc
    // @sigma_outlier_perc: consider outliers those where
    //     |elem - mean| >= sigma_outlier_perc * sigma
    // @warmup: number of warmup runs
    // @init_runs: initial number of runs
    // @init_batch: initial batch size
    // @max_experiments: maximum number of experiments run until non-convergence
    //     is assumed (runs + batch)
    //
    // When the clock is auto-calibrated, the same parameters will be used.
    //
    // The batch size will be increased until:
    //     clock_time <= mean * (clock_overhead_perc / 100)
    template<class Clock>
    class parameters
    {
    public:
        parameters();

        using clock_type = Clock;
        typename result<clock_type>::duration clock_time;
        double clock_overhead_perc;

        double mean_err_perc;
        double sigma_outlier_perc;

        size_t warmup;
        size_t init_runs;
        size_t init_batch;
        size_t max_experiments;
    };

    namespace time {

        // Calibrate clock overheads.
        //
        // If any parameter in #parameters is zero, a sane default will be set
        // and returned.
        template<class Clock>
        parameters<Clock>
        calibrate(const trun::parameters<Clock> & parameters);

    }


    // Calculate the mean time of running 'func(args...)'; also returns the
    // standard error and sample size.
    //
    // The result is ensured with 95% confidence to have a standard error lower
    // than 'mean_err_perc * mean'.
    //
    // If #params.clock_time is zero, #time::calibrate will be used.
    //
    // If results do not converge (#max_experiments), return the mean with lowest
    // standard error found so far.
    //
    // Runs are batched to keep the clock overheads under control.
    template<class Clock, class Func, class... Args>
    result<Clock> && run(const parameters<Clock> && parameters,
                         Func&& func, Args&&... args);

}

#include <trun-detail.hpp>

#endif // TRUN_HPP
