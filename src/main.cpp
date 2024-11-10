#include <chrono>
#include <iostream>

#include "SolvingManager.hpp"

int main(int argc, char** argv)
{
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    auto start = steady_clock::now();

    {
        if (argc < 4) {
            std::cout << "Usage: ./se_solver [a1] [b1] [c1] [a2] [b2] [c2] ...\n";
            return 1;
        }
        SolvingManager sm;
        sm.run(argc, argv);
    }

    auto finish = steady_clock::now();
    auto duration_ms = duration_cast<milliseconds>(finish - start).count();
    auto duration_us = duration_cast<microseconds>(finish - start).count();

    if (duration_ms < 1) {
        std::cout << "Time elapsed: " << duration_us << "µs\n";
    } else {
        std::cout << "Time elapsed: " << duration_ms << "ms\n";
    }
    return 0;
}
