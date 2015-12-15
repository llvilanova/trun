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
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>


namespace trun {

    enum class message {
        NONE,
        INFO,
        DEBUG,
        DEBUG_VERBOSE,
    };

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

        // Mean experiment duration of each run (including outliers)
        //
        // Has contents only when @get_runs is used in trun().
        std::vector<duration> runs;

        // Scale this results down (divide) by the given factor.
        //
        // Useful when the benchmarked function actually contains multiple
        // instances of the experiment (e.g., an unrolled loop).
        result<Clock> scale(unsigned long long factor) const;

        // Return new results with units converted according to #ClockTarget.
        template<class ClockTarget>
        result<ClockTarget> convert() const;
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
    // @run_size: minimum number of runs
    //     (default: TRUN_RUN_SIZE)
    //     Statistics are calculated across runs. At least this number of
    //     experiment runs will be executed.
    // @run_size_min_significance: minimum number of runs for statistical significance
    //     (default: TRUN_RUN_SIZE)
    //     Experiments with less than this number of non-outlier results will be
    //     discarded.
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
        size_t run_size_min_significance;
        size_t batch_size;
        size_t max_experiments;

        // Return new parameters with units converted according to #ClockTarget.
        template<class ClockTarget>
        parameters<ClockTarget> convert() const;
    };


    // Manage time.
    namespace time {

        using default_clock = std::chrono::steady_clock;

        // Calibrate clock overheads.
        //
        // Uses slightly stricter parameters than the default ones.
        template<class Clock = default_clock, trun::message msg = trun::message::NONE>
        static
        parameters<Clock> calibrate();

        // Calibrate clock overheads.
        template<class Clock, trun::message msg = trun::message::NONE>
        static
        parameters<Clock> calibrate(const trun::parameters<Clock> & parameters);

        // Get the units name for a given clock and ratio.
        //
        // Example:
        //    units<std::nano>(std::chrono::steady_clock()) == "ns"
        //    units<std::milli>(std::chrono::steady_clock()) == "ms"
        //    units<std::ratio<60>>(std::chrono::steady_clock()) == "min"
        template<class Clock, class Ratio>
        static
        std::string units(const Clock &clock);

        // Count processor cycles.
        class tsc_cycles {
        public:
            using rep = unsigned long long;
            using period = std::ratio<1>;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<tsc_cycles>;

            // Get timestamp.
            static time_point now();

            // Check if the TSC is usable.
            static void check();
            // Get TSC frequency (approximated).
            static unsigned long long frequency();
            // Get cycle time.
            static std::chrono::duration<double, std::pico> cycle_time();
            // Translate number of cycles into time.
            template<class Rep, class Period>
            static std::chrono::duration<double, std::pico> time(
                const std::chrono::duration<Rep, Period> &d);
        };

        // Use the TSC to count time.
        class tsc_clock {
        public:
            using rep = double;
            using period = std::milli;
            using duration = std::chrono::duration<rep, period>;
            using time_point = std::chrono::time_point<tsc_clock>;

            static time_point now();
        };

    }

    // Time the experiment 'func()'.
    //
    // You can use a functor or a lambda to invoke functions with arguments:
    //
    //     auto f = []() { func(args...); };
    //     auto res = trun::run(f)
    //     trun::dump::csv<std::nano>(res);
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
    // If #parameters.clock_time is zero, #time::calibrate with default
    // parameters will be used.
    //
    // It accounts for time measurement overheads by ensuring that:
    //
    //     clock_time <= mean * (clock_overhead_perc / 100)
    //
    // If results do not converge (#parameters.max_experiments is reached),
    // return the mean with lowest standard deviation found so far.
    //
    // If template argument @get_runs is used, the result will contain elements
    // in #result<Clock>.runs.
    //
    // You can provide the 'func_*' arguments to gather additional statistics on
    // each batch. See above for their meaning. The timing results do not
    // include the calls to these functions.
    //
    // This function accepts 6 optional callbacks (all arguments are size_t):
    //
    // - (0) func_iter_start  (iter, run_size, batch_size)
    // - (1) func_batch_start (iter, run     , batch_size)
    // - (2) func_batch_stop  (iter, run     , batch_size)
    // - (3) func_iter_stop   (iter, run_size, batch_size)
    // - (4) func_batch_select(iter, run     , batch_size)
    // - (5) func_iter_select (iter, run_size, batch_size)
    //
    // - 0,3: Signal start/stop of an iteration (outermost loop: all runs + batches)
    // - 1,2: Signal start/stop of a run (one batch)
    // - 4  : Signal selection of a batch as a non-outlier
    // - 5  : Signal selection of an iteration as candidate for final result.
    //        The last iteration selected corresponds to the final results.
    //        Argument #run_size does not include outliers.
    template<trun::message msg = trun::message::NONE, bool get_runs = false,
             class Clock, class Func, class... FuncCB>
    static
    typename std::enable_if< sizeof...(FuncCB) == 0 || sizeof...(FuncCB) == 6,
                             result<Clock> >::type
    run(const parameters<Clock> & parameters, Func&& func,
        FuncCB&& ...func_cbs);

    // Same with default parameters
    template<class Clock = ::trun::time::default_clock, trun::message msg = trun::message::NONE,
             bool get_runs = false, class Func, class... FuncCB>
    static
    typename std::enable_if< sizeof...(FuncCB) == 0 || sizeof...(FuncCB) == 6,
                             result<Clock> >::type
    run(Func&& func, FuncCB&& ...func_cbs);


    // Dump results.
    namespace dump {

        // Dump results using comma-separate value (CSV) format
        //
        // @results: the results to dump
        // @output: output stream
        // @show_header: whether to dump the CSV header
        // @force_converged: fail if results are not converged
        //
        // You can easily dump results onto a file:
        //     std::ofstream output("/some/path", std::ios::out);
        //     trun::dump::csv(results, output);
        template<class Ratio = std::ratio<1>, class Clock>
        static
        void csv(const result<Clock> &results,
                 std::ostream &output = std::cout,
                 bool show_header = true,
                 bool force_converged = true);

        // Dump CSV header
        template<class Ratio = std::ratio<1>, class Clock>
        static
        void csv_header(const result<Clock> &results,
                        std::ostream &output = std::cout,
                        bool show_header = true,
                        bool force_converged = true);

        // Dump experiment run results using comma-separate value (CSV) format
        template<class Ratio = std::ratio<1>, class Clock>
        static
        void csv_runs(const result<Clock> &results,
                      std::ostream &output = std::cout,
                      bool show_header = true,
                      bool force_converged = true);

    }
}

#include <trun/detail/result.hpp>
#include <trun/detail/run.hpp>
#include <trun/detail/dump.hpp>

#endif // TRUN_HPP
