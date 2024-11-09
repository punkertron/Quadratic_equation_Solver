#include "SolvingManager.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <optional>
#include <system_error>  // for errc
#include <thread>
#include <vector>

#include "ConcurrentQueue.hpp"
#include "ConsoleOutput.hpp"
#include "EquationCoefficients.hpp"

static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output)
{
    for (int i = 0; i < amount; i += 3) {
        int pos = startIndex + i;
        if (i + 3 > amount) {
            bool secondArgExists = i + 1 < amount;
            output.print('(', argv[pos], secondArgExists ? ", " : "",
                         secondArgExists ? argv[pos + 1] : "",
                         ") => not enough arguments for quadratic equation\n");
            break;
        }

        int a{};
        if (std::from_chars(argv[pos], argv[pos] + std::strlen(argv[pos]), a).ec != std::errc()) {
            output.print('(', argv[pos], ' ', argv[pos + 1], ' ', argv[pos + 2],
                         ") => not correct arguments for quadratic equation\n");
            continue;
        }
        int b{};
        if (std::from_chars(argv[pos + 1], argv[pos + 1] + std::strlen(argv[pos + 1]), b).ec !=
            std::errc()) {
            output.print('(', argv[pos], ' ', argv[pos + 1], ' ', argv[pos + 2],
                         ") => not correct arguments for quadratic equation\n");
            continue;
        }
        int c{};
        if (std::from_chars(argv[pos + 2], argv[pos + 2] + std::strlen(argv[pos + 2]), c).ec !=
            std::errc()) {
            output.print('(', argv[pos], ' ', argv[pos + 1], ' ', argv[pos + 2],
                         ") => not correct arguments for quadratic equation\n");
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
    int i{0};
    static constexpr int BUFFER_SIZE{2048};
    static constexpr int MAX_LENGTH_ONE_LINE{200};
    char buffer[BUFFER_SIZE] = {'\0'};
    while (std::optional<EquationCoefficients> opt = equationCoefQueue.dequeue()) {
        // by such a check, we ensure that there is no going beyond the buffer boundaries
        if (i > BUFFER_SIZE - MAX_LENGTH_ONE_LINE) {
            output.print(buffer);
            std::memset(buffer, '\0', i);
            i = 0;
        }
        i += std::sprintf(buffer + i, "(%d, %d, %d) => ", opt->a_, opt->b_, opt->c_);

        long long discriminant = (opt->b_ * opt->b_ - 4 * opt->a_ * opt->c_);
        double temp{};
        double x1{};
        double x2{};

        if (opt->a_ == 0) {
            if (opt->b_ == 0) {
                if (opt->c_ == 0) {
                    i += std::sprintf(buffer + i, "(any) ");
                } else {
                    i += std::sprintf(buffer + i, "(no roots) ");
                }
            } else {
                i += std::sprintf(buffer + i, "([%g]) ", -opt->c_ / static_cast<double>(opt->b_));
            }
            i += std::sprintf(buffer + i, "(no extremum)\n");
            continue;
        }

        if (discriminant < 0) {
            i += std::sprintf(buffer + i, "(no roots) ");
            goto find_extremum;
        }
        temp = -0.5 * (opt->b_ + sign(opt->b_) * std::sqrt(discriminant));
        x1 = temp / opt->a_;
        if (discriminant == 0) {
            i += std::sprintf(buffer + i, "([%g]) ", x1);
            goto find_extremum;
        }
        x2 = opt->c_ / temp;
        i += std::sprintf(buffer + i, "([%g], [%g]) ", x1, x2);

    find_extremum:
        if (opt->a_ == 0) {
            i += std::sprintf(buffer + i, "(no extremum)\n");
            continue;
        }
        const double x = -opt->b_ / (2.0 * opt->a_);
        const double y = opt->a_ * x * x + opt->b_ * x + opt->c_;
        i += std::sprintf(buffer + i, "(extremum: X=[%g], Y=[%g])\n", x, y);
    }
    output.print(buffer);
}

static int getArgsPerOneBucket(const int argc, const int numOfThreads)
{
    int argsPerOneParser = std::max(argc < EquationCoefficients::TOTAL_COEFFICIENTS_NUM
                                        ? argc
                                        : EquationCoefficients::TOTAL_COEFFICIENTS_NUM,
                                    argc / (numOfThreads / 2));
    // the argsPerOneParser must be a multiple of the number of coefficients
    while (argsPerOneParser % EquationCoefficients::TOTAL_COEFFICIENTS_NUM) {
        --argsPerOneParser;
    }
    return argsPerOneParser;
}

void SolvingManager::run(int argc, char** argv)
{
    const int numOfThreads =
        std::max(static_cast<int>(std::thread::hardware_concurrency()), 2);  // never can be odd?
    const int argsPerOneBucket = getArgsPerOneBucket(argc, numOfThreads);

    ConsoleOutput output;
    std::vector<ConcurrentQueue<EquationCoefficients>> equationCoefQueue(numOfThreads / 2);
    std::vector<std::thread> threads;
    for (int i = 0; i * 2 < numOfThreads; ++i) {
        int startIndex = 1 + i * argsPerOneBucket;
        // last bucket must include all remaining arguments
        int argsInBucket = i * 2 == numOfThreads / 2 ? argc - startIndex : argsPerOneBucket;
        threads.emplace_back(parse, std::ref(equationCoefQueue[i]), startIndex, argsInBucket, argv,
                             std::ref(output));
        threads.emplace_back(solve, std::ref(equationCoefQueue[i]), std::ref(output));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
