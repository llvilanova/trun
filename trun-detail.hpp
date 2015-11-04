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
#include <trun/detail/parameters.hpp>


//////////////////////////////////////////////////////////////////////
// * Steady clock

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
    }
}


//////////////////////////////////////////////////////////////////////
// * Experiment run

namespace trun {

    namespace detail {

        template<class C>
        typename result<C>::duration duration_raw(const typename result<C>::duration::rep & value) {
                using res_duration = typename result<C>::duration;
                using res_rep = typename result<C>::duration::rep;
		return std::forward<res_duration>(res_duration(res_rep(value)));
        }

        template<class C>
        typename result<C>::duration duration_clock(const typename C::duration & value) {
                using res_duration = typename result<C>::duration;
                using res_rep = typename result<C>::duration::rep;
		return std::forward<res_duration>(res_duration(res_rep(value.count())));
        }

        template<class R, class C, class F, class... Args>
        void do_loops(std::vector<R> & samples,
                      const size_t warmup, const size_t outer, const size_t inner,
                      C clock, F&& func, Args&&... args)
        {
            for (size_t i = 0; i < warmup; i++) {
                func(std::forward<Args>(args)...);
            }

            for (size_t i = 0; i < outer; i++) {
                auto start = clock.now();
                for (size_t j = 0; j < inner; j++) {
                    func(std::forward<Args>(args)...);
                }
                auto end = clock.now();
                auto delta = duration_clock<C>(end - start);
                samples[i] = delta.count();
            }
        }

        template<bool calibrating, class P>
        typename std::enable_if<calibrating, void>::type
        update_clock_time(P & params, const typename result<typename P::clock_type>::duration & time)
        {
            params.clock_time = time;
        }

        template<bool calibrating, class P>
        typename std::enable_if<!calibrating, void>::type
        update_clock_time(P &, const typename result<typename P::clock_type>::duration &)
        {
        }

        // Run workload 'func(args...)' until timing stabilizes.
        //
        // Returns the mean time of one workload unit.
        //
        // Runs two nested loops:
        // - outer
        //   - Calculates mean and standard error; adds resiliency against
        //     transient noise.
        //   - Dynamically sized according to:
        //       http://en.wikipedia.org/wiki/Sample_size_determination#Estimation_of_means
        //         outer = 16 * sigma^2 / width^2
        //         width = mean * mean_err_perc
        //         sima^2 = variance
        //       Increase capped to 'max_outer_multiplier'.
        // - inner
        //   - Aggregates workload on batches; minimizes clock overheads.
        //   - Dynamically sized until:
        //       clock_time <= #inner * mean * clock_overhead_perc
        //       Increase capped to 'max_inner_multiplier' (if not 'calibrating')
        //
        // Stops when:
        //   sigma <= mean_err_perc * mean  => mean
        //   total runs >= max_experiments  => mean with lowest sigma
        template<bool calibrating, class P, class F, class... A>
        void do_run(result<typename P::clock_type> & res, P & params, F&& func, A&&... args)
        {
            using C = typename P::clock_type;
            using rep_type = typename result<C>::duration::rep;

            size_t max_outer_multiplier = 2;
            size_t outer = params.init_runs;
            size_t max_inner_multiplier = 2;
            size_t inner = params.init_batch;

            result<C> res_current;
            result<C> res_best;
            std::vector<rep_type> samples(2 * outer);
            res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());
            bool match = false;
            bool hit_max = false;
            size_t experiments = 0;
            // increase inner every iterations_check iterations
            size_t iterations = 0;
            size_t iterations_check = 10;

            DEBUG("clock_overhead_perc=%f mean_err_perc=%f warmup=%lu "
                  "init_runs=%lu init_batch=%lu max_experiments=%lu",
                  params.clock_overhead_perc, params.mean_err_perc, params.warmup,
                  params.init_runs, params.init_batch, params.max_experiments);

