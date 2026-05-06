#include "Matrix.hpp"
#include "LinearAlgebra.hpp"
#include "NumericalStability.hpp"
#include <cstring>
#include <cmath>
#include <random>
#include <limits>
#include <set>
#include <algorithm>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cnlab {

static size_t getElementSize(DataType type) {
    switch (type) {
        case DataType::Float: return sizeof(float);
        case DataType::Double: return sizeof(double);
        case DataType::Int32: return sizeof(int32_t);
        case DataType::Int64: return sizeof(int64_t);
        case DataType::ComplexDouble: return sizeof(std::complex<double>);
        case DataType::Bool: return sizeof(bool);
    }
    return sizeof(double);
}

Matrix::Matrix() : rows_(0), cols_(0), type_(DataType::Double), data_(nullptr), elementSize_(sizeof(double)) {}

Matrix::Matrix(size_t rows, size_t cols, DataType type) 
    : rows_(rows), cols_(cols), type_(type), data_(nullptr), elementSize_(getElementSize(type)) {
    allocate();
}

Matrix::Matrix(const Matrix& other) 
    : rows_(other.rows_), cols_(other.cols_), type_(other.type_), data_(nullptr), elementSize_(other.elementSize_) {
    allocate();
    copyFrom(other);
}

Matrix::Matrix(Matrix&& other) noexcept 
    : rows_(other.rows_), cols_(other.cols_), type_(other.type_), 
      data_(other.data_), elementSize_(other.elementSize_) {
    other.rows_ = 0;
    other.cols_ = 0;
    other.data_ = nullptr;
}

Matrix::~Matrix() {
    deallocate();
}

Matrix& Matrix::operator=(const Matrix& other) {
    if (this != &other) {
        deallocate();
        rows_ = other.rows_;
        cols_ = other.cols_;
        type_ = other.type_;
        elementSize_ = other.elementSize_;
        allocate();
        copyFrom(other);
    }
    return *this;
}

Matrix& Matrix::operator=(Matrix&& other) noexcept {
    if (this != &other) {
        deallocate();
        rows_ = other.rows_;
        cols_ = other.cols_;
        type_ = other.type_;
        elementSize_ = other.elementSize_;
        data_ = other.data_;
        other.rows_ = 0;
        other.cols_ = 0;
        other.data_ = nullptr;
    }
    return *this;
}

void Matrix::allocate() {
    if (size() > 0) {
        data_ = std::malloc(size() * elementSize_);
        if (!data_) {
            throw std::bad_alloc();
        }
        std::memset(data_, 0, size() * elementSize_);
    }
}

void Matrix::deallocate() {
    if (data_) {
        std::free(data_);
        data_ = nullptr;
    }
}

void Matrix::copyFrom(const Matrix& other) {
    if (data_ && other.data_ && size() == other.size()) {
        std::memcpy(data_, other.data_, size() * elementSize_);
    }
}

void Matrix::checkDimensions(const Matrix& other, const std::string& op) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::invalid_argument("Matrix dimensions must agree for " + op);
    }
}

void Matrix::checkSquare() const {
    if (!isSquare()) {
        throw std::invalid_argument("Matrix must be square");
    }
}

void Matrix::checkIndex(size_t r, size_t c) const {
    if (r >= rows_ || c >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
}

Matrix Matrix::zeros(size_t rows, size_t cols, DataType type) {
    return Matrix(rows, cols, type);
}

Matrix Matrix::ones(size_t rows, size_t cols, DataType type) {
    Matrix result(rows, cols, type);
    double* ptr = result.data();
    std::fill(ptr, ptr + result.size(), 1.0);
    return result;
}

Matrix Matrix::eye(size_t n, DataType type) {
    Matrix result(n, n, type);
    double* ptr = result.data();
    for (size_t i = 0; i < n; ++i) {
        ptr[i * n + i] = 1.0;
    }
    return result;
}

Matrix Matrix::rand(size_t rows, size_t cols) {
    Matrix result(rows, cols);
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double* ptr = result.data();
    for (size_t i = 0; i < result.size(); ++i) {
        ptr[i] = dist(gen);
    }
    return result;
}

Matrix Matrix::randn(size_t rows, size_t cols) {
    Matrix result(rows, cols);
    static std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> dist(0.0, 1.0);
    double* ptr = result.data();
    for (size_t i = 0; i < result.size(); ++i) {
        ptr[i] = dist(gen);
    }
    return result;
}

Matrix Matrix::randi(int imax, size_t rows, size_t cols) {
    return randi(1, imax, rows, cols);
}

Matrix Matrix::randi(int imin, int imax, size_t rows, size_t cols) {
    Matrix result(rows, cols);
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(imin, imax);
    double* ptr = result.data();
    for (size_t i = 0; i < result.size(); ++i) {
        ptr[i] = static_cast<double>(dist(gen));
    }
    return result;
}

Matrix Matrix::randperm(size_t n) {
    return randperm(n, n);
}

Matrix Matrix::randperm(size_t n, size_t k) {
    if (k > n) k = n;
    Matrix result(k, 1);
    std::vector<size_t> perm(n);
    for (size_t i = 0; i < n; ++i) perm[i] = i + 1;
    
    static std::mt19937 gen(std::random_device{}());
    for (size_t i = n - 1; i > 0; --i) {
        std::uniform_int_distribution<size_t> dist(0, i);
        size_t j = dist(gen);
        std::swap(perm[i], perm[j]);
    }
    
    for (size_t i = 0; i < k; ++i) {
        result(i, 0) = static_cast<double>(perm[i]);
    }
    return result;
}

double& Matrix::at(size_t r, size_t c) {
    checkIndex(r, c);
    return data()[index(r, c)];
}

const double& Matrix::at(size_t r, size_t c) const {
    checkIndex(r, c);
    return data()[index(r, c)];
}

double& Matrix::operator()(size_t r, size_t c) {
    return at(r, c);
}

const double& Matrix::operator()(size_t r, size_t c) const {
    return at(r, c);
}

double& Matrix::operator()(size_t idx) {
    if (idx >= size()) {
        throw std::out_of_range("Matrix linear index out of range");
    }
    return data()[idx];
}

const double& Matrix::operator()(size_t idx) const {
    if (idx >= size()) {
        throw std::out_of_range("Matrix linear index out of range");
    }
    return data()[idx];
}

Matrix Matrix::operator+(const Matrix& other) const {
    checkDimensions(other, "addition");
    Matrix result(rows_, cols_);
    const double* a = data();
    const double* b = other.data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = NumericalStability::safeAdd(a[i], b[i]);
    }
    return result;
}

