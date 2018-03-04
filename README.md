# trun (*timed run*)

Small C++ infrastructure to time experiments in a statistically significant
way. This is particularly usefull for evaluating micro-benchmarks in a reliable
way.

The measured experiment is passed as a function, and *trun* will calculate the
time it takes to execute it. The simplest way of using *trun* has some sensible
defaults:

<pre>
#include <trun.hpp>

void experiment() {
    // do something important
}

int main(int argc, char *argv[]) {
    // run experiment
    auto results = trun::run(experiment);
    // print results to standard output
    trun::dump::csv(results);
}
</pre>

To set your own statistical parameters, simply pass a `trun::parameters` object
to `trun::run`:

<pre>
    trun::parameters params;
    // ... set parameters
    auto results = trun::run(params, experiment);
    trun::dump::csv(results);
</pre>

## Properties of experiment results

* Runs `experiment` until the results match the expected statistical
  significance (they converge):

  <pre>
  while (not converged) {
      // warmup system
      for(int warmup_batch_idx = 0; warmup_batch_idx < warmup_batch_size; warmup_batch_idx++) {
          experiment();
      }
      // start set of experiments
      for (int run = 0; run < num_runs; run++) {
          // start batch
          auto batch_start = time();
          for(int batch_idx = 0; batch_idx < batch_size; batch_idx++) {
              experiment();
          }
          auto batch_end = time();
          auto time = (batch_end - batch_start) / batch_size;
      }
      // ... calculate statistics ...
  }
  </pre>

* Uses `std::chrono::steady_clock` to measure execution time. Can be changed
  through the `Clock` template parameter.

* Runs `experiment` 30 times before each set of experiments to warm the system
  up. Can be changed through `trun::parameters::warmup_batch_size`.

* Calls to `experiment` are measured in batches to ensure the overhead of the
  selected clock is within 0.1% of the mean (the batch size is calculated
  dynamically, but starts at 30). Can be changed though
  `trun::parameters::run_size` and `trun::parameters::clock_overhead_perc`.

* Assumes the time to execute `experiment` follows a gaussian distribution, and
  discards measurement outliers with 99.99% confidence. Can be changed through
  `trun::parameters::confidence_outlier_sigma` (ignores runs beyond
  `trun::parameters::confidence_outlier_sigma * sigma` of the mean).

* Ensures the standard deviation is within 1% of the mean with 95.45%
  confidence. Can be changed through `trun::parameters::stddev_perc` and
  `trun::parameters::confidence_sigma`.

* Keeps running batches of experiments until 30 runs meet the selected criteria,
  or until 300 seconds have passed (and then assumes the experiment results do
  not converge). Can be changed through `trun::parameters::run_size_min_significance`
  and `trun::parameters::experiment_timeout`.


## Tapping into experiment runs

You can provide some functions that will be called at certain pre-defined points
in the evaluation process:

* Every time a set of experiments starts/stops (`func_iter_start` and
  `func_iter_stop`).

* Every time a run (batch of experiments) starts stops (`func_batch_start` and
  `func_batch_stop`).

* Every time a batch is selected as a non-outlier in a set of experiments
  (`func_batch_seletct`).

* Every time a set of experiments is selected as a candidate for the final
  results (`func_iter_select`). Can be called multiple times in case the results
  never converge.
