#include "NumericalStability.hpp"
#include "LinearAlgebra.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

namespace cnlab {

bool NumericalStability::isNearZero(double x, double tol) {
    return std::abs(x) < tol;
}

bool NumericalStability::isNearEqual(double a, double b, double tol) {
    if (std::isinf(a) || std::isinf(b)) {
        return a == b;
    }
    double diff = std::abs(a - b);
    if (diff < tol) return true;
    double maxab = std::max(std::abs(a), std::abs(b));
    return diff < tol * maxab;
}

bool NumericalStability::isFinite(double x) {
    return std::isfinite(x) && std::abs(x) < SAFE_MAX && std::abs(x) > SAFE_MIN;
}

bool NumericalStability::isValid(const Matrix& A) {
    const double* data = A.data();
    size_t n = A.size();
    
    for (size_t i = 0; i < n; ++i) {
        if (!std::isfinite(data[i])) {
            return false;
        }
    }
    return true;
}

double NumericalStability::safeDivide(double a, double b) {
    if (isNearZero(b)) {
        if (isNearZero(a)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return (a > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    double result = a / b;
    
    if (!std::isfinite(result)) {
        if (std::isnan(result)) {
            return 0.0;
        }
        return (result > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    return result;
}

double NumericalStability::safeSqrt(double x) {
    if (x < 0) {
        return 0.0;
    }
    if (x > SAFE_MAX) {
        return SAFE_MAX;
    }
    return std::sqrt(x);
}

double NumericalStability::safeLog(double x) {
    if (x <= 0) {
        return -SAFE_MAX;
    }
    if (x > SAFE_MAX) {
        return std::log(SAFE_MAX);
    }
    return std::log(x);
}

Matrix NumericalStability::safeInverse(const Matrix& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("Matrix must be square for inverse");
    }
    
    double cond = conditionNumber(A);
    if (cond > 1e15) {
        throw std::runtime_error("Matrix is ill-conditioned, inverse may be inaccurate");
    }
    
    return A.inverse();
}

double NumericalStability::conditionNumber(const Matrix& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("Condition number requires square matrix");
    }
    
    try {
        SVDResult svd = svd_decomposition(A);
        double maxSingular = 0.0;
        double minSingular = SAFE_MAX;
        size_t n = std::min(A.rows(), A.cols());
        
        for (size_t i = 0; i < n; ++i) {
            double s = svd.S(i, i);
            if (s > maxSingular) maxSingular = s;
            if (s < minSingular && s > EPSILON) minSingular = s;
        }
        
        if (minSingular < EPSILON) {
            return SAFE_MAX;
        }
        
        return maxSingular / minSingular;
    } catch (...) {
        return SAFE_MAX;
    }
}

double NumericalStability::estimateRank(const Matrix& A, double tol) {
    try {
        SVDResult svd = svd_decomposition(A);
        size_t rank = 0;
        size_t n = std::min(A.rows(), A.cols());
        
        double maxSingular = 0.0;
        for (size_t i = 0; i < n; ++i) {
            if (svd.S(i, i) > maxSingular) {
                maxSingular = svd.S(i, i);
            }
        }
        
        double threshold = tol * std::max(A.rows(), A.cols()) * maxSingular;
        
        for (size_t i = 0; i < n; ++i) {
            if (svd.S(i, i) > threshold) {
                rank++;
            }
        }
        
        return static_cast<double>(rank);
    } catch (...) {
        return 0.0;
    }
}

void NumericalStability::scaleMatrix(Matrix& A, double& scale) {
    size_t m = A.rows();
    size_t n = A.cols();
    
    double maxVal = 0.0;
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            double val = std::abs(A(i, j));
            if (val > maxVal) maxVal = val;
        }
    }
    
    if (maxVal > SAFE_MAX || maxVal < SAFE_MIN) {
        scale = 1.0 / maxVal;
        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                A(i, j) *= scale;
            }
        }
    } else {
        scale = 1.0;
    }
}