Matrix Matrix::operator+(double scalar) const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = a[i] + scalar;
    }
    return result;
}

Matrix operator+(double scalar, const Matrix& mat) {
    return mat + scalar;
}

Matrix Matrix::operator-(const Matrix& other) const {
    checkDimensions(other, "subtraction");
    Matrix result(rows_, cols_);
    const double* a = data();
    const double* b = other.data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = NumericalStability::safeSubtract(a[i], b[i]);
    }
    return result;
}

Matrix Matrix::operator-(double scalar) const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = a[i] - scalar;
    }
    return result;
}

Matrix operator-(double scalar, const Matrix& mat) {
    Matrix result(mat.rows_, mat.cols_);
    const double* a = mat.data();
    double* c = result.data();
    for (size_t i = 0; i < mat.size(); ++i) {
        c[i] = scalar - a[i];
    }
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
    if (cols_ != other.rows_) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }
    
    constexpr size_t BLOCK = 64;
    
    Matrix result(rows_, other.cols_);
    const double* a = data();
    const double* b = other.data();
    double* c = result.data();
    
    std::memset(c, 0, rows_ * other.cols_ * sizeof(double));
    
    const size_t M = rows_;
    const size_t N = other.cols_;
    const size_t K = cols_;
    
    for (size_t i0 = 0; i0 < M; i0 += BLOCK) {
        size_t iMax = (i0 + BLOCK < M) ? i0 + BLOCK : M;
        
        for (size_t j0 = 0; j0 < N; j0 += BLOCK) {
            size_t jMax = (j0 + BLOCK < N) ? j0 + BLOCK : N;
            
            for (size_t k0 = 0; k0 < K; k0 += BLOCK) {
                size_t kMax = (k0 + BLOCK < K) ? k0 + BLOCK : K;
                
                for (size_t i = i0; i < iMax; ++i) {
                    for (size_t k = k0; k < kMax; ++k) {
                        double aik = a[i * K + k];
                        
                        for (size_t j = j0; j < jMax; ++j) {
                            c[i * N + j] = NumericalStability::safeAdd(c[i * N + j], 
                                NumericalStability::safeMultiply(aik, b[k * N + j]));
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

Matrix Matrix::operator*(double scalar) const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = a[i] * scalar;
    }
    return result;
}

Matrix operator*(double scalar, const Matrix& mat) {
    return mat * scalar;
}

Matrix Matrix::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Division by zero");
    }
    return operator*(1.0 / scalar);
}

Matrix Matrix::operator%(const Matrix& other) const {
    checkDimensions(other, "%");
    Matrix result(rows_, cols_);
    const double* a = data();
    const double* b = other.data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::fmod(a[i], b[i]);
    }
    return result;
}

Matrix Matrix::operator%(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Modulo by zero");
    }
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::fmod(a[i], scalar);
    }
    return result;
}

Matrix Matrix::operator-() const {
    return operator*(-1.0);
}

Matrix& Matrix::operator+=(const Matrix& other) {
    *this = *this + other;
    return *this;
}

Matrix& Matrix::operator-=(const Matrix& other) {
    *this = *this - other;
    return *this;
}

Matrix& Matrix::operator*=(double scalar) {
    *this = *this * scalar;
    return *this;
}

Matrix& Matrix::operator/=(double scalar) {
    *this = *this / scalar;
    return *this;
}

Matrix& Matrix::operator%=(const Matrix& other) {
    *this = *this % other;
    return *this;
}

Matrix& Matrix::operator%=(double scalar) {
    *this = *this % scalar;
    return *this;
}

Matrix Matrix::transpose() const {
    Matrix result(cols_, rows_);
    const double* a = data();
    double* c = result.data();
    
    constexpr size_t BLOCK = 64;
    
    for (size_t i0 = 0; i0 < rows_; i0 += BLOCK) {
        size_t iMax = (i0 + BLOCK < rows_) ? i0 + BLOCK : rows_;
        
        for (size_t j0 = 0; j0 < cols_; j0 += BLOCK) {
            size_t jMax = (j0 + BLOCK < cols_) ? j0 + BLOCK : cols_;
            
            for (size_t i = i0; i < iMax; ++i) {
                for (size_t j = j0; j < jMax; ++j) {
                    c[j * rows_ + i] = a[i * cols_ + j];
                }
            }
        }
    }
    
    return result;
}

Matrix Matrix::ctranspose() const {
    return transpose();
}

