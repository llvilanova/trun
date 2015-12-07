/** trun/detail/parameters.hpp ---
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

#ifndef TRUN__DETAIL__PARAMETERS_HPP
#define TRUN__DETAIL__PARAMETERS_HPP 1

#include <trun.hpp>

namespace trun {
    namespace detail {
        namespace parameters {

            template<class P>
            static inline
            void check(const P & params)
            {
#define _TRUN_PARAM_CHECK(p, eq)                                        \
                do {                                                    \
                    if (params.p < 0) {                                 \
                        errx(1, "[trun] cannot have negative '" #p "'"); \
                    }                                                   \
                    if (eq && params.p == 0) {                          \
                        errx(1, "[trun] cannot have null '" #p "'");    \
                    }                                                   \
                } while (0)

                _TRUN_PARAM_CHECK(clock_time.count(), false);
                _TRUN_PARAM_CHECK(clock_overhead_perc, false);
                _TRUN_PARAM_CHECK(confidence_sigma, true);
                _TRUN_PARAM_CHECK(confidence_outlier_sigma, true);
                _TRUN_PARAM_CHECK(stddev_perc, true);
                _TRUN_PARAM_CHECK(run_size, true);
                _TRUN_PARAM_CHECK(batch_size, true);
#undef _TRUN_PARAM_CHECK

                if (params.run_size < TRUN_RUN_SIZE) {
                    warnx("[trun] run_size is not statistically significant (< %d)", TRUN_RUN_SIZE);
                }
            }

        }
    }
}

template<class C>
inline
trun::parameters<C>::parameters()
    :clock_time(0)
    ,clock_overhead_perc(TRUN_CLOCK_OVERHEAD_PERC)
    ,confidence_sigma(TRUN_CONFIDENCE_SIGMA)
    ,confidence_outlier_sigma(TRUN_CONFIDENCE_OUTLIER_SIGMA)
    ,stddev_perc(TRUN_STDDEV_PERC)
    ,warmup_batch_size(TRUN_WARMUP_BATCH_SIZE)
    ,run_size(TRUN_RUN_SIZE)
    ,batch_size(TRUN_BATCH_SIZE)
    ,max_experiments(TRUN_MAX_EXPERIMENTS)
{
}

template<class Clock>
template <class ClockTarget>
inline
trun::parameters<ClockTarget>
trun::parameters<Clock>::convert() const
{
    using tsc_cycles = ::trun::time::tsc_cycles;
    trun::parameters<ClockTarget> res;
    if (std::is_same<Clock, tsc_cycles>::value &&
        !std::is_same<ClockTarget, tsc_cycles>::value) {
        res.clock_time = tsc_cycles::time(this->clock_time);
    } else if (!std::is_same<Clock, tsc_cycles>::value &&
               std::is_same<ClockTarget, tsc_cycles>::value) {
        auto d = std::chrono::duration_cast<
            std::chrono::duration<typename Clock::rep, std::ratio<1>>
            >(this->clock_time);
        d *= tsc_cycles::frequency();
        res.clock_time = typename ClockTarget::duration(
            typename ClockTarget::rep(d.count()));
    } else {
        res.clock_time = this->clock_time;
    }

    res.clock_overhead_perc = this->clock_overhead_perc;
    res.confidence_sigma = this->confidence_sigma;
    res.confidence_outlier_sigma = this->confidence_outlier_sigma;
    res.stddev_perc = this->stddev_perc;
    res.warmup_batch_size = this->warmup_batch_size;
    res.run_size = this->run_size;
    res.batch_size = this->batch_size;
    res.max_experiments = this->max_experiments;
    return res;
}

#endif // TRUN__DETAIL__PARAMETERS_HPP
