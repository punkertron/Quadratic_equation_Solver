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

inline static bool parseToInt(const char* str, int& result)
{
    return std::from_chars(str, str + std::strlen(str), result).ec == std::errc();
}

// Parse arguments. If successful than add them to the equationCoefQueue (another thread would
// monitor this queue) This function doesn't use Buffer for the output, because
//   1) It is assumed that the function will not issue error messages frequently.
//   2) The size of any lines can be really huge And there is no need to write a lot of code because
//   of the reason 1)
static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output)
{
    // firstly, check the number of arguments - must be 3
    // secondly, try parse each argument, and (if success) add them to the queue
    for (int i = 0; i < amount; i += EquationCoefficients::TOTAL_COEFFICIENTS_NUM) {
        int pos = startIndex + i;
        if (i + 3 > amount) {
            bool secondArgExists = i + 1 < amount;
            output.print('(', argv[pos], secondArgExists ? ", " : "",
                         secondArgExists ? argv[pos + 1] : "",
                         ") => not enough arguments for quadratic equation\n");
            break;
        }

        int a{};
        int b{};
        int c{};
        if (!parseToInt(argv[pos], a) || !parseToInt(argv[pos + 1], b) ||
            !parseToInt(argv[pos + 2], c)) {
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

// Take data from queue equationCoefQueue and find the roots of equation and extremum.
// Save the output to the Buffer. When Buffer is full, pass it to the ConsoleOutput.
//   The logic of bufferring isn't good, we assume that MAX_LENGTH_ONE_LINE would include one line.
//   But for this short solution it is fine Maybe also need to remove goto...
// This function looks huge, but the logic inside it simple. It's huge, because it handles edge
// cases
static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output)
{
    int i{0};
    static constexpr int BUFFER_SIZE{4096};
    static constexpr int MAX_LENGTH_ONE_LINE{300};
    char buffer[BUFFER_SIZE] = {'\0'};
    while (std::optional<EquationCoefficients> opt = equationCoefQueue.dequeue()) {
        const int a{opt->a_};
        const int b{opt->b_};
        const int c{opt->c_};

        // by such a check, we ensure that there is no going beyond the buffer boundaries
        if (i > BUFFER_SIZE - MAX_LENGTH_ONE_LINE) {
            output.print(buffer);
            std::memset(buffer, '\0', i);
            i = 0;
        }
        i += std::sprintf(buffer + i, "(%d, %d, %d) => ", a, b, c);

        long long discriminant = std::pow(b, 2) - 4LL * a * c;
        double temp{};
        double x1{};
        double x2{};

        if (a == 0) {
            if (b == 0) {
                if (c == 0) {
                    i += std::sprintf(buffer + i, "(any) ");
                } else {
                    i += std::sprintf(buffer + i, "(no roots) ");
                }
            } else {
                i += std::sprintf(buffer + i, "([%g]) ", -c / static_cast<double>(b));
            }
            i += std::sprintf(buffer + i, "(no extremum)\n");
            continue;
        }

        if (discriminant < 0) {
            i += std::sprintf(buffer + i, "(no roots) ");
            goto find_extremum;
        }
        temp = -0.5 * (b + sign(b) * std::sqrt(discriminant));
        x1 = temp / a;
        if (discriminant == 0) {
            i += std::sprintf(buffer + i, "([%g]) ", x1);
            goto find_extremum;
        }
        x2 = c / temp;
        i += std::sprintf(buffer + i, "([%g], [%g]) ", x1, x2);

    find_extremum:
        if (a == 0) {
            i += std::sprintf(buffer + i, "(no extremum)\n");
            continue;
        }
        const double x = -b / (2.0 * a);
        const double y = a * x * x + b * x + c;
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
    threads.reserve(numOfThreads);
    for (int i = 0; i * 2 < numOfThreads; ++i) {
        // start from 1 because argv[0] is not the parameter of equation
        int startIndex = 1 + i * argsPerOneBucket;
        // last bucket must include all remaining arguments
        int argsInBucket = (i + 1) * 2 == numOfThreads ? argc - startIndex : argsPerOneBucket;

        // 1st thread - "parse" function which adds data to the queue
        // 2nd thread - "solve" function which receive data from the queue
        threads.emplace_back(parse, std::ref(equationCoefQueue[i]), startIndex, argsInBucket, argv,
                             std::ref(output));
        threads.emplace_back(solve, std::ref(equationCoefQueue[i]), std::ref(output));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