Matrix Matrix::inverse() const {
    checkSquare();
    size_t n = rows_;
    
    // Check condition number for numerical stability
    double cond = NumericalStability::conditionNumber(*this);
    if (cond > NumericalStability::CONDITION_THRESHOLD) {
        throw std::runtime_error("Matrix is ill-conditioned (cond=" + std::to_string(cond) + 
                                "), inverse may be numerically unstable");
    }
    
    Matrix A = *this;
    Matrix result = eye(n);
    
    double* a = A.data();
    double* r = result.data();
    
    for (size_t i = 0; i < n; ++i) {
        double pivot = a[i * n + i];
        if (std::abs(pivot) < 1e-10) {
            throw std::runtime_error("Matrix is singular");
        }
        
        for (size_t j = 0; j < n; ++j) {
            a[i * n + j] = NumericalStability::safeDivide(a[i * n + j], pivot);
            r[i * n + j] = NumericalStability::safeDivide(r[i * n + j], pivot);
        }
        
        for (size_t k = 0; k < n; ++k) {
            if (k != i) {
                double factor = a[k * n + i];
                for (size_t j = 0; j < n; ++j) {
                    a[k * n + j] = NumericalStability::safeSubtract(a[k * n + j], 
                        NumericalStability::safeMultiply(factor, a[i * n + j]));
                    r[k * n + j] = NumericalStability::safeSubtract(r[k * n + j], 
                        NumericalStability::safeMultiply(factor, r[i * n + j]));
                }
            }
        }
    }
    
    return result;
}

double Matrix::det() const {
    checkSquare();
    size_t n = rows_;
    if (n == 1) return data()[0];
    if (n == 2) return data()[0] * data()[3] - data()[1] * data()[2];
    
    Matrix A = *this;
    double* a = A.data();
    double det = 1.0;
    
    for (size_t i = 0; i < n; ++i) {
        double pivot = a[i * n + i];
        if (std::abs(pivot) < 1e-10) {
            return 0.0;
        }
        det *= pivot;
        
        for (size_t j = i + 1; j < n; ++j) {
            double factor = a[j * n + i] / pivot;
            for (size_t k = i; k < n; ++k) {
                a[j * n + k] -= factor * a[i * n + k];
            }
        }
    }
    
    return det;
}

double Matrix::norm() const {
    return std::sqrt(NumericalStability::compensatedDotProduct(data(), data(), size()));
}

double Matrix::cond() const {
    checkSquare();
    Matrix s = svd();
    double max_singular = s(0, 0);
    double min_singular = s(0, 0);
    for (size_t i = 1; i < s.size(); ++i) {
        if (s(i) < min_singular) min_singular = s(i);
        if (s(i) > max_singular) max_singular = s(i);
    }
    if (min_singular == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return max_singular / min_singular;
}

int Matrix::rank() const {
    double tol = std::max(rows_, cols_) * std::numeric_limits<double>::epsilon();
    Matrix s = svd();
    int r = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s(i) > tol) {
            r++;
        }
    }
    return r;
}

Matrix Matrix::row(size_t r) const {
    if (r >= rows_) {
        throw std::out_of_range("Row index out of range");
    }
    Matrix result(1, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t j = 0; j < cols_; ++j) {
        c[j] = a[r * cols_ + j];
    }
    return result;
}

Matrix Matrix::col(size_t c) const {
    if (c >= cols_) {
        throw std::out_of_range("Column index out of range");
    }
    Matrix result(rows_, 1);
    const double* a = data();
    double* r = result.data();
    for (size_t i = 0; i < rows_; ++i) {
        r[i] = a[i * cols_ + c];
    }
    return result;
}

Matrix Matrix::reshape(size_t newRows, size_t newCols) const {
    if (newRows * newCols != size()) {
        throw std::invalid_argument("Reshape dimensions must match total size");
    }
    Matrix result = *this;
    result.rows_ = newRows;
    result.cols_ = newCols;
    return result;
}

Matrix Matrix::flatten() const {
    return reshape(1, size());
}

Matrix Matrix::repmat(size_t m, size_t n) const {
    Matrix result(rows_ * m, cols_ * n);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t r = 0; r < rows_; ++r) {
                for (size_t co = 0; co < cols_; ++co) {
                    c[(i * rows_ + r) * result.cols_ + j * cols_ + co] = a[r * cols_ + co];
                }
            }
        }
    }
    return result;
}

double Matrix::sum() const {
    return NumericalStability::kahanSum(data(), size());
}

double Matrix::mean() const {
    return sum() / static_cast<double>(size());
}

double Matrix::max() const {
    if (isEmpty()) {
        throw std::runtime_error("Cannot find max of empty matrix");
    }
    const double* a = data();
    return *std::max_element(a, a + size());
}

double Matrix::min() const {
    if (isEmpty()) {
        throw std::runtime_error("Cannot find min of empty matrix");
    }
    const double* a = data();
    return *std::min_element(a, a + size());
}

double Matrix::prod() const {
    const double* a = data();
    double result = 1.0;
    for (size_t i = 0; i < size(); ++i) {
        result *= a[i];
    }
    return result;
}

