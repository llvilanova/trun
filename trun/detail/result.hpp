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
    for (auto e: r.runs) {
        std::get<0>(e) /= factor;
    }
    return r;
}


template<class Clock>
template <class ClockTarget>
inline
trun::result<ClockTarget>
trun::result<Clock>::convert() const
{
    trun::result<ClockTarget> res;
    res.runs.resize(this->runs.size());
    if (std::is_same<Clock, trun::time::tsc_cycles>::value &&
        !std::is_same<ClockTarget, trun::time::tsc_cycles>::value) {
        res.min = trun::time::tsc_cycles::time(this->min);
        res.min_all = trun::time::tsc_cycles::time(this->min_all);
        res.max = trun::time::tsc_cycles::time(this->max);
        res.max_all = trun::time::tsc_cycles::time(this->max_all);
        res.mean = trun::time::tsc_cycles::time(this->mean);
        res.sigma = trun::time::tsc_cycles::time(this->sigma);
        for (auto i=0; i<res.runs.size(); i++) {
            auto elem = this->runs[i];
            res.runs[i] = std::make_pair(trun::time::tsc_cycles::time(std::get<0>(elem)),
                                         std::get<1>(elem));
        }
    } else if (!std::is_same<Clock, trun::time::tsc_cycles>::value &&
               std::is_same<ClockTarget, trun::time::tsc_cycles>::value) {
        errx(1, "[trun] not implemented");
    } else {
        res.min = this->min;
        res.min_all = this->min_all;
        res.max = this->max;
        res.max_all = this->max_all;
        res.mean = this->mean;
        res.sigma = this->sigma;
        for (auto i=0; i<res.runs.size(); i++) {
            res.runs[i] = this->runs[i];
        }
    }
    res.run_size = this->run_size;
    res.run_size_all = this->run_size_all;
    res.batch_size = this->batch_size;
    res.converged = this->converged;
    return res;
}

#endif // TRUN__DETAIL__RESULT_HPP
