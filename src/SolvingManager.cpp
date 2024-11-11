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


// use C-Style arrays for buffering output.
static constexpr int BUFFER_SIZE{4096};
// if our current position in Buffer > (BUFFER_SIZE - MAX_LENGTH_ONE_LINE), then pass the Buffer to the ConsoleOutput.
static constexpr int MAX_LENGTH_ONE_LINE{300};


// Parse arguments.
// If successful, then add them to the equationCoefQueue (another thread would monitor this queue).
// Save the output with parsing errors to the buffer[BUFFER_SIZE]. Finally, pass this buffer to the ConsoleOutput.
//   IMPORTANT: At the very end, we call the "solve" function so that the producer becomes the consumer.
static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output);


// Take data from the queue equationCoefQueue and find the roots of the equation and extremum.
// Save the output to the buffer[BUFFER_SIZE]. Finally, pass this buffer to the ConsoleOutput.
static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output);



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// solve

inline static void findExtremum(char buffer[], int& i, const int a, const int b, const int c)
{
    const double x = -b / (2.0 * a);
    const double y = a * x * x + b * x + c;
    i += std::sprintf(buffer + i, "(extremum: X=[%g], Y=[%g])\n", x, y);
}

template <typename T>
inline static int sign(T value)
{
    return value < 0 ? -1 : 1;
}

inline static void findRootsNonParabola(char buffer[], int& i, const int b, const int c)
{
    if (b == 0) {
        if (c == 0) {
            i += std::sprintf(buffer + i, "(any) ");
        } else {
            i += std::sprintf(buffer + i, "(no roots) ");
        }
    } else {
        i += std::sprintf(buffer + i, "([%g]) ", -c / static_cast<double>(b));
    }
}

static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output)
{
    int i{0};
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

        if (a == 0) {  // not a parabola
            findRootsNonParabola(buffer, i, b, c);
            i += std::sprintf(buffer + i, "(no extremum)\n");
            continue;
        }

        long long discriminant = static_cast<long long>(std::pow(b, 2)) - 4LL * a * c;
        if (discriminant < 0) {
            i += std::sprintf(buffer + i, "(no roots) ");
            findExtremum(buffer, i, a, b, c);
            continue;
        }

        double temp = -0.5 * (b + sign(b) * std::sqrt(discriminant));
        double x1 = temp / a;
        if (discriminant == 0) {
            i += std::sprintf(buffer + i, "([%g]) ", x1);
            findExtremum(buffer, i, a, b, c);
            continue;
        }

        double x2 = c / temp;
        i += std::sprintf(buffer + i, "([%g], [%g]) ", x1, x2);

        findExtremum(buffer, i, a, b, c);
    }
    output.print(buffer);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// parse

inline static bool parseToInt(const char* str, int& result)
{
    auto [ptr, ec] = std::from_chars(str, str + std::strlen(str), result);
    return ec == std::errc() && ptr == str + std::strlen(str);
}

static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output)
{
    {
        int i{0};
        char buffer[BUFFER_SIZE] = {'\0'};
        for (int j = 0; j < amount; j += 3) {
            // by such a check, we ensure that there is no going beyond the buffer boundaries
            if (i > BUFFER_SIZE - MAX_LENGTH_ONE_LINE) {
                output.print(buffer);
                std::memset(buffer, '\0', i);
                i = 0;
            }

            int pos = startIndex + j;
            if (j + 3 > amount) {  // check the number of arguments - must be 3
                i += std::sprintf(buffer + i, "(%.20s", argv[pos]);
                if (j + 1 < amount) {  // check second arg exists or not
                    i += std::sprintf(buffer + i, ", %.20s", argv[pos + 1]);
                }
                i += std::sprintf(buffer + i, ") => not enough arguments for quadratic equation\n");
                break;
            }

            // try to parse each argument, and (if successful) add it to the queue
            int a{};
            int b{};
            int c{};
            if (!parseToInt(argv[pos], a) || !parseToInt(argv[pos + 1], b) ||
                !parseToInt(argv[pos + 2], c)) {
                i += std::sprintf(
                    buffer + i,
                    "(%.20s, %.20s, %.20s) => not correct arguments for quadratic equation\n",
                    argv[pos], argv[pos + 1], argv[pos + 2]);
                continue;
            }
            equationCoefQueue.enqueue({a, b, c});
        }
        output.print(buffer);
        equationCoefQueue.setDone();
    }

    // producer becomes consumer
    solve(equationCoefQueue, output);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// SolvingManager::run

inline static int getArgsPerOneBucket(const int argc, const int numOfThreads)
{
    int argsPerOneParser = std::max(3, argc / (numOfThreads / 2));
    // the argsPerOneParser must be a multiple of the number of coefficients
    while (argsPerOneParser % 3) {
        --argsPerOneParser;
    }
    return argsPerOneParser;
}

// split all arguments into buckets. for each bucket create a pair of threads (parse and solve)
// NOTE: parse call solve at the end
void SolvingManager::run(int argc, char** argv)
{
    const int numOfThreads =
        std::max(static_cast<int>(std::thread::hardware_concurrency()), 2);  // never can be odd?
    const int argsPerOneBucket = getArgsPerOneBucket(argc, numOfThreads);

    ConsoleOutput output;  // Output from threads is only done through this class.
    std::vector<ConcurrentQueue<EquationCoefficients>> equationCoefQueues(numOfThreads / 2);
    std::vector<std::thread> threads;
    threads.reserve(numOfThreads);
    for (int i = 0; i * 2 < numOfThreads; ++i) {
        // start from 1 because argv[0] is not the parameter of equation
        int startIndex = 1 + i * argsPerOneBucket;
        // last bucket must include all remaining arguments
        int argsInBucket = (i + 1) * 2 == numOfThreads ? argc - startIndex : argsPerOneBucket;

        // 1st thread - "parse", which parses and adds data to the queue. After parsing, calls "solve".
        // 2nd thread - "solve", which receives data from the queue and solves the equation.
        threads.emplace_back(parse, std::ref(equationCoefQueues[i]), startIndex, argsInBucket, argv,
                             std::ref(output));
        threads.emplace_back(solve, std::ref(equationCoefQueues[i]), std::ref(output));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
