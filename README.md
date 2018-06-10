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
      size_t batch_group_size = ... dynamic ...;
      size_t batch_size = ... dynamic ...;
      // warmup system
      for(size_t i = 0; i < warmup_batch_group_size; i++) {
          experiment();
      }
      // start a group of experiment batches
      for (size_t i = 0; i < batch_group_size; i++) {
          // warmup batch
          for(size_t j = 0; j < warmup_batch_size; j++) {
              experiment();
          }
          // time batch
          auto batch_start = time();
          for(size_t j = 0; j < batch_size; j++) {
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

* Runs `experiment` multiple times before each set of timed experiments to warm
  the system up. Can be changed through
  `trun::parameters::warmup_batch_group_size` and
  `trun::parameters::warmup_batch_size`.

* Calls to `experiment` are measured in batches to ensure the overhead of the
  selected clock is within 0.1% of the mean (the batch size is calculated
  dynamically). Can be changed though `trun::parameters::batch_size` and
  `trun::parameters::clock_overhead_perc`.

* Assumes the time to execute `experiment` follows a gaussian distribution, and
  discards measurement outliers with 99.99% confidence. Can be changed through
  `trun::parameters::confidence_outlier_sigma` (ignores runs beyond
  `trun::parameters::confidence_outlier_sigma * sigma` of the mean).

* Ensures the standard deviation is within 1% of the mean with 95.45%
  confidence. Can be changed through `trun::parameters::stddev_perc` and
  `trun::parameters::confidence_sigma`.

* Keeps running the experiment until the measurements meet the selected
  criteria, or until 300 seconds have passed (and then assumes the experiment
  results do not converge). Can be changed through
  `trun::parameters::batch_group_min_size` and
  `trun::parameters::experiment_timeout`.


## Advanced usage

You can time more complex experiments by passing the object trun::mod_clock to
trun::run(). In this case, it is up to your function to calculate the execution
time of an experment batch.

This can be handy when timing external programs, kernel functions, etc.
