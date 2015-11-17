/** trun/detail/dump.hpp ---
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

#ifndef TRUN__DETAIL__DUMP_HPP
#define TRUN__DETAIL__DUMP_HPP 1

#include <chrono>
#include <fstream>
#include <ratio>
#include <type_traits>


namespace trun {
    namespace dump {

        static inline
        std::ofstream
        stream(const std::string &output)
        {
            return std::ofstream(output, std::ios::out);
        }

        template<class Ratio = std::nano, class Clock>
        static inline
        void csv(const result<Clock> &results, std::ostream & output,
                 bool show_header, bool force_converged)
        {
            if (force_converged && !results.converged) {
                errx(1, "[trun] Results did not converge (tried %d runs on batches of %d)",
                     results.run_size_all, results.batch_size);
            }

            if (show_header) {
                csv_header<Ratio>(results, output, show_header, force_converged);
            }

            auto one = [&](typename result<Clock>::duration t) {
                output << std::chrono::duration<double, Ratio>(t).count() << ",";
            };
            one(results.mean);
            one(results.sigma);
            one(results.min);
            one(results.max);
            output << results.run_size << ","
                   << results.batch_size << ","
                   << results.converged << ",";

            one(results.min_all);
            one(results.max_all);
            output << results.run_size_all << "\n";
        }

        template<class Ratio = std::nano, class Clock>
        static inline
        void csv_header(const result<Clock> &results, std::ostream & output,
                        bool show_header, bool force_converged)
        {
            if (force_converged && !results.converged) {
                errx(1, "[trun] Results did not converge (tried %d runs on batches of %d)",
                     results.run_size_all, results.batch_size);
            }

            if (!show_header) {
                return;
            }

            std::string units = ::trun::time::units<Ratio>(Clock());
            output << "mean(" << units << "),"
                   << "sigma(" << units << "),"
                   << "min(" << units << "),"
                   << "max(" << units << "),"
                   << "run_size,batch_size,converved,"
                   << "min_all(" << units << "),"
                   << "max_all(" << units << "),"
                   << "run_size_all\n";
        }

    }
}

#endif // TRUN__DETAIL__DUMP_HPP
