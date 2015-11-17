/** trun/detail/time-cycles.hpp ---
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

#ifndef TRUN__DETAIL__TIME_CYCLES_HPP
#define TRUN__DETAIL__TIME_CYCLES_HPP 1

#include <trun/detail/core.hpp>
#include <unistd.h>


//////////////////////////////////////////////////
// tsc_cycles

inline
void
trun::time::tsc_cycles::check()
{
#if (defined(__GNUC__) || defined(__ICC) || defined(__SUNPRO_C)) && defined(__x86_64__)
    // ensure the counter is invariant (constant frequency)
    unsigned int op, eax, ebx, ecx, edx;
    op = 0x80000007;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a" (op));
    auto tsc_invariant = edx & (1<<8);
    if (!tsc_invariant) {
        errx(1, "[trun] trun::time::tsc_cycles does not have a constant frequency");
    }
#else
#if !defined(NWARN_TSC_BARRIER_CYCLES)
#warning trun::time::tsc_cycles not supported (define NWARN_TSC_BARRIER_CYCLES to disable)
#endif
    errx(1, "[trun] trun::time::tsc_cycles not supported");
#endif
}

inline
trun::time::tsc_cycles::time_point
trun::time::tsc_cycles::now()
{
#if (defined(__GNUC__) || defined(__ICC) || defined(__SUNPRO_C)) && defined(__x86_64__)
    unsigned high, low;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    unsigned long long res = ((unsigned long long)low) | (((unsigned long long)high) << 32);
    return tsc_cycles::time_point(tsc_cycles::duration(res));
#else
    errx(1, "[trun] trun::time::tsc_cycles not supported in this system");
#endif
}

inline
unsigned long long
trun::time::tsc_cycles::frequency()
{
    static unsigned long long f = 0;
    if (f == 0) {
        auto t1 = now();
        sleep(2);
        auto t2 = now();
        f = (t2 - t1).count() / 2;
    }
    return f;
}

inline
std::chrono::duration<double, std::pico>
trun::time::tsc_cycles::cycle_time()
{
    auto f = frequency();
    return std::chrono::duration<double, std::ratio<1>>(1.0/f);
}

template<class Rep, class Period>
inline
std::chrono::duration<double, std::nano>
trun::time::tsc_cycles::time(const std::chrono::duration<Rep, Period> &d)
{
    auto unit = std::chrono::duration< double, std::ratio<1> >(d);
    auto t = unit / trun::time::tsc_cycles::frequency();
    return std::chrono::duration<double, std::nano>(t);
}

namespace trun {
    namespace time {

        namespace detail {

            void check(const ::trun::time::tsc_cycles &clock)
            {
                clock.check();
            }

        }

        template<class Ratio>
        static inline
        std::string
        units(const ::trun::time::tsc_cycles &clock)
        {
            std::string units = "?";
            if (std::ratio_equal<std::femto, Ratio>::value) {
                units = "f";
            } else if (std::ratio_equal<std::pico, Ratio>::value) {
                units = "p";
            } else if (std::ratio_equal<std::nano, Ratio>::value) {
                units = "n";
            } else if (std::ratio_equal<std::micro, Ratio>::value) {
                units = "u";
            } else if (std::ratio_equal<std::milli, Ratio>::value) {
                units = "m";
            } else if (std::ratio_equal<std::ratio<1>, Ratio>::value) {
                units = "";
            } else if (std::ratio_equal<std::kilo, Ratio>::value) {
                units = "K";
            } else if (std::ratio_equal<std::mega, Ratio>::value) {
                units = "M";
            } else if (std::ratio_equal<std::giga, Ratio>::value) {
                units = "G";
            } else if (std::ratio_equal<std::tera, Ratio>::value) {
                units = "T";
            } else if (std::ratio_equal<std::peta, Ratio>::value) {
                units = "P";
            }
            if (units != "?") {
                units += "cycles";
            }
            return units;
        }

    }
}


//////////////////////////////////////////////////
// tsc_barrier_cycles

inline
void
trun::time::tsc_barrier_cycles::check()
{
#if (defined(__GNUC__) || defined(__ICC) || defined(__SUNPRO_C)) && defined(__x86_64__)
    // ensure the counter is invariant (constant frequency)
    unsigned int op, eax, ebx, ecx, edx;
    op = 0x80000007;
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a" (op));
    auto tsc_invariant = edx & (1<<8);
    if (!tsc_invariant) {
        errx(1, "[trun] trun::time::tsc_barrier_cycles does not have a constant frequency");
    }
#else
#if !defined(NWARN_TSC_BARRIER_CYCLES)
#warning trun::time::tsc_barrier_cycles not supported (define NWARN_TSC_BARRIER_CYCLES to disable)
#endif
    errx(1, "[trun] trun::time::tsc_barrier_cycles not supported");
#endif
}

inline
trun::time::tsc_barrier_cycles::time_point
trun::time::tsc_barrier_cycles::now()
{
#if (defined(__GNUC__) || defined(__ICC) || defined(__SUNPRO_C)) && defined(__x86_64__)
    unsigned int eax, ebx, ecx, edx;
    unsigned high, low, signature;
    asm volatile("rdtscp" : "=a" (low), "=d" (high), "=c"(signature));
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    unsigned long long res = ((unsigned long long)low) | (((unsigned long long)high) << 32);
    return tsc_barrier_cycles::time_point(tsc_barrier_cycles::duration(res));
#else
    errx(1, "[trun] trun::time::tsc_barrier_cycles not supported in this system");
#endif
}

inline
unsigned long long
trun::time::tsc_barrier_cycles::frequency()
{
    static unsigned long long f = 0;
    if (f == 0) {
        auto t1 = now();
        sleep(2);
        auto t2 = now();
        f = (t2 - t1).count() / 2;
    }
    return f;
}

inline
std::chrono::duration<double, std::pico>
trun::time::tsc_barrier_cycles::cycle_time()
{
    auto f = frequency();
    return std::chrono::duration<double, std::ratio<1>>(1.0/f);
}

template<class Rep, class Period>
inline
std::chrono::duration<double, std::nano>
trun::time::tsc_barrier_cycles::time(const std::chrono::duration<Rep, Period> &d)
{
    auto unit = std::chrono::duration< double, std::ratio<1> >(d);
    auto t = unit / trun::time::tsc_barrier_cycles::frequency();
    return std::chrono::duration<double, std::nano>(t);
}

namespace trun {
    namespace time {

        namespace detail {

            void check(const ::trun::time::tsc_barrier_cycles &clock)
            {
                clock.check();
            }

        }

        template<class Ratio>
        static inline
        std::string
        units(const ::trun::time::tsc_barrier_cycles &clock)
        {
            return units<Ratio>(::trun::time::tsc_cycles());
        }

    }
}

#endif // TRUN__DETAIL__TIME_CYCLES_HPP
