# Single Thread Solution

---

It's actually very fast, too.

It solves 50000+ equations and writes them to a file (using output redirection) in about 0.19 seconds on the author's machine (average value after several runs, the result of the `time` utility). A bonus is the consistent output of the results according to the order of the parameters.

The multithreaded version solves the same problem in 0.165 seconds (it can be even better, 0.155 sec if we use C-Style arrays for buffering).
This is not the gain one expects from implementing multithreading.
But maybe this is not the most suitable task for multithreading (too few arguments can be passed via argv).
Or the author has made a mistake somewhere :no_good:

It is also worth noting the running time of programs using chrono (with output redirection):
- 40 ms - Multithreaded version
- 60 ms - Singlethreaded version