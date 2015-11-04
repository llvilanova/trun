/** trun/detail/core.hpp ---
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

#ifndef TRUN__DETAIL__CORE_IMPL_HPP
#define TRUN__DETAIL__CORE_IMPL_HPP 1

#include <limits>

#include <trun/detail/core.hpp>


namespace trun {
    namespace detail {
        namespace core {

            template<class C>
            static inline
            typename result<C>::duration duration_raw(const typename result<C>::duration::rep & value) {
                using res_duration = typename result<C>::duration;
                using res_rep = typename result<C>::duration::rep;
		return std::forward<res_duration>(res_duration(res_rep(value)));
            }

            template<class C>
            static inline
            typename result<C>::duration duration_clock(const typename C::duration & value) {
                using res_duration = typename result<C>::duration;
                using res_rep = typename result<C>::duration::rep;
		return std::forward<res_duration>(res_duration(res_rep(value.count())));
            }

            template<bool calibrating, class P>
            static inline
            typename std::enable_if<calibrating, void>::type
            update_clock_time(P & params, const typename result<typename P::clock_type>::duration & time)
            {
                params.clock_time = time;
            }

            template<bool calibrating, class P>
            static inline
            typename std::enable_if<!calibrating, void>::type
            update_clock_time(P &, const typename result<typename P::clock_type>::duration &)
            {
            }


            template<class R, class C, class F, class... Args>
            static inline
            void do_loops(std::vector<R> & samples,
                          const size_t warmup, const size_t run_size, const size_t inner,
                          C clock, F&& func, Args&&... args)
            {
                for (size_t i = 0; i < warmup; i++) {
                    func(std::forward<Args>(args)...);
                }

                for (size_t i = 0; i < run_size; i++) {
                    auto start = clock.now();
                    for (size_t j = 0; j < inner; j++) {
                        func(std::forward<Args>(args)...);
                    }
                    auto end = clock.now();
                    auto delta = duration_clock<C>(end - start);
                    samples[i] = delta.count();
                }
            }


        }
    }
}

// Run workload 'func(args...)' until timing stabilizes.
//
// Returns the mean time of one workload unit.
//
// Runs two nested loops:
// - run_size
//   - Calculates mean and standard error; adds resiliency against
//     transient noise.
//   - Dynamically sized according to:
//       http://en.wikipedia.org/wiki/Sample_size_determination#Estimation_of_means
//         run_size = 16 * sigma^2 / width^2
//         width = mean * mean_err_perc
//         sima^2 = variance
//       Increase capped to 'max_run_size_multiplier'.
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
static inline
void trun::detail::core::run(result<typename P::clock_type> & res, P & params, F&& func, A&&... args)
{
    using C = typename P::clock_type;
    using rep_type = typename result<C>::duration::rep;

    size_t max_run_size_multiplier = 2;
    size_t run_size = params.run_size;
    size_t max_inner_multiplier = 2;
    size_t inner = params.init_batch;

    result<C> res_current;
    result<C> res_best;
    std::vector<rep_type> samples(2 * run_size);
    res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());
    bool match = false;
    bool hit_max = false;
    size_t experiments = 0;
    // increase inner every iterations_check iterations
    size_t iterations = 0;
    size_t iterations_check = 10;

    DEBUG("clock_overhead_perc=%f mean_err_perc=%f warmup=%lu "
          "runs_size=%lu init_batch=%lu max_experiments=%lu",
          params.clock_overhead_perc, params.mean_err_perc, params.warmup_batch_size,
          params.run_size, params.init_batch, params.max_experiments);

    do {
        // run experiment
        if (samples.size() < run_size) {
            samples.resize(run_size);
        }
        do_loops(samples, params.warmup_batch_size, run_size, inner, C(),
                 std::forward<F>(func), std::forward<A>(args)...);

        // calculate statistics
        size_t real_run_size;
        {
            real_run_size = 0;
            res_current.min = res_current.min_nooutliers =
                duration_raw<C>(std::numeric_limits<rep_type>::max());
            res_current.max = res_current.max_nooutliers =
                duration_raw<C>(0);
            rep_type sum = 0;
            rep_type sq_sum = 0;
            for (size_t i = 0; i < run_size; i++) {
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
                real_run_size++;
            }
            rep_type mean = sum / real_run_size;
            rep_type variance = (sq_sum / real_run_size) - std::pow(mean, 2);
            rep_type sigma = std::sqrt(variance);

            // remove outliers: |elem - mean| > sigma_outlier_perc * sigma
            real_run_size = sum = sq_sum = 0;
            for (size_t i = 0; i < run_size; i++) {
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
                    real_run_size++;
                }
            }
            mean = sum / real_run_size;
            variance = (sq_sum / real_run_size) - std::pow(mean, 2);
            res_current.mean = duration_raw<C>(mean);
            res_current.sigma = duration_raw<C>(std::sqrt(variance));
        }
        auto width = res_current.mean.count() * params.mean_err_perc;
        res_current.run_size = run_size;
        res_current.batch_size = inner;

        iterations++;
        experiments += (run_size * inner);

        // keep track of best result in case we hit the run limit
        if (res_current.sigma < res_best.sigma) {
            res_best = res_current;
        }

        DEBUG("mean=%f sigma=%f width=%f run_size=%lu real_run_size=%lu inner=%lu experiments=%lu",
              res_current.mean.count(), res_current.sigma.count(), width,
              run_size, real_run_size, inner, experiments);

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
        auto new_run_size = 16.0 * pow(res_current.sigma.count(), 2) / pow(width, 2);
        if (new_run_size > run_size * max_run_size_multiplier) {
            run_size = std::ceil(run_size * max_run_size_multiplier);
        } else if (new_run_size < params.run_size) {
            run_size = params.run_size;
        } else if (inner <= old_inner && new_run_size < run_size) {
            // do not decrease run_size if we did not increase inner
        } else {
            run_size = std::ceil(new_run_size);
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

#endif // TRUN__DETAIL__CORE_IMPL_HPP
