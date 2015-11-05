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
            void loops(std::vector<R> & samples,
                       const size_t warmup, const size_t run_size, const size_t batch_size,
                       C clock, F&& func, Args&&... args)
            {
                for (size_t i = 0; i < warmup; i++) {
                    func(std::forward<Args>(args)...);
                    asm volatile("" : : : "memory");
                }

                for (size_t i = 0; i < run_size; i++) {
                    auto start = clock.now();
                    for (size_t j = 0; j < batch_size; j++) {
                        func(std::forward<Args>(args)...);
                        asm volatile("" : : : "memory");
                    }
                    auto end = clock.now();
                    auto delta = duration_clock<C>(end - start);
                    samples[i] = delta.count();
                }
            }

            template<bool first, class Clock>
            static inline
            result<Clock> stats(const parameters<Clock> &params,
                                std::vector<typename result<Clock>::duration::rep> &samples,
                                const typename result<Clock>::duration::rep mean,
                                const typename result<Clock>::duration::rep sigma)
            {
                using rep_type = typename result<Clock>::duration::rep;

                result<Clock> res;
                res.min = res.min_all = duration_raw<Clock>(
                    std::numeric_limits<rep_type>::max());
                res.max = res.max_all = duration_raw<Clock>(
                    std::numeric_limits<rep_type>::min());
                res.run_size = 0;
                res.run_size_all = params.run_size;
                res.batch_size = params.batch_size;

                rep_type outlier = params.confidence_outlier_sigma * sigma;
                if (first) {
                    outlier = std::numeric_limits<rep_type>::max();
                }
                rep_type sum = 0;
                rep_type sq_sum = 0;

                const auto it_end = samples.begin() + params.run_size;
                for (auto it = samples.begin(); it != it_end; it++) {
                    // normalize to per-experiment results
                    auto elem = *it;
                    elem = elem / params.batch_size;

                    // statistics of all experiments
                    res.min_all = std::min(duration_raw<Clock>(elem), res.min_all);
                    res.max_all = std::max(duration_raw<Clock>(elem), res.max_all);

                    // ignore outliers
                    if (std::abs(elem - mean) > outlier) {
                        // printf("x %f | %f | %f | %f | %d\n", elem, mean, sigma, outlier, first);
                        continue;
                    }

                    // statistics of non-outliers
                    *it = elem;
                    res.min = std::min(duration_raw<Clock>(elem), res.min);
                    res.max = std::max(duration_raw<Clock>(elem), res.max);
                    sum += elem;
                    sq_sum += std::pow(elem, 2);
                    res.run_size++;
                }

                rep_type s_mean = sum / res.run_size;
                rep_type s_variance = (sq_sum / res.run_size) - std::pow(s_mean, 2);
                rep_type s_sigma = std::sqrt(s_variance);

                res.mean = duration_raw<Clock>(s_mean);
                res.sigma = duration_raw<Clock>(s_sigma);

                return std::move(res);
            }

        }
    }
}

