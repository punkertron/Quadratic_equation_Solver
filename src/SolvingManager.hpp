#ifndef SOLVER_MANAGER_HPP
#define SOLVER_MANAGER_HPP

#include "ConcurrentQueue.hpp"
#include "ConsoleOutput.hpp"
#include "EquationCoefficients.hpp"

class SolvingManager {
public:
    void run(int argc, char** argv);
};

#endif  // SOLVER_MANAGER_HPP