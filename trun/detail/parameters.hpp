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
            void check(const P & params)
            {
#define _TRUN_PARAM_CHECK(p, eq)                                        \
                do {                                                    \
                    if (params.p < 0) {                                 \
                        errx(1, "cannot have negative '" #p "'");       \
                    }                                                   \
                    if (eq && params.p == 0) {                          \
                        errx(1, "cannot have null '" #p "'");           \
                    }                                                   \
                } while (0)

                _TRUN_PARAM_CHECK(clock_time.count(), false);
                _TRUN_PARAM_CHECK(clock_overhead_perc, false);
                _TRUN_PARAM_CHECK(mean_err_perc, false);
                _TRUN_PARAM_CHECK(init_runs, true);
                _TRUN_PARAM_CHECK(init_batch, true);
#undef _TRUN_PARAM_CHECK
            }

        }
    }
}

template<class C>
trun::parameters<C>::parameters()
    :clock_time(0)
    ,clock_overhead_perc(TRUN_CLOCK_OVERHEAD_PERC)
    ,mean_err_perc(0)
    ,sigma_outlier_perc(0)
    ,warmup_batch_size(TRUN_WARMUP_BATCH_SIZE)
    ,init_runs(0)
    ,init_batch(0)
    ,max_experiments(0)
{
}

#endif // TRUN__DETAIL__PARAMETERS_HPP
