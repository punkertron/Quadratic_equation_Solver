#include <iostream>

#include "SolvingManager.hpp"

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cout << "Usage: ./se_solver [a] [b] [c]\n";
        return 1;
    }
    SolvingManager sm;
    sm.run(argc, argv);
    return 0;
}
