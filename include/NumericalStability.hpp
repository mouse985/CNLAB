#pragma once

#include "Matrix.hpp"
#include <limits>

namespace cnlab {

class NumericalStability {
public:
    static constexpr double EPSILON = 1e-15;
    static constexpr double SAFE_MIN = 1e-300;
    static constexpr double SAFE_MAX = 1e300;
    static constexpr double CONDITION_THRESHOLD = 1e15;
    
    static bool isNearZero(double x, double tol = EPSILON);
    static bool isNearEqual(double a, double b, double tol = EPSILON);
    static bool isFinite(double x);
    static bool isValid(const Matrix& A);
    static bool isWellConditioned(const Matrix& A, double threshold = CONDITION_THRESHOLD);
    
    static double safeAdd(double a, double b);
    static double safeSubtract(double a, double b);
    static double safeMultiply(double a, double b);
    static double safeDivide(double a, double b);
    static double safeSqrt(double x);
    static double safeLog(double x);
    static double safeExp(double x);
    static double safePow(double base, double exp);
    
    static Matrix safeInverse(const Matrix& A);
    static Matrix safeSolve(const Matrix& A, const Matrix& b);
    static double conditionNumber(const Matrix& A);
    static double estimateRank(const Matrix& A, double tol = EPSILON);
    
    static void scaleMatrix(Matrix& A, double& scale);
    static Matrix balancedMatrix(const Matrix& A);
    
    static double kahanSum(const double* data, size_t n);
    static double compensatedDotProduct(const double* a, const double* b, size_t n);
};

}
