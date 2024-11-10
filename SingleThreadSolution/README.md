# Single Thread Solution

---

It's actually very fast, too. It solves 50000+ equations and writes them to a file (using output redirection) in about 0.19 seconds (average value after several runs, the result of the `time` utility). A bonus is the consistent output of the results according to the order of the parameters.

The multithreaded version solves the same problem in 0.182 seconds. This is not the gain one expects from implementing multithreading. But maybe this is not the most suitable task for multithreading. Or the author has made a mistake somewhere :no_good: