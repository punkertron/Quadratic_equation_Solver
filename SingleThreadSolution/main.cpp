#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <system_error>  // for errc

// large buffer for output
static constexpr int BUFFER_SIZE{65536};
static constexpr int MAX_LENGTH_ONE_LINE{300};
static char output_buffer[BUFFER_SIZE];
static size_t buffer_offset{0};

static inline void print()
{
    std::printf("%s", output_buffer);
    std::memset(output_buffer, '\0', buffer_offset);
    buffer_offset = 0;
}

// Function to append formatted output to the buffer.
inline static void appendToBuffer(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int written =
        std::vsnprintf(output_buffer + buffer_offset, BUFFER_SIZE - buffer_offset, format, args);
    va_end(args);

    if (written > 0) {
        buffer_offset += written;
        if (buffer_offset >= BUFFER_SIZE - MAX_LENGTH_ONE_LINE) {
            print();
        }
    }
}

// Parse an integer from a string. Returns true if successful
inline static bool parseToInt(const char* str, int& result)
{
    return std::from_chars(str, str + std::strlen(str), result).ec == std::errc();
}

// Solve the quadratic equation and buffer the result
inline static void solve(int a, int b, int c)
{
    if (a == 0) {
        if (b != 0) {
            double root = -static_cast<double>(c) / b;
            appendToBuffer(" => ([%g]) (no extremum)\n", root);
        } else {
            appendToBuffer(" => no roots (degenerate equation)\n");
        }
        return;
    }

    long long discriminant = std::pow(b, 2) - 4LL * a * c;
    const double inv_2a = 0.5 / a;

    if (discriminant > 0) {
        double sqrt_discriminant = std::sqrt(discriminant);
        double root1 = (-b - sqrt_discriminant) * inv_2a;
        double root2 = (-b + sqrt_discriminant) * inv_2a;
        appendToBuffer(" => ([%g], [%g])", root1, root2);
    } else if (discriminant == 0) {
        double root = -b * inv_2a;
        appendToBuffer(" => ([%g])", root);
    } else {
        appendToBuffer(" => (no roots)");
    }

    const double x = -b * inv_2a;
    const double y = a * x * x + b * x + c;
    appendToBuffer(" (extremum: X=[%g], Y=[%g])\n", x, y);
}

int main(int argc, char* argv[])
{
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    auto start = steady_clock::now();
    {
        if (argc < 4) {
            std::printf("Usage: ./se_solver [a1] [b1] [c1] [a2] [b2] [c2] ...\n");
            return 1;
        }

        for (int i = 1; i < argc; i += 3) {
            if (i + 2 >= argc) {
                appendToBuffer("(");
                for (int j = i; j < argc; ++j) {
                    appendToBuffer("%s%s", argv[j], (j < argc - 1 ? " " : ""));
                }
                appendToBuffer(") => not enough arguments for quadratic equation\n");
                break;
            }

            int a{};
            int b{};
            int c{};
            if (!parseToInt(argv[i], a) || !parseToInt(argv[i + 1], b) ||
                !parseToInt(argv[i + 2], c)) {
                appendToBuffer("(%s, %s, %s) => not correct arguments for quadratic equation\n",
                               argv[i], argv[i + 1], argv[i + 2]);
                continue;
            }

            appendToBuffer("(%d, %d, %d)", a, b, c);
            solve(a, b, c);
        }
        print();
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
