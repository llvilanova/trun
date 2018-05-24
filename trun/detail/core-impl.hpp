/** trun/detail/core.hpp ---
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

#ifndef TRUN__DETAIL__CORE_IMPL_HPP
#define TRUN__DETAIL__CORE_IMPL_HPP 1

#include <cmath>
#include <err.h>
#include <limits>

#include <trun/detail/core.hpp>


namespace trun {
    namespace detail {
        namespace core {

            template<class Target, class... Args>
            struct has_mod;

            template<class Target>
            struct has_mod<Target> {
                static const bool value = false;
            };

            template<class Target, class Arg, class... Args>
            struct has_mod<Target, Arg, Args...> {
                static const bool value = std::is_same<
                    Target, typename std::remove_reference<Arg>::type>::value or
                    has_mod<Target, Args...>::value;
            };

            template<class... Args>
            struct has_get_runs : public has_mod<trun::mod_get_runs_type, Args...> {};

            template<class... Args>
            struct has_clock : public has_mod<trun::mod_clock_type, Args...> {};


            template<class Target, class Arg>
            typename std::enable_if<std::is_same<Target, Arg>::value,
                                    void>::type
            hook_call2(size_t s1, size_t s2, size_t s3, void(*arg)(Arg, size_t, size_t, size_t))
            {
                Target target;
                arg(target, s1, s2, s3);
            }

            template<class Target, class Arg>
            typename std::enable_if<not std::is_same<Target, Arg>::value,
                                    void>::type
            hook_call2(size_t s1, size_t s2, size_t s3, void(*arg)(Arg, size_t, size_t, size_t))
            {
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_iter_start, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_iter_start>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_iter_stop, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_iter_stop>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_iter_select, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_iter_select>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_run_start, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_run_start>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_run_stop, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_run_stop>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, void(*arg)(hook_run_select, size_t, size_t, size_t))
            {
                hook_call2<Target, hook_run_select>(s1, s2, s3, arg);
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, mod_get_runs_type _)
            {
                (void)_;
            }

            template<class Target>
            void
            hook_call1(size_t s1, size_t s2, size_t s3, mod_clock_type _)
            {
                (void)_;
            }

            template<class Target>
            void
            hook_call(size_t s1, size_t s2, size_t s3)
            {
            }

            template<class Target, class Arg, class... Args>
            void
            hook_call(size_t s1, size_t s2, size_t s3, Arg&& arg, Args&&... args)
            {
                hook_call1<Target>(s1, s2, s3, arg);
                hook_call<Target, Args...>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_iter_start(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_iter_start>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_run_start(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_run_start>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_run_stop(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_run_stop>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_iter_stop(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_iter_stop>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_run_select(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_run_select>(s1, s2, s3, std::forward<Args>(args)...);
            }

            template<class... Args>
            void func_iter_select(const size_t& s1, const size_t& s2, const size_t& s3, Args&&... args)
            {
                hook_call<trun::hook_iter_select>(s1, s2, s3, std::forward<Args>(args)...);
            }


            template<class Clock, class Func, class... Args>
            typename std::enable_if<has_clock<Args...>::value,
                                    typename result<Clock>::duration>::type
            func_call(Func&& func, size_t iteration, size_t run, size_t warmup, size_t batch_size)
            {
                return func(iteration, run, warmup, batch_size);
            }

            template<class Clock, class Func, class... Args>
            typename std::enable_if<not has_clock<Args...>::value,
                                    typename result<Clock>::duration>::type
            func_call(Func&& func, size_t iteration, size_t run, size_t warmup, size_t batch_size)
            {
                for (size_t i = 0; i < warmup; i++) {
                    func();
                    asm volatile("" : : : "memory");
                }
                auto start = Clock::now();
                for (size_t i = 0; i < batch_size; i++) {
                    func();
                    asm volatile("" : : : "memory");
                }
                auto end = Clock::now();
                return end - start;
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

            static inline
            typename result<trun::time::tsc_clock>::duration
            to_time(const typename result<trun::time::tsc_cycles>::duration &dur)
            {
                return trun::time::tsc_cycles::time(dur);
            }

            template<class Clock>
            static inline
            typename result<Clock>::duration
            to_time(const typename result<Clock>::duration &dur)
            {
                return dur;
            }

            template<class C, class R, class F, class... Args>
            static inline
            void loops(std::vector<R> & samples, const size_t iteration,
                       const size_t iteration_warmup, const size_t run_warmup,
                       const size_t run_size, const size_t batch_size,
                       F&& func, Args&&... args)
            {
                // NOTE: not inlined to allow more optimizations on each phase
                (void)detail::core::func_call<C, F, Args...>(
                    std::forward<F>(func), 0, 0, iteration_warmup, 0);

                func_iter_start(iteration, run_size, batch_size,
                                std::forward<Args>(args)...);

                for (size_t run = 0; run < run_size; run++) {
                    func_run_start(iteration, run, batch_size,
                                   std::forward<Args>(args)...);
                    auto delta = detail::core::func_call<C, F, Args...>(
                        std::forward<F>(func), iteration, run, run_warmup, batch_size);
                    func_run_stop(iteration, run, batch_size,
                                  std::forward<Args>(args)...);
                    samples[run] = delta.count();
                }

                func_iter_stop(iteration, run_size, batch_size,
                               std::forward<Args>(args)...);
            }

            template<trun::message msg, class Clock, class... Args>
            static inline
            result<Clock> stats(const parameters<Clock> &params,
                                const size_t iteration,
                                std::vector<typename result<Clock>::duration::rep> &samples,
                                std::vector<bool> &outliers,
                                Args&&... args)
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

                // Calculate statistics using Welford's method
                // (also appears in TAOCP, vol. 2)
                rep_type m, m_prev=0, s=0, s_prev=0;
                auto add_elem = [&](const size_t i, const rep_type elem) {
                    if (i==0) {
                        m_prev = m = elem;
                        s_prev = 0;
                    } else {
                        m = m_prev + (elem - m_prev)/(i+1);
                        s = s_prev + (elem - m_prev) * (elem - m);
                        m_prev = m;
                        s_prev = s;
                    }
                };
                auto get_mean = [&](size_t size) {
                    if (size > 1) {
                        return m;
                    } else {
                        return 0.0;
                    }
                };
                auto get_sigma = [&](size_t size) {
                    if (size > 0) {
                        auto variance = s / (size - 1);
                        return sqrt(variance);
                    } else {
                        return 0.0;
                    }
                };

                // statistics for all samples
                for (size_t i = 0; i < params.run_size; i++) {
                    // normalize to per-experiment results
                    auto elem = samples[i];
		    elem = elem / params.batch_size;
		    samples[i] = elem;

                    res.min_all = std::min(duration_raw<Clock>(elem), res.min_all);
                    res.max_all = std::max(duration_raw<Clock>(elem), res.max_all);
                    add_elem(i, elem);
                }

                rep_type mean = get_mean(res.run_size_all);
                rep_type sigma = get_sigma(res.run_size_all);
                res.mean_all = duration_raw<Clock>(mean);
                res.sigma_all = duration_raw<Clock>(sigma);
                rep_type outlier = params.confidence_outlier_sigma * sigma;

                // statistics for non-outliers
                for (size_t i = 0; i < params.run_size; i++) {
                    auto elem = samples[i];
                    bool is_outlier = std::abs(elem - mean) > outlier;
                    if (detail::core::has_get_runs<Args...>::value) {
                        outliers[i] = is_outlier;
                    }
                    trun::detail::message<trun::message::DEBUG_VERBOSE, msg>(
                        "value=%f outlier=%d", elem, is_outlier);

                    // ignore outliers
                    if (is_outlier) {
                        continue;
                    }

                    func_run_select(iteration, i,
                                    // is_outlier,
                                    params.batch_size,
                                    std::forward<Args>(args)...);

                    res.min = std::min(duration_raw<Clock>(elem), res.min);
                    res.max = std::max(duration_raw<Clock>(elem), res.max);
                    add_elem(res.run_size, elem);
                    res.run_size++;
                }

                mean = get_mean(res.run_size);
                sigma = get_sigma(res.run_size);
                res.mean = duration_raw<Clock>(mean);
                res.sigma = duration_raw<Clock>(sigma);

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
template<bool calibrating, trun::message msg, class P, class F, class... Args>
static inline
void trun::detail::core::run(::trun::result<typename P::clock_type> & res, P & params,
                             F&& func, Args&&... args)
{
    using C = typename P::clock_type;
    using rep_type = typename result<C>::duration::rep;

    double max_run_size_multiplier = 3;
    double max_batch_size_multiplier = 3;
    parameters<C> p = params;
    double clock_overhead = p.clock_overhead_perc / 100;
    double stddev_ratio = p.stddev_perc / 100;

    std::vector<rep_type> samples(p.run_size);
    std::vector<bool> outliers(p.run_size);
    result<C> res_best;
    res_best.mean = duration_raw<C>(0);
    res_best.sigma = duration_raw<C>(std::numeric_limits<rep_type>::max());
    res_best.converged = false;

    size_t iterations = 0;
    // first iteration is never enough
    double target_batch_size = std::numeric_limits<double>::max();

    // check if population is statistically significant
    auto significant = [&](const result<C>& current) {
        return current.run_size >= params.run_size_min_significance;
    };

    auto topple_runs = [](result<C>& dest,
                          const std::vector<double>& src_samples,
                          const std::vector<bool>& src_outliers) {
        if (detail::core::has_get_runs<Args...>::value) {
            dest.runs.resize(dest.run_size_all);
            for (size_t i = 0; i < dest.run_size_all; i++) {
                dest.runs[i] = std::make_tuple(duration_raw<C>(src_samples[i]), src_outliers[i]);
            }
        }
    };

    trun::detail::message<trun::message::DEBUG, msg>(
        "clock_overhead_perc=%f confidence_sigma=%f stddev_perc=%f "
        "i_warmup=%lu r_warmup=%lu runs_size=%lu batch_size=%lu experiment_timeout=%f",
        p.clock_overhead_perc, p.confidence_sigma, p.stddev_perc,
        p.iteration_warmup_batch_size, p.run_warmup_batch_size,
        p.run_size, p.batch_size, p.experiment_timeout);

    std::chrono::steady_clock::rep experiment_timeout_delta =
        (p.experiment_timeout * std::chrono::steady_clock::period::den)
        / std::chrono::steady_clock::period::num;
    auto timeout_start = std::chrono::steady_clock::now();

    while (true) {
        // try to calculate how much more it will take
        auto timeout_now = std::chrono::steady_clock::now();
        auto timeout_cur_delta = timeout_now - timeout_start;
        auto experiment_delta = detail::core::to_time(res_best.mean) *
            ((p.run_size * (p.run_warmup_batch_size + p.batch_size)) + p.iteration_warmup_batch_size);
        if ((timeout_cur_delta + experiment_delta).count() >= experiment_timeout_delta) {
            trun::detail::message<trun::message::INFO, msg>(
                "experiment timed out after %lu(+%lu) sec",
                std::chrono::duration_cast<std::chrono::seconds>(timeout_cur_delta).count(),
                std::chrono::duration_cast<std::chrono::seconds>(experiment_delta).count());
            break;
        }

        // run experiment batches
        if (samples.size() < p.run_size) {
            samples.resize(p.run_size);
            if (detail::core::has_get_runs<Args...>::value) {
                outliers.resize(p.run_size);
            }
        } else if (samples.size() > p.run_size * 2) {
            samples.resize(p.run_size * 2);
            if (detail::core::has_get_runs<Args...>::value) {
                outliers.resize(p.run_size * 2);
            }
        }
        loops<C>(samples, iterations, p.iteration_warmup_batch_size, p.run_warmup_batch_size,
                 p.run_size, p.batch_size,
                 std::forward<F>(func), std::forward<Args>(args)...);
        iterations++;

        // calculate statistics
        result<C> res_curr = stats<msg>(
            p, iterations-1, samples, outliers,
            std::forward<Args>(args)...);
        auto width = res_curr.mean.count() * stddev_ratio;
        auto width_all = res_curr.mean_all.count() * stddev_ratio;

        if (msg >= trun::message::DEBUG) {
            timeout_now = std::chrono::steady_clock::now();
            timeout_cur_delta = timeout_now - timeout_start;
            trun::detail::message<trun::message::DEBUG, msg>(
                "mean=%f sigma=%f width=%f run_size=%lu run_size_all=%lu batch_size=%lu"
                " experiment_delta=%lu",
                res_curr.mean.count(), res_curr.sigma.count(), width,
                res_curr.run_size, res_curr.run_size_all, res_curr.batch_size,
                std::chrono::duration_cast<std::chrono::seconds>(timeout_cur_delta).count());
        }

        // update 'clock_time' if we're in calibration mode
        update_clock_time<calibrating>(p, res_curr.mean);

        // check if we're done
        {
            bool match = res_curr.sigma.count() <= width;
            // keep running if we were capped by batch size growth
            bool can_match = p.batch_size >= target_batch_size;
            res_curr.converged = match && can_match;
            if (match && significant(res_curr)) {
                res_best = res_curr;
                topple_runs(res_best, samples, outliers);
                func_iter_select(iterations-1, p.run_size, p.batch_size,
                                 std::forward<Args>(args)...);
                if (can_match) {
                    break;
                }
            } else {
                // keep track of best result in case we hit the run limit
                if (res_curr.sigma < res_best.sigma &&
                    (significant(res_curr) ||
                     // select at least one
                     iterations <= 1)) {
                    res_best = res_curr;
                    topple_runs(res_best, samples, outliers);
                    func_iter_select(iterations-1, p.run_size, p.batch_size,
                                     std::forward<Args>(args)...);
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
            auto mean_all = res_curr.mean_all.count() - res_curr.sigma_all.count();
            if (mean_all < 0) {
                mean_all = res_curr.mean_all.count();
            }
            auto max_batch_size = p.batch_size * max_batch_size_multiplier;
            auto min_batch_size = p.batch_size / max_batch_size_multiplier;
            target_batch_size = p.clock_time.count() / (mean_all * clock_overhead);
            if (target_batch_size > max_batch_size && iterations == 1) {
                p.batch_size = std::ceil(max_batch_size);
            } else if (target_batch_size < min_batch_size && iterations > 1) {
                p.batch_size = std::ceil(min_batch_size);
            } else if (target_batch_size > p.batch_size){
                p.batch_size = std::ceil(target_batch_size);
            }
        }

        // calculate new population size
        {
            auto max_run_size = p.run_size * max_run_size_multiplier;
            auto top_run_size = p.run_size * max_run_size_multiplier * 10;
            auto min_run_size = p.run_size / max_run_size_multiplier;
            auto new_run_size = pow((2 * p.confidence_sigma * res_curr.sigma_all.count()) / width_all, 2);
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
