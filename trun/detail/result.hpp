/** trun/detail/result.hpp ---
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

#ifndef TRUN__DETAIL__RESULT_HPP
#define TRUN__DETAIL__RESULT_HPP 1

#include <err.h>


template<class Clock>
inline
trun::result<Clock>
trun::result<Clock>::scale(unsigned long long factor) const
{
    auto r = *this;
    r.min /= factor;
    r.min_all /= factor;
    r.max /= factor;
    r.max_all /= factor;
    r.mean /= factor;
    r.sigma /= factor;
    return r;
}


template<class Clock>
static inline
constexpr bool
is_cycles()
{
    return std::is_same<Clock, trun::time::tsc_cycles>::value
        || std::is_same<Clock, trun::time::tsc_barrier_cycles>::value;
}

template<class Clock>
template <class ClockTarget>
inline
trun::result<ClockTarget>
trun::result<Clock>::convert() const
{
    trun::result<ClockTarget> res;
    if (is_cycles<Clock>() && !is_cycles<ClockTarget>()) {
        res.min = Clock::time(this->min);
        res.min_all = Clock::time(this->min_all);
        res.max = Clock::time(this->max);
        res.max_all = Clock::time(this->max_all);
        res.mean = Clock::time(this->mean);
        res.sigma = Clock::time(this->sigma);
    } else if (!is_cycles<Clock>() && is_cycles<ClockTarget>()) {
        errx(1, "[trun] not implemented");
    } else {
        res.min = this->min;
        res.min_all = this->min_all;
        res.max = this->max;
        res.max_all = this->max_all;
        res.mean = this->mean;
        res.sigma = this->sigma;
    }
    res.run_size = this->run_size;
    res.run_size_all = this->run_size_all;
    res.batch_size = this->batch_size;
    res.converged = this->converged;
    return res;
}

#endif // TRUN__DETAIL__RESULT_HPP