Matrix Matrix::sum(int dim) const {
    if (dim == 1) {
        Matrix result(1, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double s = 0.0;
            for (size_t i = 0; i < rows_; ++i) {
                s += at(i, j);
            }
            result(0, j) = s;
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, 1);
        for (size_t i = 0; i < rows_; ++i) {
            double s = 0.0;
            for (size_t j = 0; j < cols_; ++j) {
                s += at(i, j);
            }
            result(i, 0) = s;
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::mean(int dim) const {
    if (dim == 1) {
        Matrix result = sum(1);
        for (size_t j = 0; j < cols_; ++j) {
            result(0, j) /= rows_;
        }
        return result;
    } else if (dim == 2) {
        Matrix result = sum(2);
        for (size_t i = 0; i < rows_; ++i) {
            result(i, 0) /= cols_;
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::max(int dim) const {
    if (dim == 1) {
        Matrix result(1, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double m = at(0, j);
            for (size_t i = 1; i < rows_; ++i) {
                if (at(i, j) > m) m = at(i, j);
            }
            result(0, j) = m;
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, 1);
        for (size_t i = 0; i < rows_; ++i) {
            double m = at(i, 0);
            for (size_t j = 1; j < cols_; ++j) {
                if (at(i, j) > m) m = at(i, j);
            }
            result(i, 0) = m;
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::min(int dim) const {
    if (dim == 1) {
        Matrix result(1, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double m = at(0, j);
            for (size_t i = 1; i < rows_; ++i) {
                if (at(i, j) < m) m = at(i, j);
            }
            result(0, j) = m;
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, 1);
        for (size_t i = 0; i < rows_; ++i) {
            double m = at(i, 0);
            for (size_t j = 1; j < cols_; ++j) {
                if (at(i, j) < m) m = at(i, j);
            }
            result(i, 0) = m;
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::prod(int dim) const {
    if (dim == 1) {
        Matrix result(1, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double p = 1.0;
            for (size_t i = 0; i < rows_; ++i) {
                p *= at(i, j);
            }
            result(0, j) = p;
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, 1);
        for (size_t i = 0; i < rows_; ++i) {
            double p = 1.0;
            for (size_t j = 0; j < cols_; ++j) {
                p *= at(i, j);
            }
            result(i, 0) = p;
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

double Matrix::var() const {
    if (size() < 2) {
        throw std::runtime_error("Cannot compute variance of matrix with less than 2 elements");
    }
    double m = mean();
    const double* a = data();
    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < size(); ++i) {
        double diff = a[i] - m;
        sum_sq_diff += diff * diff;
    }
    return sum_sq_diff / (size() - 1);  // 样本方差 (n-1)
}

double Matrix::std() const {
    return std::sqrt(var());
}

Matrix Matrix::var(int dim) const {
    if (dim == 1) {
        Matrix result(1, cols_);
        Matrix m = mean(1);
        for (size_t j = 0; j < cols_; ++j) {
            double sum_sq_diff = 0.0;
            for (size_t i = 0; i < rows_; ++i) {
                double diff = at(i, j) - m(0, j);
                sum_sq_diff += diff * diff;
            }
            result(0, j) = sum_sq_diff / (rows_ - 1);
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, 1);
        Matrix m = mean(2);
        for (size_t i = 0; i < rows_; ++i) {
            double sum_sq_diff = 0.0;
            for (size_t j = 0; j < cols_; ++j) {
                double diff = at(i, j) - m(i, 0);
                sum_sq_diff += diff * diff;
            }
            result(i, 0) = sum_sq_diff / (cols_ - 1);
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::std(int dim) const {
    Matrix var_result = var(dim);
    for (size_t i = 0; i < var_result.rows(); ++i) {
        for (size_t j = 0; j < var_result.cols(); ++j) {
            var_result(i, j) = std::sqrt(var_result(i, j));
        }
    }
    return var_result;
}

Matrix Matrix::cumsum() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    double sum = 0.0;
    for (size_t i = 0; i < size(); ++i) {
        sum += a[i];
        c[i] = sum;
    }
    return result;
}

Matrix Matrix::cumsum(int dim) const {
    if (dim == 1) {
        Matrix result(rows_, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double sum = 0.0;
            for (size_t i = 0; i < rows_; ++i) {
                sum += at(i, j);
                result(i, j) = sum;
            }
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < cols_; ++j) {
                sum += at(i, j);
                result(i, j) = sum;
            }
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::cumprod() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    double prod = 1.0;
    for (size_t i = 0; i < size(); ++i) {
        prod *= a[i];
        c[i] = prod;
    }
    return result;
}

Matrix Matrix::cumprod(int dim) const {
    if (dim == 1) {
        Matrix result(rows_, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            double prod = 1.0;
            for (size_t i = 0; i < rows_; ++i) {
                prod *= at(i, j);
                result(i, j) = prod;
            }
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i) {
            double prod = 1.0;
            for (size_t j = 0; j < cols_; ++j) {
                prod *= at(i, j);
                result(i, j) = prod;
            }
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::diff() const {
    return diff(1);
}

Matrix Matrix::diff(int n) const {
    if (n <= 0) {
        throw std::invalid_argument("n must be positive");
    }
    if (isVector()) {
        if (size() <= static_cast<size_t>(n)) {
            return Matrix(0, 0);
        }
        size_t result_size = size() - n;
        Matrix result(1, result_size);
        Matrix temp = *this;
        for (int d = 0; d < n; ++d) {
            size_t current_size = temp.size() - 1;
            Matrix next(1, current_size);
            for (size_t i = 0; i < current_size; ++i) {
                next(0, i) = temp(0, i + 1) - temp(0, i);
            }
            temp = next;
        }
        return temp;
    } else {
        throw std::runtime_error("diff currently only supports vectors");
    }
}

Matrix Matrix::abs() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::abs(a[i]);
    }
    return result;
}

Matrix Matrix::sqrt() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::sqrt(a[i]);
    }
    return result;
}

Matrix Matrix::exp() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::exp(a[i]);
    }
    return result;
}

Matrix Matrix::log() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::log(a[i]);
    }
    return result;
}

Matrix Matrix::log10() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::log10(a[i]);
    }
    return result;
}

Matrix Matrix::pow(double p) const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::pow(a[i], p);
    }
    return result;
}

Matrix Matrix::sin() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::sin(a[i]);
    }
    return result;
}

Matrix Matrix::cos() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::cos(a[i]);
    }
    return result;
}

Matrix Matrix::tan() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::tan(a[i]);
    }
    return result;
}

Matrix Matrix::asin() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::asin(a[i]);
    }
    return result;
}

