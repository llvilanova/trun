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

#define __unused(arg) _unused_ ## arg __attribute__((unused))

#ifndef TRUN_DEBUG_TIME
#define TRUN_DEBUG_TIME std::pico
#endif

#if defined(TRUN_DEBUG)
#if (defined(NDEBUG) && !TRUN_DEBUG) || !TRUN_DEBUG
#define DEBUG(args...)
#else
#define DEBUG(args...) warnx("[trun] " args)
#endif
#else
#define DEBUG(args...)
#endif

#if defined(TRUN_INFO)
#if (defined(NDEBUG) && !TRUN_INFO) || !TRUN_INFO
#define INFO(args...)
#else
#define INFO(args...) warnx("[trun] " args)
#endif
#else
#define INFO(args...)
#endif

#endif // TRUN__DETAIL__COMMON_HPP
