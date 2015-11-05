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
    // Timing numbers are for a single experiment, and do not contain
    // outliers. The '_all' fields are the only ones that contain outliers.
    template<class Clock>
    class result
    {
    public:
        using duration = std::chrono::duration<double, typename Clock::period>;

        duration min, min_all;
        duration max, max_all;
        duration mean;
        duration sigma;
        size_t run_size, run_size_all;
        size_t batch_size;
        bool converged;
    };

#define TRUN_CLOCK_OVERHEAD_PERC 0.1    // 0.1% timing overhead
#define TRUN_CONFIDENCE_SIGMA 2         // 2 * sigma -> 95.45%
#define TRUN_CONFIDENCE_OUTLIER_SIGMA 4 // 4 * sigma -> 99.99%
#define TRUN_STDDEV_PERC 1              // 1% of the mean
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
    // @confidence_sigma: target confidence interval
    //     (default: TRUN_CONFIDENCE_SIGMA)
    //     The value is set in terms of sigma (stddev / 2). See 68-95-99.7 rule
    //     (https://en.wikipedia.org/wiki/68-95-99.7_rule#Table_of_numerical_values).
    // @confidence_outlier_sigma: target confidence interval to discard outliers
    //     (default: TRUN_CONFIDENCE_OUTLIER_SIGMA)
    //     The value is set in terms of sigma (stddev / 2).
    // @stddev_perc: target standard deviation
    //     (default: TRUN_STDDEV_PERC)
    //     The value is set in terms of a percentage of the mean.
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
    template<class Clock>
    class parameters
    {
    public:
        parameters();

        using clock_type = Clock;
        typename result<clock_type>::duration clock_time;
        double clock_overhead_perc;

        double confidence_sigma;
        double confidence_outlier_sigma;
        double stddev_perc;

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


    // Time the experiment 'func(args...)'.
    //
    // The result is ensured with the target confidence
    // (#params.confidence_sigma) to have a standard deviation lower than a
    // given percentage of the mean:
    //
    //     stddev <= mean * (stddev_perc / 100)
    //
    // This assumes that experiments follow a gaussian distribution, and ignores
    // those beyond #parameters.confidence_outlier_sigma * sigma of the mean:
    //
    // If #parameters.clock_time is zero, #time::calibrate will be used.
    //
    // It accounts for time measurement overheads by ensuring that:
    //
    //     clock_time <= mean * (clock_overhead_perc / 100)
    //
    // If results do not converge (#parameters.max_experiments is reached),
    // return the mean with lowest standard deviation found so far.
    template<class Clock, class Func, class... Args>
    result<Clock> && run(const parameters<Clock> & parameters,
                         Func&& func, Args&&... args);

}

#include <trun/detail/run.hpp>

#endif // TRUN_HPP
