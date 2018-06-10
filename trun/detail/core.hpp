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

#ifndef TRUN__DETAIL__CORE_HPP
#define TRUN__DETAIL__CORE_HPP 1

namespace trun {
    namespace detail {
        namespace core {

            template<bool calibrating, trun::message msg, class C,
                     class F, class... Args>
            static inline
            result<C> run(trun::parameters<C> params, F&& func, Args&&... args);

        }
    }
}

#include <trun/detail/core-impl.hpp>

#endif // TRUN__DETAIL__CORE_HPP
