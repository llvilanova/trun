/** trun.hpp ---
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
    // Timing numbers are for a single experiment instance, and do not contain
    // outliers. The '_all' fields are the only ones that contain outliers. The
    // @batch_size field is for the last batch only.
    template<class Clock>
    class result
    {
    public:
        using duration = std::chrono::duration<double, typename Clock::period>;

        duration min, min_all;
        duration max, max_all;
        duration mean, mean_all;
        duration sigma, sigma_all;
        size_t batches, batches_all;
        bool converged;

        struct batch
        {
            duration time;
            bool outlier;
        };

        // Has contents only when #mod_get_runs is used in trun().
        std::vector<batch> runs;

        // Scale this results down (divide) by the given factor.
        //
        // Useful when the benchmarked function actually contains multiple
        // instances of the experiment (e.g., an unrolled loop).
        result<Clock> scale(unsigned long long factor) const;

        // Return new results with units converted according to #ClockTarget.
        template<class ClockTarget>
        result<ClockTarget> convert() const;
    };

#define TRUN_STDDEV_PERC 1              // 1% of the mean
#define TRUN_CONFIDENCE_SIGMA 2         // 2 * sigma -> 95.45%
#define TRUN_CONFIDENCE_OUTLIER_SIGMA 4 // 4 * sigma -> 99.99%
#define TRUN_EXPERIMENT_TIMEOUT 300     // timeout after 5min

#define TRUN_WARMUP_BATCH_GROUP_SIZE 0
#define TRUN_INITIAL_BATCH_GROUP_SIZE 1
#define TRUN_BATCH_GROUP_MIN_SIZE 30 // rule of thumb for statistical significance

#define TRUN_CLOCK_OVERHEAD_PERC 0.1    // 0.1% timing overhead
#define TRUN_WARMUP_BATCH_SIZE 0
#define TRUN_INITIAL_BATCH_SIZE 1

    // Run parameters.
    //
    // @stddev_perc: target standard deviation
    //     (default: TRUN_STDDEV_PERC)
    //     The value is set in terms of a percentage of the measured mean. If
    //     set to zero, runs the set of experiments only once.
    // @confidence_sigma: target confidence interval
    //     (default: TRUN_CONFIDENCE_SIGMA)
    //     The value is set in terms of sigma (stddev / 2). See 68-95-99.7 rule
    //     (https://en.wikipedia.org/wiki/68-95-99.7_rule#Table_of_numerical_values).
    //     Used to select the number of experiment batches (batch group) to compute
    //     statistics across
    //     (https://en.wikipedia.org/wiki/Sample_size_determination#Estimation_of_a_mean):
    //       batch group = (2 * confidence_sigma * sigma)^2 / width^2
    //       width = mean * stddev_perc
    //       sigma^2 = variance
    //     If set to zero, uses the fixed value of @batch_group_size.
    // @confidence_outlier_sigma: target confidence interval to discard outliers
    //     (default: TRUN_CONFIDENCE_OUTLIER_SIGMA)
    //     The value is set in terms of sigma (stddev / 2).  Ignores experiments
    //     where:
    //       confidence_outlier_sigma * sigma_all > |time - mean_all|
    // @experiment_timeout: maximum running time (seconds) until non-convergence
    //     (default: TRUN_EXPERIMENT_TIMEOUT)
    //
    // @warmup_batch_group_size: number of warmup experiments before every batch group
    //     (default: TRUN_WARMUP_BATCH_GROUP_SIZE)
    // @batch_group_size: number of batches to calculate statistics on
    //     (default: TRUN_INITIAL_BATCH_GROUP_SIZE)
    //     Each batch group is executed entirely before recalculating
    //     statistics. The value is dynamically-adapted, unless
    //     @confidence_sigma is set to zero.
    // @batch_group_min_size: minimum number of batches to calculate statistics on
    //     (default: TRUN_BATCH_GROUP_MIN_SIZE)
    //     This bounds the lower limit calculated through @confidence_sigma.
    //
    // @Clock: the clock type (any provided by std::chrono).
    // @clock_time: the time it takes to take a measurement with #Clock.
    //     (default: auto-calibrated)
    // @clock_overhead_perc: target clock overhead percentage
    //     (default: TRUN_CLOCK_OVERHEAD_PERC)
    //     Respective to the time of a single measurement. Used to select the
    //     size of experiment batches:
    //       batch_size >= clock_time / ((mean - sigma) * clock_overhead_perc / 100)
    //     If set to zero, uses the fixed value of @batch_size.
    // @warmup_batch_size: number of warmup experiments before every timed batch
    //     (default: TRUN_WARMUP_BATCH_SIZE)
    // @batch_size: number of experiment instances on each batch
    //     (default: TRUN_INITIAL_BATCH_SIZE)
    //     Each batch is timed separately to reduce clock overheads. The value
    //     is dynamically adapted, unless @clock_overhead_perc is set to zero.
    template<class Clock>
    class parameters
    {
    public:
        parameters();

        double stddev_perc;
        double confidence_sigma;
        double confidence_outlier_sigma;
        double experiment_timeout;

        size_t warmup_batch_group_size;
        size_t batch_group_size;
        size_t batch_group_min_size;

        using clock_type = Clock;
        typename result<clock_type>::duration clock_time;
        double clock_overhead_perc;
        size_t warmup_batch_size;
        size_t batch_size;

        // Return new parameters with units converted according to #ClockTarget.
        template<class ClockTarget>
        parameters<ClockTarget> convert() const;

        // Return new parameters tweaked for clock calibration.
        static parameters<Clock> get_clock_params();
    };


    // See below.
    class mod_clock_type;

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

        // Calibrate clock overheads.
        //
        // See run() with #trun::mod_clock below.
        template<class Clock, trun::message msg = trun::message::NONE, class Func>
        static
        parameters<Clock> calibrate(Func&& func, trun::mod_clock_type &mod_clock);

        // Calibrate clock overheads.
        template<class Clock, trun::message msg = trun::message::NONE, class Func>
        static
        parameters<Clock> calibrate(const trun::parameters<Clock> & parameters,
                                    Func&& func, trun::mod_clock_type &mod_clock);

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


    // Run modifier arguments

    // Fill in the elements in #result<Clock>.runs (default is not to do it).
    class mod_get_runs_type {};
    mod_get_runs_type mod_get_runs;

    // Assume @func runs an entire batch and returns its timing.
    //
    // Useful to time functions in child processes, kernel module functions,
    // etc. where externally timing a call to the function includes code we do
    // not want to include in the timing result.
    //
    // A regular trun::run() to time func() is equivalent to passing the
    // following func_run():
    //
    //   template<class Clock>
    //   result<Clock>::duration func_run(size_t batch_group
    //                                    size_t warmup_batch_size,
    //                                    size_t batch_size)
    //   {
    //       for (size_t i = 0; i < warmup_batch_size; i++) {
    //           func();
    //           asm volatile("" : : : "memory");
    //       }
    //       auto start = Clock::now();
    //       for (size_t i = 0; i < batch_size; i++) {
    //           func();
    //           asm volatile("" : : : "memory");
    //       }
    //       auto end = clock::now();
    //       return end - start;
    //   }
    //
    //   int main(int argc, char *argv[])
    //   {
    //       trun::run(func_run, trun::mod_clock);
    //   {
    class mod_clock_type {};
    mod_clock_type mod_clock;


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
    // If #parameters.clock_time is zero, it will use #trun::time::calibrate
    // with default parameters.
    //
    // If results do not converge in the given time
    // (#parameters.experiment_timeout), returns the results with the lowest
    // standard deviation found so far.
    //
    // You can pass the objects in trun::mod_* to modify the behavior of
    // trun::run(). See above for their meaning.
    template<trun::message msg = trun::message::NONE, class Clock,
             class Func, class... Args>
    static
    result<Clock>
    run(const parameters<Clock> & parameters, Func&& func,
        Args&& ...args);

    // Same with default parameters
    template<class Clock = ::trun::time::default_clock, trun::message msg = trun::message::NONE,
             class Func, class... Args>
    static
    result<Clock>
    run(Func&& func, Args&& ...args);


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