            do {
                // run experiment
                if (samples.size() < outer) {
                    samples.resize(outer);
                }
                do_loops(samples, params.warmup, outer, inner, C(),
                         std::forward<F>(func), std::forward<A>(args)...);

                // calculate statistics
                size_t real_outer;
                {
                    real_outer = 0;
                    res_current.min = res_current.min_nooutliers =
                        duration_raw<C>(std::numeric_limits<rep_type>::max());
                    res_current.max = res_current.max_nooutliers =
                        duration_raw<C>(0);
                    rep_type sum = 0;
                    rep_type sq_sum = 0;
                    for (size_t i = 0; i < outer; i++) {
                        auto elem = samples[i] / inner;
                        samples[i] = elem;
                        if (duration_raw<C>(elem) < res_current.min) {
                            res_current.min = duration_raw<C>(elem);
                        }
                        if (duration_raw<C>(elem) > res_current.max) {
                            res_current.max = duration_raw<C>(elem);
                        }
                        sum += elem;
                        sq_sum += std::pow(elem, 2);
                        real_outer++;
                    }
                    rep_type mean = sum / real_outer;
                    rep_type variance = (sq_sum / real_outer) - std::pow(mean, 2);
                    rep_type sigma = std::sqrt(variance);

                    // remove outliers: |elem - mean| > sigma_outlier_perc * sigma
                    real_outer = sum = sq_sum = 0;
                    for (size_t i = 0; i < outer; i++) {
                        auto elem = samples[i];
                        if (std::abs(elem - mean) < (params.sigma_outlier_perc * sigma)) {
                            if (duration_raw<C>(elem) < res_current.min_nooutliers) {
                                res_current.min_nooutliers = duration_raw<C>(elem);
                            }
                            if (duration_raw<C>(elem) > res_current.max_nooutliers) {
                                res_current.max_nooutliers = duration_raw<C>(elem);
                            }
                            sum += elem;
                            sq_sum += std::pow(elem, 2);
                            real_outer++;
                        }
                    }
                    mean = sum / real_outer;
                    variance = (sq_sum / real_outer) - std::pow(mean, 2);
                    res_current.mean = duration_raw<C>(mean);
                    res_current.sigma = duration_raw<C>(std::sqrt(variance));
                }
                auto width = res_current.mean.count() * params.mean_err_perc;
                res_current.run_size = outer;
                res_current.batch_size = inner;

                iterations++;
                experiments += (outer * inner);

                // keep track of best result in case we hit the run limit
                if (res_current.sigma < res_best.sigma) {
                    res_best = res_current;
                }

                DEBUG("mean=%f sigma=%f width=%f outer=%lu real_outer=%lu inner=%lu experiments=%lu",
                      res_current.mean.count(), res_current.sigma.count(), width,
                      outer, real_outer, inner, experiments);

                // update 'clock_time' if we're in calibration mode
                update_clock_time<calibrating>(params, res_current.mean);

                // keep timing overhead under 'clock_overhead_perc'
                //     clock_time <= new_inner * mean * clock_overhead_perc
                auto old_inner = inner;
                auto new_inner = ((params.clock_time.count() / params.clock_overhead_perc) /
                                  res_current.mean.count());
                if (!calibrating && new_inner > inner * max_inner_multiplier) {
                    inner = std::ceil(inner * max_inner_multiplier);
                } else if (new_inner > inner) {
                    inner = std::ceil(new_inner);
                }
                // increase inner if we're not converging
                if (!match && iterations % iterations_check == 0) {
                    inner *= max_inner_multiplier;
                }

                // keep standard error in 95% under 'mean_err_perc'
                auto new_outer = 16.0 * pow(res_current.sigma.count(), 2) / pow(width, 2);
                if (new_outer > outer * max_outer_multiplier) {
                    outer = std::ceil(outer * max_outer_multiplier);
                } else if (new_outer < params.init_runs) {
                    outer = params.init_runs;
                } else if (inner <= old_inner && new_outer < outer) {
                    // do not decrease outer if we did not increase inner
                } else {
                    outer = std::ceil(new_outer);
                }

                // exit conditions
                match = res_current.sigma.count() <= width;
                if (!match && experiments >= params.max_experiments) {
                    hit_max = true;
                }

            } while((!match || (calibrating && iterations < 2)) && !hit_max);

