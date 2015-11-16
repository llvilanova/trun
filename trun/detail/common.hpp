/** trun/detail/common.hpp ---
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

#ifndef TRUN__DETAIL__COMMON_HPP
#define TRUN__DETAIL__COMMON_HPP 1

#include <err.h>


#define __unused(arg) _unused_ ## arg __attribute__((unused))

namespace trun {
    namespace detail {

        template<bool show, class... Args>
        static inline
        void info(const std::string & msg, Args&&...args)
        {
            if (show) {
                std::string s = "[trun] " + msg;
                warnx(s.c_str(), std::forward<Args>(args)...);
            }
        }

        template<bool show, class... Args>
        static inline
        void debug(const std::string & msg, Args&&...args)
        {
            if (show) {
                std::string s = "[trun] " + msg;
                warnx(s.c_str(), std::forward<Args>(args)...);
            }
        }

    }
}

#endif // TRUN__DETAIL__COMMON_HPP
