# Quadratic Equation Solver

---

## Contents
1. [Project idea](#project-idea)
2. [How To Build And Run](#how-to-build-and-run)
3. [Details (Special conditions and limitations)](#details-special-conditions-and-limitations)
4. [TODO](#todo)

---

### Project idea
Just solve the Quadratic Equation. But using multithreading. And take into account effective data processing and general performance.

---

### How To Build And Run
_Development was done mainly on Linux (Debian 12), but also tested building and running on Windows._

On linux (using cmake and make):
```bash
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build ./build
```
Then pass arguments to the program `./so_solver [a] [b] [c] [a] [b] [c] ...`:
```bash
./se_solver -2 4 6 0 4 7 4 2 a 4 0 -16 100
```
And see the following output (order of the lines can be different):
```txt
(-2, 4, 6) => ([3], [-1]) (extremum: X=[1], Y=[8])
(0, 4, 7) => ([-1.75]) (no extremum)
(4 2 a) => not correct arguments for quadratic equation
(4, 0, -16) => ([-2], [2]) (extremum: X=[0], Y=[-16])
(100) => not enough arguments for quadratic equation
```

---

### Details (Special conditions and limitations)
We take the number of available threads (`thread::hardware_concurrency()`), divide this number into pairs - one thread will do parsing and the other will solve the equation. These are two separate functions: [**parse**](./src/SolvingManager.cpp?plain=1#L23) and [**solve**](./src/SolvingManager.cpp?plain=1#L73).
The number of all arguments is divided by the number of pairs of threads. That is, each pair of threads has predefined input arguments that this pair of threads will work with.<br/><br/>
For example, there are 4 threads available in the system. A user has requested the solution of 10 equations.
- 4 threads => 2 pairs of threads (“parse” and “solve”).
- 10 equations and 2 pairs of threads => each pair of threads is given 5 equations.

**The program does not guarantee the sequence of solutions to the equations.** That is, the solutions of the 3rd and 4th equations may be printed first, and then the solutions of the 1st and 2nd equations.
Within each pair of threads the interaction is done through [**ConcurrentQueue**](./src/ConcurrentQueue.hpp) and it would be possible to create several consumers or several producers.

Each thread can write messages to the console (the parser writes parsing error messages, and the solver writes solutions to the equations). To keep the output unmixed, std::cout is accessed through the [**ConsoleOutput**](./src/ConsoleOutput.hpp) class, which internally contains std::mutex for console output. If there are a lot of threads, they can all bump into ConsoleOutput for console output, so the output of the solve function is [pre-buffered](./src/SolvingManager.cpp?plain=1#L78) and passed to std::cout as a buffer - measurements have shown that this really speeds things up. Buffers are C-Style arrays, but we can consider translating this to C++-Style as a refinement.

---

### TODO
- Return to the program for errors and exceptions. Now errors from library functions are not handled.
- Check all C-Style code and switch to the C++-Style.