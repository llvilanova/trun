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

#include <cmath>
#include <err.h>
#include <limits>

#include <trun/detail/core.hpp>


namespace trun {
    namespace detail {
        namespace core {

            template<int index, class... Fcb>
            typename std::enable_if<std::tuple_size<std::tuple<Fcb...>>::value == 0,
                                    void>::type
            func_call(const size_t& s1, const size_t& s2, const size_t& s3,
                           Fcb&&... func_cbs)
            {
            }

            template<int index, class... Fcb>
            typename std::enable_if<(std::tuple_size<std::tuple<Fcb...>>::value > 0),
                                    void>::type
            func_call(const size_t& s1, const size_t& s2, const size_t& s3,
                           Fcb&&... func_cbs)
            {
                std::tuple<Fcb...> cbs(std::forward<Fcb>(func_cbs)...);
                if (std::tuple_size<decltype(cbs)>::value >= (index+1)) {
                    std::get<index>(cbs)(s1, s2, s3);
                }
            }

            template<class... F>
            void func_iter_start(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<0>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class... F>
            void func_batch_start(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<1>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class... F>
            void func_batch_stop(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<2>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class... F>
            void func_iter_stop(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<3>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class... F>
            void func_batch_select(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<4>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class... F>
            void func_iter_select(const size_t& s1, const size_t& s2, const size_t& s3, F&&... f)
            {
                func_call<5>(s1, s2, s3, std::forward<F>(f)...);
            }

            template<class C>
            static inline
            typename result<C>::duration duration_raw(const typename result<C>::duration::rep & value) {
                return typename result<C>::duration(value);
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

            template<class C, class R, class F>
            static __attribute__((noinline))
            void loops_warmup(std::vector<R> & samples,
                              const size_t warmup, const size_t run_size, const size_t batch_size,
                              F&& func)
            {
                for (size_t i = 0; i < warmup; i++) {
                    func();
                    asm volatile("" : : : "memory");
                }
            }

            template<class C, class R, class F, class... Fcb>
            static __attribute__((noinline))
            void loops_work(std::vector<R> & samples,
                            const size_t iteration,
                            const size_t warmup, const size_t run_size, const size_t batch_size,
                            F&& func, Fcb&&... func_cbs)
            {
                func_iter_start(iteration, run_size, batch_size,
                                std::forward<Fcb>(func_cbs)...);

                for (size_t i = 0; i < run_size; i++) {
                    func_batch_start(iteration, i, batch_size,
                                     std::forward<Fcb>(func_cbs)...);
                    auto start = C::now();
                    for (size_t j = 0; j < batch_size; j++) {
                        func();
                        asm volatile("" : : : "memory");
                    }
                    auto end = C::now();
                    func_batch_stop(iteration, i, batch_size,
                                    std::forward<Fcb>(func_cbs)...);
                    auto delta = end - start;
                    samples[i] = delta.count();
                }

                func_iter_stop(iteration, run_size, batch_size,
                               std::forward<Fcb>(func_cbs)...);
            }

            template<class C, class R, class F, class... Fcb>
            static inline
            void loops(std::vector<R> & samples, const size_t iteration,
                       const size_t warmup, const size_t run_size, const size_t batch_size,
                       F&& func, Fcb&&... func_cbs)
            {
                // NOTE: not inlined to allow more optimizations on each phase
                loops_warmup<C>(samples, warmup, run_size, batch_size, std::forward<F>(func));
                loops_work<C>(samples, iteration, warmup, run_size, batch_size,
                              std::forward<F>(func), std::forward<Fcb>(func_cbs)...);
            }

            template<trun::message msg, class Clock, class... Fcb>
            static inline
            result<Clock> stats(const parameters<Clock> &params,
                                const size_t iteration,
                                std::vector<typename result<Clock>::duration::rep> &samples,
                                Fcb&&... func_cbs)
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

                rep_type sum = 0;
                rep_type sq_sum = 0;

                // calculate overall statistics
                for (size_t i = 0; i < params.run_size; i++) {
                    // normalize to per-experiment results
                    auto elem = samples[i];
		    elem = elem / params.batch_size;
		    samples[i] = elem;

                    // statistics of all experiments
                    res.min_all = std::min(duration_raw<Clock>(elem), res.min_all);
                    res.max_all = std::max(duration_raw<Clock>(elem), res.max_all);

                    // statistics of non-outliers
                    sum += elem;
                    sq_sum += std::pow(elem, 2);
                }

                rep_type s_mean = sum / res.run_size_all;
                rep_type s_variance = (sq_sum / res.run_size_all) - std::pow(s_mean, 2);
                rep_type s_sigma = std::sqrt(s_variance);
                rep_type outlier = params.confidence_outlier_sigma * s_sigma;

                // calculate statistics without outliers
                sum = 0;
                sq_sum = 0;
                for (size_t i = 0; i < params.run_size; i++) {
                    auto elem = samples[i];

                    trun::detail::message<trun::message::DEBUG_VERBOSE, msg>(
                        "value=%f outlier=%d", elem, std::abs(elem - s_mean) > outlier);

                    // ignore outliers
                    if (std::abs(elem - s_mean) > outlier) {
                        continue;
                    }

                    func_batch_select(iteration, i, params.batch_size,
                                      std::forward<Fcb>(func_cbs)...);

                    // statistics of non-outliers
                    res.min = std::min(duration_raw<Clock>(elem), res.min);
                    res.max = std::max(duration_raw<Clock>(elem), res.max);
                    sum += elem;
                    sq_sum += std::pow(elem, 2);
                    res.run_size++;
                }

                s_mean = sum / res.run_size;
                s_variance = (sq_sum / res.run_size) - std::pow(s_mean, 2);
                s_sigma = std::sqrt(s_variance);

                res.mean = duration_raw<Clock>(s_mean);
                res.sigma = duration_raw<Clock>(s_sigma);

                return std::move(res);
            }

        }
    }
}

// Run workload 'func()' until timing stabilizes.
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
template<bool calibrating, trun::message msg, class P, class F, class... Fcb>
static inline
void trun::detail::core::run(::trun::result<typename P::clock_type> & res, P & params,
                             F&& func, Fcb&&... func_cbs)
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
    result<C> res_best;
    res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());

    size_t experiments = 0;
    size_t iterations = 0;

    // check if population is statistically significant
    auto significant = [&](const result<C>& current) {
        return current.run_size >= params.run_size;
    };

    trun::detail::message<trun::message::DEBUG, msg>(
        "clock_overhead_perc=%f confidence_sigma=%f stddev_perc=%f "
        "warmup=%lu runs_size=%lu batch_size=%lu max_experiments=%lu",
        p.clock_overhead_perc, p.confidence_sigma, p.stddev_perc,
        p.warmup_batch_size, p.run_size, p.batch_size, p.max_experiments);

    while (true) {
        old_batch_size = p.batch_size;

        experiments += (p.run_size * p.batch_size);
        if (experiments >= p.max_experiments) {
            break;
        }

        // run experiment batches
        if (samples.size() < p.run_size) {
            samples.resize(p.run_size);
        } else if (samples.size() > p.run_size * 2) {
            samples.resize(p.run_size * 2);
        }
        loops<C>(samples, iterations, p.warmup_batch_size, p.run_size, p.batch_size,
                 std::forward<F>(func), std::forward<Fcb>(func_cbs)...);
        iterations++;

        // calculate statistics
        result<C> res_curr = stats<msg>(p, iterations-1, samples,
                                        std::forward<Fcb>(func_cbs)...);
        auto width = res_curr.mean.count() * stddev_ratio;

        trun::detail::message<trun::message::DEBUG, msg>(
            "mean=%f sigma=%f width=%f run_size=%lu run_size_all=%lu batch_size=%lu experiments=%lu",
            res_curr.mean.count(), res_curr.sigma.count(), width,
            res_curr.run_size, res_curr.run_size_all, res_curr.batch_size, experiments);

        // update 'clock_time' if we're in calibration mode
        update_clock_time<calibrating>(p, res_curr.mean);

        // check if we're done
        {
            bool match = res_curr.sigma.count() <= width;
            bool can_match = !((calibrating && iterations < 2) ||
                               (!calibrating && old_batch_size < 2));
            res_curr.converged = match && can_match;
            if (match && significant(res_curr)) {
                res_best = res_curr;
                func_iter_select(iterations-1, p.run_size, p.batch_size,
                                 std::forward<Fcb>(func_cbs)...);
                if (can_match) {
                    break;
                }
            } else {
                // keep track of best result in case we hit the run limit
                if (res_curr.sigma < res_best.sigma && significant(res_curr)) {
                    res_best = res_curr;
                    func_iter_select(iterations-1, p.run_size, p.batch_size,
                                     std::forward<Fcb>(func_cbs)...);
                }
            }
        }

        // discard populations without statistical significance
        if (!significant(res_curr)) {
            continue;
        }

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
            auto max_run_size = p.run_size * max_run_size_multiplier;
            auto top_run_size = p.run_size * max_run_size_multiplier * 10;
            auto min_run_size = p.run_size / max_run_size_multiplier;
            auto new_run_size = pow((2 * p.confidence_sigma * res_curr.sigma.count()) / width, 2);
            if (new_run_size > max_run_size && iterations == 1) {
                p.run_size = std::ceil(max_run_size);
            } else if (new_run_size < min_run_size && iterations > 1) {
                p.run_size = std::ceil(min_run_size);
            } else if (new_run_size > top_run_size){
                p.run_size = std::ceil(top_run_size);
            } else if (new_run_size > p.run_size){
                p.run_size = std::ceil(new_run_size);
            }
            if (p.run_size < params.run_size) {
                p.run_size = params.run_size;
            }
        }
    }

    res = res_best;
}

#endif // TRUN__DETAIL__CORE_IMPL_HPP
