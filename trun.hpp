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


// Show debug messages
#ifndef TRUN_DEBUG
#define TRUN_DEBUG 0
#endif

// Show info messages
#ifndef TRUN_INFO
#define TRUN_INFO 0
#endif


namespace trun {

    // Experiment run results.
    //
    // Timing numbers are for a single experiment.
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
#define TRUN_WARMUP_BATCH_SIZE 0
#define TRUN_RUN_SIZE 30                // minimal statistical significance
#define TRUN_BATCH_SIZE 1
#define TRUN_MAX_EXPERIMENTS 1000000

    // Run parameters.
    //
    // @Clock: the clock type (any provided by std::chrono).
    // @clock_time: the time it takes to take a measurement with #Clock.
    //     (default: auto-calibrated)
    // @clock_overhead_perc: target clock overhead percentage
    //     (default: TRUN_CLOCK_OVERHEAD_PERC)
    //     Respective to the time of a single measurement.
    // @mean_err_perc: keep running until
    //     standard error <= mean * mean_err_perc
    // @sigma_outlier_perc: consider outliers those where
    //     |elem - mean| >= sigma_outlier_perc * sigma
    // @warmup_batch_size: number of experiments before every round
    //     (default: TRUN_WARMUP_BATCH_SIZE)
    // @run_size: initial number of runs
    //     (default: TRUN_RUN_SIZE)
    //     Statistics are calculated across runs.
    // @batch_size: initial number of iterations to batch together
    //     (default: TRUN_BATCH_SIZE)
    //     Every batch is timed separately to reduce clock overheads.
    // @max_experiments: maximum number of experiments until non-convergence
    //     (default: TRUN_MAX_EXPERIMENTS)
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

        size_t warmup_batch_size;
        size_t run_size;
        size_t batch_size;
        size_t max_experiments;
    };

    namespace time {

        // Calibrate clock overheads.
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
    template<class Clock, class Func, class... Args>
    result<Clock> && run(const parameters<Clock> & parameters,
                         Func&& func, Args&&... args);

}

#include <trun/detail/run.hpp>

#endif // TRUN_HPP