Matrix Matrix::acos() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::acos(a[i]);
    }
    return result;
}

Matrix Matrix::atan() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::atan(a[i]);
    }
    return result;
}

Matrix Matrix::sinh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::sinh(a[i]);
    }
    return result;
}

Matrix Matrix::cosh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::cosh(a[i]);
    }
    return result;
}

Matrix Matrix::tanh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::tanh(a[i]);
    }
    return result;
}

Matrix Matrix::asinh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::asinh(a[i]);
    }
    return result;
}

Matrix Matrix::acosh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::acosh(a[i]);
    }
    return result;
}

Matrix Matrix::atanh() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::atanh(a[i]);
    }
    return result;
}

Matrix Matrix::sech() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = 1.0 / std::cosh(a[i]);
    }
    return result;
}

Matrix Matrix::csch() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = 1.0 / std::sinh(a[i]);
    }
    return result;
}

Matrix Matrix::coth() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::cosh(a[i]) / std::sinh(a[i]);
    }
    return result;
}

Matrix Matrix::sec() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = 1.0 / std::cos(a[i]);
    }
    return result;
}

Matrix Matrix::csc() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = 1.0 / std::sin(a[i]);
    }
    return result;
}

Matrix Matrix::cot() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::cos(a[i]) / std::sin(a[i]);
    }
    return result;
}

Matrix Matrix::asec() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::acos(1.0 / a[i]);
    }
    return result;
}

Matrix Matrix::acsc() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::asin(1.0 / a[i]);
    }
    return result;
}

Matrix Matrix::acot() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = M_PI / 2.0 - std::atan(a[i]);
    }
    return result;
}

Matrix Matrix::log2() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::log2(a[i]);
    }
    return result;
}

Matrix Matrix::atan2(const Matrix& other) const {
    checkDimensions(other, "atan2");
    Matrix result(rows_, cols_);
    const double* a = data();
    const double* b = other.data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::atan2(a[i], b[i]);
    }
    return result;
}

Matrix Matrix::atan2(const Matrix& y, const Matrix& x) {
    y.checkDimensions(x, "atan2");
    Matrix result(y.rows_, y.cols_);
    const double* py = y.data();
    const double* px = x.data();
    double* c = result.data();
    for (size_t i = 0; i < y.size(); ++i) {
        c[i] = std::atan2(py[i], px[i]);
    }
    return result;
}

Matrix Matrix::floor() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::floor(a[i]);
    }
    return result;
}

Matrix Matrix::ceil() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::ceil(a[i]);
    }
    return result;
}

Matrix Matrix::round() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::round(a[i]);
    }
    return result;
}

Matrix Matrix::fix() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = (a[i] >= 0) ? std::floor(a[i]) : std::ceil(a[i]);
    }
    return result;
}

Matrix Matrix::sign() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        if (a[i] > 0) c[i] = 1.0;
        else if (a[i] < 0) c[i] = -1.0;
        else c[i] = 0.0;
    }
    return result;
}

Matrix Matrix::expm1() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::expm1(a[i]);
    }
    return result;
}

Matrix Matrix::log1p() const {
    Matrix result(rows_, cols_);
    const double* a = data();
    double* c = result.data();
    for (size_t i = 0; i < size(); ++i) {
        c[i] = std::log1p(a[i]);
    }
    return result;
}

Matrix Matrix::hypot(const Matrix& x, const Matrix& y) {
    x.checkDimensions(y, "hypot");
    Matrix result(x.rows_, x.cols_);
    const double* px = x.data();
    const double* py = y.data();
    double* c = result.data();
    for (size_t i = 0; i < x.size(); ++i) {
        c[i] = std::hypot(px[i], py[i]);
    }
    return result;
}

int Matrix::nextpow2(int n) {
    if (n <= 0) return 0;
    int p = 0;
    while ((1 << p) < n) {
        ++p;
    }
    return p;
}

std::string Matrix::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "[";
    for (size_t i = 0; i < rows_; ++i) {
        if (i > 0) oss << " ";
        for (size_t j = 0; j < cols_; ++j) {
            oss << std::setw(10) << at(i, j);
            if (j < cols_ - 1) oss << ", ";
        }
        if (i < rows_ - 1) oss << ";\n";
    }
    oss << "]";
    return oss.str();
}

void Matrix::print(const std::string& name) const {
    if (!name.empty()) {
        std::cout << name << " =\n";
    }
    std::cout << toString() << std::endl;
}

std::vector<double> Matrix::toStdVector() const {
    std::vector<double> result;
    result.reserve(size());
    for (size_t i = 0; i < size(); ++i) {
        result.push_back((*this)(i));
    }
    return result;
}

Matrix Matrix::solve(const Matrix& A, const Matrix& b) {
    return solve_linear(A, b);
}

