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
                }

                for (size_t i = 0; i < run_size; i++) {
                    auto start = clock.now();
                    for (size_t j = 0; j < batch_size; j++) {
                        func(std::forward<Args>(args)...);
                    }
                    auto end = clock.now();
                    auto delta = duration_clock<C>(end - start);
                    samples[i] = delta.count();
                }
            }

            template<class Clock>
            class result : public ::trun::result<Clock>
            {
            public:
                size_t count;
            };

            template<bool nooutliers, class Clock>
            static inline
            result<Clock> stats(const parameters<Clock> &params,
                                std::vector<typename result<Clock>::duration::rep> &samples,
                                typename result<Clock>::duration::rep mean,
                                typename result<Clock>::duration::rep sigma)
            {
                using rep_type = typename result<Clock>::duration::rep;

                result<Clock> res;
                res.min = res.min_nooutliers = duration_raw<Clock>(
                    std::numeric_limits<rep_type>::max());
                res.max = res.max_nooutliers = duration_raw<Clock>(
                    std::numeric_limits<rep_type>::min());

                rep_type sum = 0;
                rep_type sq_sum = 0;
                res.count = 0;

                const auto it_end = samples.begin() + params.run_size;
                for (auto it = samples.begin(); it != it_end; it++) {
                    auto elem = *it;
                    if (!nooutliers) {
                        elem = elem / params.batch_size;
                        *it = elem;
                    }

                    if (!nooutliers ||
                        std::abs(elem - mean) < (params.sigma_outlier_perc * sigma)) {
                        if (nooutliers) {
                            res.min_nooutliers = std::min(duration_raw<Clock>(elem),
                                                          res.min_nooutliers);
                            res.max_nooutliers = std::max(duration_raw<Clock>(elem),
                                                          res.max_nooutliers);
                        } else {
                            res.min = std::min(duration_raw<Clock>(elem), res.min);
                            res.max = std::max(duration_raw<Clock>(elem), res.max);
                        }

                        sum += elem;
                        sq_sum += std::pow(elem, 2);
                        res.count++;
                    }
                }

                if (res.count == 0) {
                    errx(1, "unexpected error");
                }

                rep_type s_mean = sum / res.count;
                rep_type s_variance = (sq_sum / res.count) - std::pow(s_mean, 2);
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
// - batch_size
//   - Aggregates workload on batches; minimizes clock overheads.
//   - Dynamically sized until:
//       clock_time <= #batch_size * mean * clock_overhead_perc
//       Increase capped to 'max_batch_size_multiplier' (if not 'calibrating')
//
// Stops when:
//   sigma <= mean_err_perc * mean  => mean
//   total runs >= max_experiments  => mean with lowest sigma
template<bool calibrating, class P, class F, class... A>
static inline
void trun::detail::core::run(::trun::result<typename P::clock_type> & res, P & params, F&& func, A&&... args)
{
    using C = typename P::clock_type;
    using rep_type = typename result<C>::duration::rep;

    size_t max_run_size_multiplier = 2;
    size_t max_batch_size_multiplier = 2;
    parameters<C> p = params;

    std::vector<rep_type> samples(p.run_size);
    result<C> res_curr;
    result<C> res_best;
    res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());

    bool match = false;
    bool hit_max = false;
    size_t experiments = 0;
    // increase batch_size every iterations_check iterations
    size_t iterations = 0;
    size_t iterations_check = 10;

    DEBUG("clock_overhead_perc=%f mean_err_perc=%f warmup=%lu "
          "runs_size=%lu batch_size=%lu max_experiments=%lu",
          p.clock_overhead_perc, p.mean_err_perc, p.warmup_batch_size,
          p.run_size, p.batch_size, p.max_experiments);

    do {
        // run experiment batches
        if (samples.size() < p.run_size) {
            samples.resize(p.run_size);
        }
        loops(samples, p.warmup_batch_size, p.run_size, p.batch_size, C(),
              std::forward<F>(func), std::forward<A>(args)...);

        // calculate statistics
        {
            result<C> res_all = stats<false>(p, samples, 0, 0);
            res_curr = stats<true>(p, samples, res_all.mean.count(), res_all.sigma.count());
        }
        auto width = res_curr.mean.count() * p.mean_err_perc;

        iterations++;
        experiments += (p.run_size * p.batch_size);

        // keep track of best result in case we hit the run limit
        if (res_curr.sigma < res_best.sigma) {
            res_best = res_curr;
        }

        DEBUG("mean=%f sigma=%f width=%f run_size=%lu run_size_nooutl=%lu batch_size=%lu experiments=%lu",
              res_curr.mean.count(), res_curr.sigma.count(), width,
              p.run_size, res_curr.count, p.batch_size, experiments);

        // update 'clock_time' if we're in calibration mode
        update_clock_time<calibrating>(p, res_curr.mean);

        // keep timing overhead under 'clock_overhead_perc'
        //     clock_time <= new_batch_size * mean * clock_overhead_perc
        auto old_batch_size = p.batch_size;
        {
            auto new_batch_size = ((p.clock_time.count() / p.clock_overhead_perc) /
                                   res_curr.mean.count());
            if (!calibrating && new_batch_size > p.batch_size * max_batch_size_multiplier) {
                p.batch_size = std::ceil(p.batch_size * max_batch_size_multiplier);
            } else if (new_batch_size > p.batch_size) {
                p.batch_size = std::ceil(new_batch_size);
            }
            // increase batch_size if we're not converging
            if (!match && iterations % iterations_check == 0) {
                p.batch_size *= max_batch_size_multiplier;
            }
        }

        // keep standard error in 95% under 'mean_err_perc'
        {
            auto new_run_size = 16.0 * pow(res_curr.sigma.count(), 2) / pow(width, 2);
            if (new_run_size > p.run_size * max_run_size_multiplier) {
                p.run_size = std::ceil(p.run_size * max_run_size_multiplier);
            } else if (new_run_size < params.run_size) {
                p.run_size = params.run_size;
            } else if (p.batch_size <= old_batch_size && new_run_size < p.run_size) {
                // do not decrease run_size if we did not increase batch_size
            } else {
                p.run_size = std::ceil(new_run_size);
            }
        }

        // exit conditions
        match = res_curr.sigma.count() <= width;
        if (!match && experiments >= p.max_experiments) {
            hit_max = true;
        }

    } while((!match || (calibrating && iterations < 2)) && !hit_max);

    if (hit_max) {
        res = res_best;
    } else {
        res = res_curr;
    }
    res.run_size = p.run_size;
    res.batch_size = p.batch_size;
    res.converged = !hit_max;
}

#endif // TRUN__DETAIL__CORE_IMPL_HPP
