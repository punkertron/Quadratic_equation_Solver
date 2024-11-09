#include "SolvingManager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

#include "EquationCoefficients.hpp"

static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output)
{
    for (int i = 0; i < amount; i += 3) {
        if (i + 3 > amount) {
            output.print("error\n");
            break;
        }
        int pos = startIndex + i;
        char* end;
        int a = std::strtol(argv[pos], &end, 10);
        if (*end != '\0') {
            output.print("error parsing a ", argv[pos]);
            continue;
        }
        int b = std::strtol(argv[pos + 1], &end, 10);
        if (*end != '\0') {
            std::cout << "error parsing b " << argv[pos + 1] << std::endl;
            continue;
        }
        int c = std::strtol(argv[pos + 2], &end, 10);
        if (*end != '\0') {
            std::cout << "error parsing c " << argv[pos + 2] << std::endl;
            continue;
        }
        equationCoefQueue.enqueue({a, b, c});
    }
    equationCoefQueue.setDone();
}

template <typename T>
inline static int sign(T value)
{
    return value < 0 ? -1 : 1;
}

static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output)
{
    int i = 0;
    char buffer[2048] = {'\0'};
    while (std::optional<EquationCoefficients> opt = equationCoefQueue.dequeue()) {
        if (i > 1948) {
            output.print(buffer);
            std::memset(buffer, '\0', i);
            i = 0;
        }
        long long discriminant = (opt->b_ * opt->b_ - 4 * opt->a_ * opt->c_);
        double temp, x1, x2;
        if (discriminant < 0) {
            i += std::sprintf(buffer + i, "(%d, %d, %d) => (no roots) ", opt->a_, opt->b_, opt->c_);
            goto find_extremum;
        }
        temp = -0.5 * (opt->b_ + sign(opt->b_) * std::sqrt(discriminant));
        x1 = temp / opt->a_;
        if (discriminant == 0) {
            i += std::sprintf(buffer + i, "(%d, %d, %d) => ([%g]) ", opt->a_, opt->b_, opt->c_, x1);
            goto find_extremum;
        }
        x2 = opt->c_ / temp;
        i += std::sprintf(buffer + i, "(%d, %d, %d) => ([%g], [%g]) ", opt->a_, opt->b_, opt->c_,
                          x1, x2);

    find_extremum:
        if (opt->a_ == 0) {
            i += std::sprintf(buffer + i, "(no vertex)\n");
            continue;
        }
        const double x = -opt->b_ / 2.0 / opt->a_;
        const double y = opt->a_ * x * x + opt->b_ * x + opt->c_;
        i += std::sprintf(buffer + i, "(Xmin=[%g], Ymin=[%g])\n", x, y);
    }
    output.print(buffer);
}

static int getArgsPerOneBucket(const int argc, const int numOfThreads)
{
    int argsPerOneParser = std::max(argc < EquationCoefficients::TOTAL_COEFFICIENTS_NUM
                                        ? argc
                                        : EquationCoefficients::TOTAL_COEFFICIENTS_NUM,
                                    argc / (numOfThreads / 2));
    while (argsPerOneParser % EquationCoefficients::TOTAL_COEFFICIENTS_NUM) {
        --argsPerOneParser;
    }
    return argsPerOneParser;
}

void SolvingManager::run(int argc, char** argv)
{
    const int numOfThreads = std::max(static_cast<int>(std::thread::hardware_concurrency()), 2);
    const int argsPerOneBucket = getArgsPerOneBucket(argc, numOfThreads);

    ConsoleOutput output;
    std::vector<ConcurrentQueue<EquationCoefficients>> equationCoefQueue(numOfThreads / 2);
    std::vector<std::thread> threads;
    for (int i = 0; i * 2 < numOfThreads; ++i) {
        int startIndex = 1 + i * argsPerOneBucket;
        int argsInBucket = i * 2 == numOfThreads / 2 ? argc - startIndex : argsPerOneBucket;
        threads.emplace_back(parse, std::ref(equationCoefQueue[i]), startIndex, argsInBucket, argv,
                             std::ref(output));
        threads.emplace_back(solve, std::ref(equationCoefQueue[i]), std::ref(output));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