Matrix Matrix::lu() const {
    LUResult result = lu_decomposition(*this);
    
    size_t n = rows_;
    Matrix combined(2 * n + 1, n);
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            combined(i, j) = result.L(i, j);
            combined(i + n, j) = result.U(i, j);
        }
        combined(2 * n, i) = static_cast<double>(result.perm[i] + 1);
    }
    
    return combined;
}

Matrix Matrix::qr() const {
    QRResult result = qr_decomposition(*this);
    
    size_t m = rows_;
    size_t n = cols_;
    Matrix combined(m + n, m);
    
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < m; ++j) {
            combined(i, j) = result.Q(i, j);
        }
    }
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            combined(m + i, j) = result.R(j, i);
        }
    }
    
    return combined;
}

Matrix Matrix::svd() const {
    SVDResult result = svd_decomposition(*this);
    
    size_t m = rows_;
    size_t n = cols_;
    Matrix combined(m + n + std::min(m, n), std::max(m, n));
    
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < m; ++j) {
            combined(i, j) = result.U(i, j);
        }
    }
    
    for (size_t i = 0; i < std::min(m, n); ++i) {
        combined(m, i) = result.S(i, i);
    }
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            combined(m + std::min(m, n) + i, j) = result.V(i, j);
        }
    }
    
    return combined;
}

Matrix Matrix::eig() const {
    EigResult result = eig_decomposition(*this);
    size_t n = rows_;
    Matrix combined(2 * n, n);
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            combined(i, j) = result.V(i, j);
            combined(i + n, j) = result.D(i, j);
        }
    }
    
    return combined;
}

Matrix Matrix::chol() const {
    return chol_decomposition(*this);
}

Matrix Matrix::sort() const {
    return sort(1, true);
}

Matrix Matrix::sort(int dim) const {
    return sort(dim, true);
}

Matrix Matrix::sort(int dim, bool ascending) const {
    if (isVector()) {
        std::vector<double> vec(size());
        const double* a = data();
        for (size_t i = 0; i < size(); ++i) {
            vec[i] = a[i];
        }
        if (ascending) {
            std::sort(vec.begin(), vec.end());
        } else {
            std::sort(vec.begin(), vec.end(), std::greater<double>());
        }
        Matrix result(1, size());
        for (size_t i = 0; i < size(); ++i) {
            result(0, i) = vec[i];
        }
        return result;
    } else if (dim == 1) {
        Matrix result(rows_, cols_);
        for (size_t j = 0; j < cols_; ++j) {
            std::vector<double> col(rows_);
            for (size_t i = 0; i < rows_; ++i) {
                col[i] = at(i, j);
            }
            if (ascending) {
                std::sort(col.begin(), col.end());
            } else {
                std::sort(col.begin(), col.end(), std::greater<double>());
            }
            for (size_t i = 0; i < rows_; ++i) {
                result(i, j) = col[i];
            }
        }
        return result;
    } else if (dim == 2) {
        Matrix result(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i) {
            std::vector<double> row(cols_);
            for (size_t j = 0; j < cols_; ++j) {
                row[j] = at(i, j);
            }
            if (ascending) {
                std::sort(row.begin(), row.end());
            } else {
                std::sort(row.begin(), row.end(), std::greater<double>());
            }
            for (size_t j = 0; j < cols_; ++j) {
                result(i, j) = row[j];
            }
        }
        return result;
    }
    throw std::invalid_argument("dim must be 1 or 2");
}

Matrix Matrix::find() const {
    return find(static_cast<int>(size()));
}

Matrix Matrix::find(int k) const {
    std::vector<double> indices;
    for (size_t i = 0; i < size() && static_cast<int>(indices.size()) < k; ++i) {
        if ((*this)(i) != 0.0) {
            indices.push_back(static_cast<double>(i + 1));  // 1-based index
        }
    }
    Matrix result(1, indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        result(0, i) = indices[i];
    }
    return result;
}

Matrix Matrix::unique() const {
    std::set<double> unique_vals;
    const double* a = data();
    for (size_t i = 0; i < size(); ++i) {
        unique_vals.insert(a[i]);
    }
    Matrix result(1, unique_vals.size());
    size_t i = 0;
    for (double val : unique_vals) {
        result(0, i++) = val;
    }
    return result;
}

Matrix Matrix::repmat(const Matrix& A, size_t m, size_t n) {
    Matrix result(A.rows_ * m, A.cols_ * n);
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t r = 0; r < A.rows_; ++r) {
                for (size_t c = 0; c < A.cols_; ++c) {
                    result(i * A.rows_ + r, j * A.cols_ + c) = A(r, c);
                }
            }
        }
    }
    return result;
}

Matrix Matrix::flipud() const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result(i, j) = at(rows_ - 1 - i, j);
        }
    }
    return result;
}

Matrix Matrix::fliplr() const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result(i, j) = at(i, cols_ - 1 - j);
        }
    }
    return result;
}

Matrix Matrix::rot90() const {
    return rot90(1);
}

Matrix Matrix::rot90(int k) const {
    k = ((k % 4) + 4) % 4;  // Normalize to 0-3
    if (k == 0) return *this;
    
    Matrix result;
    if (k == 1) {
        // 90 degrees counterclockwise: (i,j) -> (cols-1-j, i)
        result = Matrix(cols_, rows_);
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                result(cols_ - 1 - j, i) = at(i, j);
            }
        }
    } else if (k == 2) {
        // 180 degrees
        result = Matrix(rows_, cols_);
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                result(rows_ - 1 - i, cols_ - 1 - j) = at(i, j);
            }
        }
    } else if (k == 3) {
        // 270 degrees counterclockwise
        result = Matrix(cols_, rows_);
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                result(j, rows_ - 1 - i) = at(i, j);
            }
        }
    }
    return result;
}