            if (hit_max) {
                res = res_best;
            } else {
                res = res_current;
            }
            res.converged = !hit_max;
        }

        template<class P>
        void check_parameters(const P & params)
        {
#define _TRUN_PARAM_CHECK(p, eq)                                \
            do {                                                \
                if (params.p < 0) {                             \
                    errx(1, "cannot have negative '" #p "'");   \
                }                                               \
                if (eq && params.p == 0) {                      \
                    errx(1, "cannot have null '" #p "'");       \
                }                                               \
            } while (0)

            _TRUN_PARAM_CHECK(clock_time.count(), false);
            _TRUN_PARAM_CHECK(clock_overhead_perc, false);
            _TRUN_PARAM_CHECK(mean_err_perc, false);
            _TRUN_PARAM_CHECK(init_runs, true);
            _TRUN_PARAM_CHECK(init_batch, true);
#undef _TRUN_PARAM_CHECK
        }

    }

    template<class C, class F, class... A>
    result<C> && run(const parameters<C> & params, F&& func, A&&... args)
    {
        result<C> res;
        if (params.clock_time.count() == 0) {
            auto new_params = time::calibrate(params);
            INFO("Executing benchmark...");
            trun::detail::do_run<false>(res, new_params,
                                        std::forward<F>(func), std::forward<A>(args)...);
        } else {
            detail::check_parameters(params);
            INFO("Executing benchmark...");
            trun::detail::do_run<false>(res, params,
                                        std::forward<F>(func), std::forward<A>(args)...);
        }
        res.mean = time::detail::clock_units<C>(res.mean);
        res.sigma = time::detail::clock_units<C>(res.sigma);
        return std::move(res);
    }

}


//////////////////////////////////////////////////////////////////////
// * Generic clock

namespace trun {
    namespace time {

        template<class C>
        parameters<C>
        calibrate(const parameters<C> & params)
        {
            detail::check<C>();

            // initialize parameters
            parameters<C> res_params = params;
            res_params.clock_time = typename C::duration(typename C::rep(0));
            if (res_params.mean_err_perc == 0) {
                res_params.mean_err_perc = 0.01;
            }
            if (res_params.sigma_outlier_perc == 0.0) {
                res_params.sigma_outlier_perc = 3.0;
            }
            if (res_params.init_runs == 0) {
                res_params.init_runs = 30;
            }
            if (res_params.init_batch == 0) {
                // user benchmarks will probably be longer than timing costs
                res_params.init_batch = 10;
            }
            if (res_params.max_experiments == 0) {
                res_params.max_experiments = 1000000;
            }
            trun::detail::check_parameters(res_params);

            parameters<C> clock_params(res_params);
            clock_params.warmup = 1000;
            clock_params.init_batch = 10000;
            clock_params.max_experiments = 1000000000;

            INFO("Calibrating clock overheads...");
            result<C> res;
            trun::detail::do_run<true>(res, clock_params, C::now);
            if (!res.converged) {
                errx(1, "clock calibration did not converge");
            }

            res_params.clock_time = detail::clock_units<C>(res.mean);

            return std::move(res_params);
        }

    }
}


//////////////////////////////////////////////////////////////////////
// * Parameters

template<class C>
trun::parameters<C>::parameters()
    :clock_time(0)
    ,clock_overhead_perc(TRUN_CLOCK_OVERHEAD_PERC)
    ,mean_err_perc(0)
    ,sigma_outlier_perc(0)
    ,warmup(0)
    ,init_runs(0)
    ,init_batch(0)
    ,max_experiments(0)
{
}

#endif // TRUN_DETAIL_HPP