Matrix NumericalStability::balancedMatrix(const Matrix& A) {
    if (!A.isSquare()) {
        return A;
    }
    
    size_t n = A.rows();
    Matrix B = A;
    
    std::vector<double> rowNorms(n, 0.0);
    std::vector<double> colNorms(n, 0.0);
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            double val = std::abs(B(i, j));
            rowNorms[i] += val;
            colNorms[j] += val;
        }
    }
    
    for (size_t iter = 0; iter < 100; ++iter) {
        bool converged = true;
        
        for (size_t i = 0; i < n; ++i) {
            if (isNearZero(rowNorms[i]) || isNearZero(colNorms[i])) {
                continue;
            }
            
            double g = rowNorms[i] / 2.0;
            double f = colNorms[i] / 2.0;
            double s = std::sqrt(f / g);
            
            if (!std::isfinite(s) || isNearZero(s)) {
                continue;
            }
            
            if (std::abs(s - 1.0) > 0.01) {
                converged = false;
                
                for (size_t j = 0; j < n; ++j) {
                    B(i, j) *= s;
                    B(j, i) /= s;
                }
                
                rowNorms[i] *= s;
                colNorms[i] /= s;
            }
        }
        
        if (converged) break;
    }
    
    return B;
}

bool NumericalStability::isWellConditioned(const Matrix& A, double threshold) {
    if (!A.isSquare()) {
        return false;
    }
    
    try {
        double cond = conditionNumber(A);
        return cond <= threshold;
    } catch (...) {
        return false;
    }
}

double NumericalStability::safeAdd(double a, double b) {
    if (!std::isfinite(a) || !std::isfinite(b)) {
        if (std::isnan(a) || std::isnan(b)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (std::isinf(a) && std::isinf(b) && (a > 0) != (b > 0)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return std::isinf(a) ? a : b;
    }
    
    double result = a + b;
    
    if (std::abs(result) > SAFE_MAX) {
        return (result > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    return result;
}

double NumericalStability::safeSubtract(double a, double b) {
    return safeAdd(a, -b);
}

double NumericalStability::safeMultiply(double a, double b) {
    if (!std::isfinite(a) || !std::isfinite(b)) {
        if (std::isnan(a) || std::isnan(b)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return (a > 0) == (b > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    double absA = std::abs(a);
    double absB = std::abs(b);
    
    if (absA > 1.0 && absB > SAFE_MAX / absA) {
        return (a > 0) == (b > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    double result = a * b;
    
    if (std::abs(result) > SAFE_MAX) {
        return (result > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    return result;
}

double NumericalStability::safeExp(double x) {
    if (x > 709.0) {
        return SAFE_MAX;
    }
    if (x < -745.0) {
        return 0.0;
    }
    return std::exp(x);
}

double NumericalStability::safePow(double base, double exp) {
    if (base < 0 && !std::isfinite(exp) && std::floor(exp) != exp) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    double result = std::pow(base, exp);
    
    if (!std::isfinite(result)) {
        if (std::isnan(result)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return (result > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    if (std::abs(result) > SAFE_MAX) {
        return (result > 0) ? SAFE_MAX : -SAFE_MAX;
    }
    
    return result;
}

Matrix NumericalStability::safeSolve(const Matrix& A, const Matrix& b) {
    if (!A.isSquare()) {
        throw std::invalid_argument("Matrix A must be square for solve");
    }
    
    if (A.rows() != b.rows()) {
        throw std::invalid_argument("Matrix dimensions must agree for solve");
    }
    
    double cond = conditionNumber(A);
    if (cond > CONDITION_THRESHOLD) {
        throw std::runtime_error("Matrix is ill-conditioned (cond=" + 
                                std::to_string(cond) + "), solution may be inaccurate");
    }
    
    return Matrix::solve(A, b);
}

double NumericalStability::kahanSum(const double* data, size_t n) {
    double sum = 0.0;
    double c = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double y = data[i] - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}

double NumericalStability::compensatedDotProduct(const double* a, const double* b, size_t n) {
    double sum = 0.0;
    double c = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double product = safeMultiply(a[i], b[i]);
        double y = product - c;
        double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }
    
    return sum;
}

}