// Run workload 'func(args...)' until timing stabilizes.
//
// Returns the mean time of one workload unit.
//
// NOTE: stddev == 2*sigma
//
// Runs two nested loops:
// - run_size
//   - Calculates mean and standard error; adds resiliency against
//     transient noise.
//   - Dynamically sized according to:
//       https://en.wikipedia.org/wiki/Sample_size_determination#Means
//         run_size = (2 * confidence_sigma * sigma)^2 / width^2
//         width = mean * stddev_perc
//         sigma^2 = variance
//       Increase capped to 'max_run_size_multiplier'.
// - batch_size
//   - Aggregates workload on batches; minimizes clock overheads.
//   - Dynamically sized until:
//       clock_time <= #batch_size * (mean - sigma) * clock_overhead
//       Increase capped to 'max_batch_size_multiplier' (if not 'calibrating')
//
// Stops when:
//   stddev <= mean * (stddev_perc / 100)  --> mean
//   total runs >= max_experiments         --> mean with lowest sigma
template<bool calibrating, class P, class F, class... A>
static inline
void trun::detail::core::run(::trun::result<typename P::clock_type> & res, P & params, F&& func, A&&... args)
{
    using C = typename P::clock_type;
    using rep_type = typename result<C>::duration::rep;

    double max_run_size_multiplier = 3;
    double max_batch_size_multiplier = 3;
    parameters<C> p = params;
    double clock_overhead = p.clock_overhead_perc / 100;
    double stddev_ratio = p.stddev_perc / 100;
    size_t old_batch_size;

    std::vector<rep_type> samples(p.run_size);
    result<C> res_prev;
    res_prev.sigma = duration_raw<C>(0);
    res_prev.mean = duration_raw<C>(0);
    result<C> res_curr;
    result<C> res_best;
    res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());

    bool match = false;
    bool hit_max = false;
    size_t experiments = 0;
    size_t iterations = 0;

    DEBUG("clock_overhead_perc=%f confidence_sigma=%f stddev_perc=%f"
          "warmup=%lu runs_size=%lu batch_size=%lu max_experiments=%lu",
          p.clock_overhead_perc, p.confidence_sigma, p.stddev_perc,
          p.warmup_batch_size, p.run_size, p.batch_size, p.max_experiments);

    do {
        old_batch_size = p.batch_size;

        // run experiment batches
        if (samples.size() < p.run_size) {
            samples.resize(p.run_size);
        } else if (samples.size() > p.run_size * 2) {
            samples.resize(p.run_size * 2);
        }
        loops(samples, p.warmup_batch_size, p.run_size, p.batch_size, C(),
              std::forward<F>(func), std::forward<A>(args)...);
        experiments += (p.run_size * p.batch_size);
        iterations++;

        // calculate statistics
        res_prev = res_curr;
        if (iterations == 1) {
            res_curr = stats<true>(p, samples, res_prev.mean.count(), res_prev.sigma.count());
        } else {
            res_curr = stats<false>(p, samples, res_prev.mean.count(), res_prev.sigma.count());
        }
        auto width = res_curr.mean.count() * stddev_ratio;

        // keep track of best result in case we hit the run limit
        if (res_curr.sigma < res_best.sigma) {
            res_best = res_curr;
        }

        DEBUG("mean=%f sigma=%f width=%f run_size=%lu batch_size=%lu experiments=%lu",
              res_curr.mean.count(), res_curr.sigma.count(), width,
              p.run_size, p.batch_size, experiments);

        // update 'clock_time' if we're in calibration mode
        update_clock_time<calibrating>(p, res_curr.mean);

        // keep timing overhead under control
        {
            // conservative mean for calculating batch
            auto mean = res_curr.mean.count() - res_curr.sigma.count();
            if (mean < 0) {
                mean = res_curr.mean.count();
            }
            auto max_batch_size = p.batch_size * max_batch_size_multiplier;
            auto min_batch_size = p.batch_size / max_batch_size_multiplier;
            auto new_batch_size = p.clock_time.count() / (mean * clock_overhead);
            if (new_batch_size > max_batch_size && iterations == 1) {
                p.batch_size = std::ceil(max_batch_size);
            } else if (new_batch_size < min_batch_size && iterations > 1) {
                p.batch_size = std::ceil(min_batch_size);
            } else if (new_batch_size > p.batch_size){
                p.batch_size = std::ceil(new_batch_size);
            }
        }

        // calculate new population size
        {
            auto old_run_size = p.run_size;
            auto max_run_size = p.run_size * max_run_size_multiplier;
            auto min_run_size = p.run_size / max_run_size_multiplier;
            auto new_run_size = pow((2 * p.confidence_sigma * res_curr.sigma.count()) / width, 2);
            if (new_run_size > max_run_size && iterations == 1) {
                p.run_size = std::ceil(max_run_size);
            } else if (new_run_size < min_run_size && iterations > 1) {
                p.run_size = std::ceil(min_run_size);
            } else if (new_run_size > p.run_size){
                p.run_size = std::ceil(new_run_size);
            }
            if (old_run_size >= TRUN_RUN_SIZE && p.run_size < TRUN_RUN_SIZE) {
                p.run_size = TRUN_RUN_SIZE;
            }
        }

        // check if results are significant
        if (res_curr.run_size < TRUN_RUN_SIZE && res_curr.run_size < p.run_size) {
            continue;
        }

        // exit conditions
        match = res_curr.sigma.count() <= width;
        if (!match && experiments >= p.max_experiments) {
            hit_max = true;
        }
    } while((!match
             || (calibrating && iterations < 2)
             || (!calibrating && old_batch_size < 2))
            && !hit_max);

    if (hit_max) {
        res = res_best;
    } else {
        res = res_curr;
    }
    res.converged = !hit_max;
}

#endif // TRUN__DETAIL__CORE_IMPL_HPP
