#include "SolvingManager.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <functional>
#include <optional>
#include <sstream>
#include <system_error>  // for errc
#include <thread>
#include <utility>
#include <vector>

#include "ConcurrentQueue.hpp"
#include "ConsoleOutput.hpp"
#include "EquationCoefficients.hpp"


// Parse arguments.
// If successful, then add them to the equationCoefQueue (another thread would monitor this queue).
// Save the output with parsing errors to the ostringstream. Finally, pass this stream to the ConsoleOutput.
//   IMPORTANT: At the very end, we call the "solve" function so that the producer becomes the consumer.
static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output);


// Take data from the queue equationCoefQueue and find the roots of the equation and extremum.
// Save the output to the ostringstream. Finally, pass this stream to the ConsoleOutput.
static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output);



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// solve

inline static void findExtremum(std::ostringstream& oss, const int a, const int b, const int c)
{
    const double x = -b / (2.0 * a);
    const double y = a * x * x + b * x + c;
    oss << "(extremum: X=[" << x << "], Y=[" << y << "])\n";
}

template <typename T>
inline static int sign(T value)
{
    return value < 0 ? -1 : 1;
}

inline static void findRootsNonParabola(std::ostringstream& oss, const int b, const int c)
{
    if (b == 0) {
        if (c == 0) {
            oss << "(any) ";
        } else {
            oss << "(no roots) ";
        }
    } else {
        oss << '(' << -c / static_cast<double>(b) << ") ";
    }
}

static void solve(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, ConsoleOutput& output)
{
    std::ostringstream oss;
    while (std::optional<EquationCoefficients> opt = equationCoefQueue.dequeue()) {
        const int a{opt->a_};
        const int b{opt->b_};
        const int c{opt->c_};

        oss << '(' << a << ", " << b << ", " << c << ") => ";

        if (a == 0) {  // not a parabola
            findRootsNonParabola(oss, b, c);
            oss << "(no extremum)\n";
            continue;
        }

        long long discriminant = std::pow(b, 2) - 4LL * a * c;
        if (discriminant < 0) {
            oss << "(no roots) ";
            findExtremum(oss, a, b, c);
            continue;
        }

        double temp = -0.5 * (b + sign(b) * std::sqrt(discriminant));
        double x1 = temp / a;
        if (discriminant == 0) {  // one root
            oss << "([" << x1 << "]) ";
            findExtremum(oss, a, b, c);
            continue;
        }

        double x2 = c / temp;
        oss << "([" << x1 << "], [" << x2 << "]) ";
        findExtremum(oss, a, b, c);
    }
    output.print(std::move(*oss.rdbuf()).str());
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// parse

inline static bool parseToInt(const char* str, int& result)
{
    return std::from_chars(str, str + std::strlen(str), result).ec == std::errc();
}

static void parse(ConcurrentQueue<EquationCoefficients>& equationCoefQueue, const int startIndex,
                  const int amount, char** argv, ConsoleOutput& output)
{
    {
        std::ostringstream oss;
        for (int i = 0; i < amount; i += 3) {
            int pos = startIndex + i;
            if (i + 3 > amount) {  // check the number of arguments - must be 3
                oss << '(' << argv[pos];
                if (i + 1 < amount) {  // check second arg exists or not
                    oss << ", " << argv[pos + 1];
                }
                oss << ") => not enough arguments for quadratic equation\n";
                break;
            }

            // try to parse each argument, and (if successful) add it to the queue
            int a{};
            int b{};
            int c{};
            if (!parseToInt(argv[pos], a) || !parseToInt(argv[pos + 1], b) ||
                !parseToInt(argv[pos + 2], c)) {
                oss << '(' << argv[pos] << ' ' << argv[pos + 1] << ' ' << argv[pos + 2]
                    << ") => not correct arguments for quadratic equation\n";
                continue;
            }
            equationCoefQueue.enqueue({a, b, c});
        }
        equationCoefQueue.setDone();
        output.print(std::move(*oss.rdbuf()).str());
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
