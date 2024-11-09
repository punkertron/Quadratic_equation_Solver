#ifndef EQUATION_COEFFICIENTS_HPP
#define EQUATION_COEFFICIENTS_HPP

// a_*x^2 + b_*x + c_ = 0
struct EquationCoefficients {
public:
    int a_{};
    int b_{};
    int c_{};
    static constexpr int TOTAL_COEFFICIENTS_NUM{3};
};

#endif  // EQUATION_COEFFICIENTS_HPP