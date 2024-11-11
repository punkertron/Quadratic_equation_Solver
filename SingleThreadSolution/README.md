# Single Thread Solution

---

It's actually very fast, too.

Single-threaded program solves 50000+ equations and writes them to a file (using output redirection) in about 0.185 seconds on the author's machine (average value after several runs, the result of the `time` utility). A bonus is the consistent output of the results according to the order of the parameters.

The multithreaded version solves the same problem in 0.155 seconds. This is not the gain one expects from implementing multithreading. But maybe this is not the most suitable task for multithreading (too few arguments can be passed via argv).
Or the author has made a mistake somewhere :no_good:

Also interesting statistics. When output to the console Elapsed time is (but these results are unstable):
- 470 ms - Multithreaded
- 430 ms - Single-threaded

It is also worth noting the running time of programs using chrono (with output redirection to the file):
- 33 ms - Multithreaded
- 60 ms - Single-threaded