Matrix Matrix::diag(int k) const {
    if (isVector()) {
        size_t n = size();
        size_t dim = n + std::abs(k);
        Matrix result(dim, dim);
        result = Matrix::zeros(dim, dim);
        size_t startRow = (k >= 0) ? 0 : -k;
        size_t startCol = (k >= 0) ? k : 0;
        for (size_t i = 0; i < n; ++i) {
            result(startRow + i, startCol + i) = (*this)(i);
        }
        return result;
    } else {
        size_t n = std::min(rows_, cols_);
        std::vector<double> diagElements;
        for (size_t i = 0; i < rows_; ++i) {
            size_t j = (k >= 0) ? i + k : i - (-k);
            if (j < cols_) {
                diagElements.push_back(at(i, j));
            }
        }
        Matrix result(1, diagElements.size());
        for (size_t i = 0; i < diagElements.size(); ++i) {
            result(0, i) = diagElements[i];
        }
        return result;
    }
}

Matrix Matrix::diag(const Matrix& v, int k) {
    return v.diag(k);
}

double Matrix::trace() const {
    checkSquare();
    double sum = 0.0;
    for (size_t i = 0; i < rows_; ++i) {
        sum += at(i, i);
    }
    return sum;
}

Matrix Matrix::tril(int k) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            if (j <= i + k) {
                result(i, j) = at(i, j);
            } else {
                result(i, j) = 0.0;
            }
        }
    }
    return result;
}

Matrix Matrix::triu(int k) const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            if (j >= i + k) {
                result(i, j) = at(i, j);
            } else {
                result(i, j) = 0.0;
            }
        }
    }
    return result;
}

Matrix Matrix::horzcat(const Matrix& A, const Matrix& B) {
    if (A.rows_ != B.rows_) {
        throw std::invalid_argument("Matrix dimensions must agree for horizontal concatenation");
    }
    Matrix result(A.rows_, A.cols_ + B.cols_);
    for (size_t i = 0; i < A.rows_; ++i) {
        for (size_t j = 0; j < A.cols_; ++j) {
            result(i, j) = A(i, j);
        }
        for (size_t j = 0; j < B.cols_; ++j) {
            result(i, A.cols_ + j) = B(i, j);
        }
    }
    return result;
}

Matrix Matrix::vertcat(const Matrix& A, const Matrix& B) {
    if (A.cols_ != B.cols_) {
        throw std::invalid_argument("Matrix dimensions must agree for vertical concatenation");
    }
    Matrix result(A.rows_ + B.rows_, A.cols_);
    for (size_t j = 0; j < A.cols_; ++j) {
        for (size_t i = 0; i < A.rows_; ++i) {
            result(i, j) = A(i, j);
        }
        for (size_t i = 0; i < B.rows_; ++i) {
            result(A.rows_ + i, j) = B(i, j);
        }
    }
    return result;
}

Matrix solve(const Matrix& A, const Matrix& b) {
    return Matrix::solve(A, b);
}

Matrix zeros(size_t rows, size_t cols) {
    return Matrix::zeros(rows, cols);
}

Matrix ones(size_t rows, size_t cols) {
    return Matrix::ones(rows, cols);
}

Matrix eye(size_t n) {
    return Matrix::eye(n);
}

Matrix rand(size_t rows, size_t cols) {
    return Matrix::rand(rows, cols);
}

Matrix randn(size_t rows, size_t cols) {
    return Matrix::randn(rows, cols);
}

std::pair<Matrix, Matrix> Matrix::meshgrid(const Matrix& x, const Matrix& y) {
    // x should be a row vector (1×n or n×1), y should be a column vector (m×1 or 1×m)
    size_t nx = x.isVector() ? x.size() : x.cols();
    size_t ny = y.isVector() ? y.size() : y.rows();

    Matrix X(ny, nx);
    Matrix Y(ny, nx);

    for (size_t i = 0; i < ny; ++i) {
        for (size_t j = 0; j < nx; ++j) {
            X(i, j) = x(j);
            Y(i, j) = y(i);
        }
    }

    return {X, Y};
}

std::pair<Matrix, Matrix> Matrix::meshgrid(const Matrix& x) {
    return meshgrid(x, x);
}

Matrix Matrix::magic(size_t n) {
    if (n == 0) return Matrix(0, 0);
    if (n == 1) return Matrix::ones(1, 1);
    if (n == 2) return Matrix::eye(2);  // No 2x2 magic square exists
    
    Matrix result(n, n);
    
    if (n % 2 == 1) {
        // Odd order: Siamese method
        size_t i = 0, j = n / 2;
        for (size_t num = 1; num <= n * n; ++num) {
            result(i, j) = static_cast<double>(num);
            size_t ni = (i + n - 1) % n;
            size_t nj = (j + 1) % n;
            if (result(ni, nj) != 0) {
                i = (i + 1) % n;
            } else {
                i = ni;
                j = nj;
            }
        }
    } else if (n % 4 == 0) {
        // Doubly even order: 4k
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                size_t num = i * n + j + 1;
                if ((i % 4 == j % 4) || ((i % 4 + j % 4) == 3)) {
                    result(i, j) = static_cast<double>(n * n - num + 1);
                } else {
                    result(i, j) = static_cast<double>(num);
                }
            }
        }
    } else {
        // Singly even order: 4k+2 (Strachey method simplified)
        size_t k = n / 2;
        Matrix A = magic(k);
        Matrix B = A + static_cast<double>(k * k);
        Matrix C = A + static_cast<double>(2 * k * k);
        Matrix D = A + static_cast<double>(3 * k * k);
        
        // Combine quadrants
        for (size_t i = 0; i < k; ++i) {
            for (size_t j = 0; j < k; ++j) {
                result(i, j) = A(i, j);
                result(i, j + k) = C(i, j);
                result(i + k, j) = D(i, j);
                result(i + k, j + k) = B(i, j);
            }
        }
        
        // Swap columns to make it magic
        size_t m = k / 2;
        for (size_t i = 0; i < k; ++i) {
            for (size_t j = 0; j < m; ++j) {
                std::swap(result(i, j), result(i + k, j));
            }
        }
        std::swap(result(0, 0), result(k, 0));
        std::swap(result(0, m), result(k, m));
    }
    
    return result;
}

