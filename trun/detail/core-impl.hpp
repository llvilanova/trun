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


            template<class Clock, class Func, class... Args>
            typename std::enable_if<has_clock<Args...>::value,
                                    typename result<Clock>::duration>::type
            func_call(Func&& func, size_t batch_group, size_t warmup_batch_size, size_t batch_size)
            {
                return func(batch_group, warmup_batch_size, batch_size);
            }

            template<class Clock, class Func, class... Args>
            typename std::enable_if<not has_clock<Args...>::value,
                                    typename result<Clock>::duration>::type
            func_call(Func&& func, size_t batch_group, size_t warmup_batch_size, size_t batch_size)
            {
                for (size_t i = 0; i < warmup_batch_size; i++) {
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

            template<bool calibrating, class C>
            static inline
            typename std::enable_if<calibrating, void>::type
            update_clock_time(trun::parameters<C> &params, typename result<C>::duration::rep time)
            {
                params.clock_time = duration_raw<C>(time);
            }

            template<bool calibrating, class C>
            static inline
            typename std::enable_if<!calibrating, void>::type
            update_clock_time(trun::parameters<C> &params, typename result<C>::duration::rep _)
            {
            }

            template <trun::message msg, class Clock, class... Args>
            struct batch_group_stats
            {
                using rep_type = typename result<Clock>::duration::rep;
                size_t count, count_all;
                std::vector<rep_type> samples;
                std::vector<bool> outliers;
                rep_type mean, mean_all;
                rep_type sigma, sigma_all;
                rep_type min, min_all;
                rep_type max, max_all;

                void resize(size_t batch_group_size, bool keep_results)
                    {
                        count = count_all = 0;
                        if (samples.size() < batch_group_size) {
                            samples.resize(batch_group_size);
                            if (keep_results) {
                                outliers.resize(batch_group_size);
                            }
                        } else if (samples.size() > batch_group_size * 2) {
                            samples.resize(batch_group_size * 2);
                            if (keep_results) {
                                outliers.resize(batch_group_size * 2);
                            }
                        }
                    }

                void add(rep_type elem)
                    {
                        samples[count_all] = elem;
                        count_all++;
                    }


                void calculate(const trun::parameters<Clock> &params)
                    {
                        rep_type mean_cur = 0, mean_prev = 0, sigma_cur = 0, sigma_prev = 0;
                        rep_type min_cur = 0, max_cur = 0;

                        auto add_one = [&](size_t idx, rep_type elem) {
                            if (idx == 0) {
                                mean_cur = mean_prev = elem;
                                sigma_cur = sigma_prev = 0;
                                min_cur = max_cur = elem;
                            } else {
                                mean_cur = mean_prev + (elem - mean_prev) / (idx + 1);
                                sigma_cur = sigma_prev + (elem - mean_prev) * (elem - mean_cur);
                                mean_prev = mean_cur;
                                sigma_prev = sigma_cur;
                                min_cur = std::min(elem, min_cur);
                                max_cur = std::max(elem, max_cur);
                            }
                        };

                        for (size_t i = 0; i < count_all; i++) {
                            auto elem = samples[i];
                            elem = elem / params.batch_size;
                            samples[i] = elem;
                            add_one(i, elem);
                        }
                        mean_all = mean_cur;
                        if (sigma_cur != 0) {
                            auto variance_all = sigma_cur / (count_all - 1);
                            sigma_all = sqrt(variance_all);
                        }
                        min_all = min_cur;
                        max_all = max_cur;

                        rep_type outlier = params.confidence_outlier_sigma * sigma_all;
                        for (size_t i = 0; i < count_all; i++) {
                            auto elem = samples[i];
                            bool is_outlier = std::abs(elem - mean_all) > outlier;
                            if (detail::core::has_get_runs<Args...>::value) {
                                outliers[i] = is_outlier;
                            }
                            trun::detail::message<trun::message::DEBUG_VERBOSE, msg>(
                                "value=%f outlier=%d", elem, is_outlier);

                            // ignore outliers
                            if (is_outlier) {
                                continue;
                            }

                            add_one(count, elem);
                            count++;
                        }
                        mean = mean_cur;
                        if (sigma_cur != 0) {
                            auto variance = sigma_cur / (count - 1);
                            sigma = sqrt(variance);
                        }
                        min = min_cur;
                        max = max_cur;
                    }
            };

        }
    }
}

