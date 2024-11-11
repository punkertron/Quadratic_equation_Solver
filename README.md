# Quadratic Equation Solver

---

## Contents
1. [Project idea](#project-idea)
2. [How To Build And Run](#how-to-build-and-run)
3. [Details and Special conditions and limitations](#details-and-special-conditions-and-limitations)
4. [TODO](#todo)

---

### Project idea
Just solve the Quadratic Equation. But using multithreading. And take into account effective data processing and general performance. Preferably Producer-Consumer pattern.

---

### How To Build And Run
_Development was done mainly on Linux (Debian 12), but also tested building on Windows (Visual Studio)._

On Linux:
```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build ./build
```
Then you can pass arguments to the program `./so_solver [a1] [b1] [c1] [a2] [b2] [c2] ...`:
```bash
./se_solver -2 4 6 0 4 7 4 2 a 4 0 -16 100
```
And you can see the following output (order of the lines can be different):
```txt
(-2, 4, 6) => ([3], [-1]) (extremum: X=[1], Y=[8])
(0, 4, 7) => ([-1.75]) (no extremum)
(4 2 a) => not correct arguments for quadratic equation
(4, 0, -16) => ([-2], [2]) (extremum: X=[0], Y=[-16])
(100) => not enough arguments for quadratic equation
time elapsed: 557µs
```

There are also [test.data](./data/test.data) file and you can solve 50000+ equations via
```bash
./build/se_solver $(cat data/test.data)
```

---

### Details And Special conditions and limitations
**_Briefly_**: we split arguments into buckets. Each bucket is served by a pair of threads: one parser and one solver. The parser adds the coefficients of the equation to the queue, and the solver takes the parameters from the queue and solves the equation. **The thread with parser after finishing parsing also calls solve function (thus, two solving threads will work on one queue).**

Threads can interfere with each other if we send them all to work with the same queue. But we know the number of arguments in advance and we can split them into buckets with their own queue.
But I think this solution will not work if the program will work as a server.

<br/>

**_Details_**: We take the number of available threads (`thread::hardware_concurrency()`), split this number into pairs - one thread will do parsing and the other will solve the equation. These are two separate functions: [**parse**](./src/SolvingManager.cpp?plain=1#L113) and [**solve**](./src/SolvingManager.cpp?plain=1#L64).
The number of all arguments is divided by the number of pairs of threads. That is, each pair of threads has predefined input arguments that this pair of threads will work with.<br/><br/>
For example, A user has requested the solution of 10 equations, `thread::hardware_concurrency()` returns 4.
- 4 threads => 2 pairs of threads (“parse” and “solve”).
- 10 equations and 2 pairs of threads => each pair of threads is given 5 equations.

**NOTE**: The **parse** function calls the **solve** function at the very end. Producer becomes Consumer.

**The program does not guarantee the sequence of solutions to the equations.** That is, the solutions of the 3rd and 4th equations may be printed first, and then the solutions of the 1st and 2nd equations.
Within each pair of threads the interaction is done through [**ConcurrentQueue**](./src/ConcurrentQueue.hpp) and it would be possible to create several consumers or several producers.

Each thread can write messages to the console (the parser writes parsing error messages, and the solver writes solutions to the equations). To keep the output unmixed, std::cout is accessed through the [**ConsoleOutput**](./src/ConsoleOutput.hpp) class, which internally contains std::mutex for console output. If there are many threads, they may all bump into ConsoleOutput for console output, so they all have their own std::ostringstream and pass it to ConsoleOutput at the end.

---

### TODO
- Check for errors and exceptions. Now errors from library functions are not handled.
- Add more programming principles, OOP, patterns, etc. For example, we have only `ConsoleOutput` class, but we may want to have `FileOutput` class, and for this we could make one base class for all Outputs and pass a pointer or reference to it to "solve" and "parse" functions. But now for this small task I decided not to complicate the code with interfaces.
- Add some kind of Buffering for the output in **solve** and **parse** functions. Previously C-Style arrays, `sprintf` and `memset` functions were used. This speeded up the application, but for simplicity I removed it. But it would be good to add some buffer.
- Change the work with small amount of arguments. Don't need to create threads and complex logic to solve 10 equations.