Matrix Matrix::hilb(size_t n) {
    Matrix result(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            result(i, j) = 1.0 / static_cast<double>(i + j + 1);
        }
    }
    return result;
}

Matrix Matrix::pascal(size_t n) {
    Matrix result(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            // Pascal matrix element: C(i+j, i) = C(i+j, j)
            double val = 1.0;
            size_t k = std::min(i, j);
            for (size_t m = 1; m <= k; ++m) {
                val = val * static_cast<double>(i + j + 1 - m) / static_cast<double>(m);
            }
            result(i, j) = val;
        }
    }
    return result;
}

Matrix Matrix::vander(const Matrix& v) {
    size_t n = v.isVector() ? v.size() : v.cols();
    Matrix result(n, n);
    for (size_t i = 0; i < n; ++i) {
        double vi = v(i);
        for (size_t j = 0; j < n; ++j) {
            result(i, j) = std::pow(vi, static_cast<double>(n - j - 1));
        }
    }
    return result;
}

Matrix Matrix::blkdiag(const Matrix& A, const Matrix& B) {
    size_t totalRows = A.rows() + B.rows();
    size_t totalCols = A.cols() + B.cols();
    Matrix result = Matrix::zeros(totalRows, totalCols);
    
    // Copy A to top-left
    for (size_t i = 0; i < A.rows(); ++i) {
        for (size_t j = 0; j < A.cols(); ++j) {
            result(i, j) = A(i, j);
        }
    }
    
    // Copy B to bottom-right
    for (size_t i = 0; i < B.rows(); ++i) {
        for (size_t j = 0; j < B.cols(); ++j) {
            result(i + A.rows(), j + A.cols()) = B(i, j);
        }
    }
    
    return result;
}

Matrix Matrix::kron(const Matrix& A, const Matrix& B) {
    size_t resultRows = A.rows() * B.rows();
    size_t resultCols = A.cols() * B.cols();
    Matrix result(resultRows, resultCols);
    
    for (size_t i = 0; i < A.rows(); ++i) {
        for (size_t j = 0; j < A.cols(); ++j) {
            double aij = A(i, j);
            for (size_t bi = 0; bi < B.rows(); ++bi) {
                for (size_t bj = 0; bj < B.cols(); ++bj) {
                    result(i * B.rows() + bi, j * B.cols() + bj) = aij * B(bi, bj);
                }
            }
        }
    }
    
    return result;
}

#define UNARY_FUNC_IMPL(func) \
    Matrix func(const Matrix& m) { return m.func(); }

UNARY_FUNC_IMPL(abs)
UNARY_FUNC_IMPL(sqrt)
UNARY_FUNC_IMPL(exp)
UNARY_FUNC_IMPL(log)
UNARY_FUNC_IMPL(log10)
UNARY_FUNC_IMPL(log2)
UNARY_FUNC_IMPL(sin)
UNARY_FUNC_IMPL(cos)
UNARY_FUNC_IMPL(tan)
UNARY_FUNC_IMPL(asin)
UNARY_FUNC_IMPL(acos)
UNARY_FUNC_IMPL(atan)
UNARY_FUNC_IMPL(sinh)
UNARY_FUNC_IMPL(cosh)
UNARY_FUNC_IMPL(tanh)
UNARY_FUNC_IMPL(asinh)
UNARY_FUNC_IMPL(acosh)
UNARY_FUNC_IMPL(atanh)
UNARY_FUNC_IMPL(sech)
UNARY_FUNC_IMPL(csch)
UNARY_FUNC_IMPL(coth)
UNARY_FUNC_IMPL(sec)
UNARY_FUNC_IMPL(csc)
UNARY_FUNC_IMPL(cot)
UNARY_FUNC_IMPL(asec)
UNARY_FUNC_IMPL(acsc)
UNARY_FUNC_IMPL(acot)
UNARY_FUNC_IMPL(floor)
UNARY_FUNC_IMPL(ceil)
UNARY_FUNC_IMPL(round)
UNARY_FUNC_IMPL(fix)
UNARY_FUNC_IMPL(sign)
UNARY_FUNC_IMPL(expm1)
UNARY_FUNC_IMPL(log1p)
UNARY_FUNC_IMPL(lu)
UNARY_FUNC_IMPL(qr)
UNARY_FUNC_IMPL(svd)
UNARY_FUNC_IMPL(eig)
UNARY_FUNC_IMPL(chol)

Matrix pow(const Matrix& m, double p) {
    return m.pow(p);
}

Matrix hypot(const Matrix& x, const Matrix& y) {
    return Matrix::hypot(x, y);
}

int nextpow2(int n) {
    return Matrix::nextpow2(n);
}

}