// Run workload 'func()' until timing stabilizes.
//
// NOTE: stddev == 2*sigma
template<bool calibrating, trun::message msg, class C, class F, class... Args>
static inline
trun::result<C> trun::detail::core::run(trun::parameters<C> params, F&& func, Args&&... args)
{
    // cap size increases (if not 'calibrating')
    double max_batch_group_size_multiplier = 3;
    double max_batch_size_multiplier = 3;

    // convert to ratios
    params.clock_overhead_perc /= 100;
    params.stddev_perc /= 100;

    batch_group_stats<msg, C, Args...> stats_res, stats;

    trun::detail::message<trun::message::DEBUG, msg>(
        "clock_overhead=%f confidence_sigma=%f confidence_outlier_sigma=%f stddev=%f "
        "warmup_batch_group_size=%lu batch_group_size=%lu warmup_batch_size=%lu batch_size=%lu experiment_timeout=%f",
        params.clock_overhead_perc, params.confidence_sigma, params.confidence_outlier_sigma, params.stddev_perc,
        params.warmup_batch_group_size, params.batch_group_size, params.warmup_batch_size, params.batch_size,
        params.experiment_timeout);

    auto timeout_start = std::chrono::steady_clock::now();

    size_t target_batch_group_size = params.batch_group_size;
    size_t target_batch_size = params.batch_size;
    unsigned int iterations = 0;
    bool converged = false;
    while (true) {
        // run experiment batches
        stats.resize(params.batch_group_size,
                     detail::core::has_get_runs<Args...>::value);
        // - pre-batch group warmup
        (void)detail::core::func_call<C, F, Args...>(
            std::forward<F>(func), 0, params.warmup_batch_group_size, 0);
        // - batch group
        for (size_t i = 0; i < params.batch_group_size; i++) {
            auto delta = detail::core::func_call<C, F, Args...>(
                std::forward<F>(func), i, params.warmup_batch_size, params.batch_size);
            stats.add(delta.count());
        }

        // calculate statistics
        stats.calculate(params);
        auto width = stats.mean * params.stddev_perc;

        auto timeout_now = std::chrono::steady_clock::now();
        auto timeout_cur_delta = timeout_now - timeout_start;
        if (msg >= trun::message::DEBUG) {
            auto t_left = params.experiment_timeout - (unsigned long)std::chrono::duration_cast<std::chrono::seconds>(
                timeout_cur_delta).count();
            trun::detail::message<trun::message::DEBUG, msg>(
                "mean=%f sigma=%f width=%f"
                " batch_group_size=%lu batch_group_size_all=%lu batch_size=%lu"
                " experiment_timeout_left=%ld",
                stats.mean, stats.sigma, width,
                stats.count, stats.count_all, params.batch_size, (long)t_left);
        }

        // update 'clock_time' if we're in calibration mode
        update_clock_time<calibrating>(params, stats.mean);

        // done in one try (forced)
        if (params.stddev_perc == 0) {
            stats_res = stats;
            break;
        }

        // check if we're done
        {
            bool match = stats.sigma <= width;
            bool can_match = true;
            // keep running if we were capped by target size growth
            can_match &= params.batch_group_size == target_batch_group_size;
            can_match &= params.batch_size == target_batch_size;
            // do at least two iterations
            can_match &= iterations > 1;
            converged = match && can_match;
            if (converged) {
                stats_res = stats;
                break;
            } else {
                // keep track of result with lowest sigma in case we timeout
                if (iterations == 0 || stats.sigma < stats_res.sigma) {
                    stats_res = stats;
                }
            }
        }

        // check for timeout
        auto timeout_cur_delta_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            timeout_cur_delta).count();
        if (timeout_cur_delta_seconds >= params.experiment_timeout) {
            trun::detail::message<trun::message::INFO, msg>(
                "experiment timed out after %lu sec", timeout_cur_delta_seconds);
            stats_res = stats;
            break;
        }

        // auto-adapt @batch_group_size
        if (params.confidence_sigma) {
            auto max_size = params.batch_group_size * max_batch_group_size_multiplier;
            auto top_size = params.batch_group_size * max_batch_group_size_multiplier * 10;
            auto min_size = params.batch_group_size / max_batch_group_size_multiplier / 10;
            auto new_size = pow((2 * params.confidence_sigma * stats.sigma_all) / width, 2);
            // compensate batch group size for outliers
            new_size += stats.count_all - stats.count;
            target_batch_group_size = std::ceil(new_size);
            if (new_size > max_size && iterations == 0) {
                params.batch_group_size = std::ceil(max_size);
            } else if (new_size < min_size && iterations > 1) {
                params.batch_group_size = std::ceil(min_size);
            } else if (new_size > top_size){
                params.batch_group_size = std::ceil(top_size);
            } else {
                params.batch_group_size = std::ceil(new_size);
            }
            if (params.batch_group_size < params.batch_group_min_size) {
                target_batch_group_size = params.batch_group_size = params.batch_group_min_size;
            }
        }

        // auto-adapt @batch_size
        if (params.clock_overhead_perc){
            // conservative mean for calculating batch
            auto mean_all = stats.mean_all - stats.sigma_all;
            if (mean_all < 0) {
                mean_all = stats.mean_all;
            }
            auto max_size = params.batch_size * max_batch_size_multiplier;
            auto top_size = params.batch_size * max_batch_size_multiplier * 10;
            auto min_size = params.batch_size / max_batch_size_multiplier;
            auto new_size = params.clock_time.count() / (mean_all * params.clock_overhead_perc);
            target_batch_size = std::ceil(new_size);
            if (new_size > max_size && iterations == 0) {
                params.batch_size = std::ceil(max_size);
            } else if (new_size < min_size && iterations > 1) {
                params.batch_size = std::ceil(min_size);
            } else if (new_size > top_size){
                params.batch_size = std::ceil(top_size);
            } else {
                params.batch_size = std::ceil(new_size);
            }
        }

        iterations++;
    }

    trun::result<C> res;
    res.min = duration_raw<C>(stats_res.min);
    res.min_all = duration_raw<C>(stats_res.min_all);
    res.max = duration_raw<C>(stats_res.max);
    res.max_all = duration_raw<C>(stats_res.max_all);
    res.mean = duration_raw<C>(stats_res.mean);
    res.mean_all = duration_raw<C>(stats_res.mean_all);
    res.sigma = duration_raw<C>(stats_res.sigma);
    res.sigma_all = duration_raw<C>(stats_res.sigma_all);
    res.batches = stats_res.count;
    res.batches_all = stats_res.count_all;
    res.converged = converged;
    if (detail::core::has_get_runs<Args...>::value) {
        res.runs.resize(stats.count_all);
        for (size_t i = 0; i < stats.count_all; i++) {
            res.runs[i].time = duration_raw<C>(stats.samples[i]);
            res.runs[i].outlier = stats.outliers[i];
        }
    }

    return res;
}

#endif // TRUN__DETAIL__CORE_IMPL_HPP
