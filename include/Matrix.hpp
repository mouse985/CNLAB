#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <complex>
#include <memory>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace cnlab {

enum class DataType {
    Double,
    Float,
    Int32,
    Int64,
    ComplexDouble,
    Bool
};

class Matrix {
public:
    Matrix();
    explicit Matrix(size_t rows, size_t cols, DataType type = DataType::Double);
    Matrix(const Matrix& other);
    Matrix(Matrix&& other) noexcept;
    ~Matrix();

    Matrix& operator=(const Matrix& other);
    Matrix& operator=(Matrix&& other) noexcept;

    static Matrix zeros(size_t rows, size_t cols, DataType type = DataType::Double);
    static Matrix ones(size_t rows, size_t cols, DataType type = DataType::Double);
    static Matrix eye(size_t n, DataType type = DataType::Double);
    static Matrix rand(size_t rows, size_t cols);
    static Matrix randn(size_t rows, size_t cols);
    static Matrix randi(int imax, size_t rows, size_t cols);
    static Matrix randi(int imin, int imax, size_t rows, size_t cols);
    static Matrix randperm(size_t n);
    static Matrix randperm(size_t n, size_t k);
    static Matrix magic(size_t n);
    static Matrix hilb(size_t n);
    static Matrix pascal(size_t n);
    static Matrix vander(const Matrix& v);
    static Matrix blkdiag(const Matrix& A, const Matrix& B);
    static Matrix kron(const Matrix& A, const Matrix& B);

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t size() const { return rows_ * cols_; }
    size_t length() const { return isVector() ? size() : std::max(rows_, cols_); }
    size_t numel() const { return size(); }
    bool isempty() const { return size() == 0; }
    DataType type() const { return type_; }
    bool isEmpty() const { return data_ == nullptr || size() == 0; }
    bool isVector() const { return rows_ == 1 || cols_ == 1; }
    bool isScalar() const { return rows_ == 1 && cols_ == 1; }
    bool isSquare() const { return rows_ == cols_; }

    double& at(size_t r, size_t c);
    const double& at(size_t r, size_t c) const;
    double& operator()(size_t r, size_t c);
    const double& operator()(size_t r, size_t c) const;
    double& operator()(size_t idx);
    const double& operator()(size_t idx) const;

    Matrix operator+(const Matrix& other) const;
    Matrix operator+(double scalar) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator-(double scalar) const;
    Matrix operator*(const Matrix& other) const;
    Matrix operator*(double scalar) const;
    Matrix operator/(double scalar) const;
    
    friend Matrix operator+(double scalar, const Matrix& mat);
    friend Matrix operator-(double scalar, const Matrix& mat);
    friend Matrix operator*(double scalar, const Matrix& mat);
    Matrix operator%(const Matrix& other) const;
    Matrix operator%(double scalar) const;
    Matrix operator-() const;

    Matrix& operator+=(const Matrix& other);
    Matrix& operator-=(const Matrix& other);
    Matrix& operator*=(double scalar);
    Matrix& operator/=(double scalar);
    Matrix& operator%=(const Matrix& other);
    Matrix& operator%=(double scalar);

    Matrix operator==(const Matrix& other) const;
    Matrix operator!=(const Matrix& other) const;
    Matrix operator>(const Matrix& other) const;
    Matrix operator<(const Matrix& other) const;
    Matrix operator>=(const Matrix& other) const;
    Matrix operator<=(const Matrix& other) const;

    Matrix operator&(const Matrix& other) const;
    Matrix operator|(const Matrix& other) const;
    Matrix operator!() const;

    Matrix transpose() const;
    Matrix ctranspose() const;
    Matrix inverse() const;
    double det() const;
    double norm() const;
    double cond() const;
    int rank() const;

    Matrix row(size_t r) const;
    Matrix col(size_t c) const;
    Matrix slice(size_t r1, size_t r2, size_t c1, size_t c2) const;
    void setRow(size_t r, const Matrix& vec);
    void setCol(size_t c, const Matrix& vec);

    Matrix reshape(size_t newRows, size_t newCols) const;
    Matrix flatten() const;
    Matrix repmat(size_t m, size_t n) const;

    double sum() const;
    double mean() const;
    double max() const;
    double min() const;
    double prod() const;
    double std() const;
    double var() const;

    Matrix sum(int dim) const;
    Matrix mean(int dim) const;
    Matrix max(int dim) const;
    Matrix min(int dim) const;
    Matrix prod(int dim) const;
    Matrix std(int dim) const;
    Matrix var(int dim) const;

    Matrix cumsum() const;
    Matrix cumsum(int dim) const;
    Matrix cumprod() const;
    Matrix cumprod(int dim) const;
    Matrix diff() const;
    Matrix diff(int n) const;

    Matrix sort() const;
    Matrix sort(int dim) const;
    Matrix sort(int dim, bool ascending) const;
    Matrix find() const;
    Matrix find(int k) const;
    Matrix unique() const;

    static Matrix repmat(const Matrix& A, size_t m, size_t n);
    Matrix flipud() const;
    Matrix fliplr() const;
    Matrix rot90() const;
    Matrix rot90(int k) const;

    Matrix abs() const;
    Matrix sqrt() const;
    Matrix exp() const;
    Matrix log() const;
    Matrix log10() const;
    Matrix log2() const;
    Matrix pow(double p) const;
    Matrix sin() const;
    Matrix cos() const;
    Matrix tan() const;
    Matrix asin() const;
    Matrix acos() const;
    Matrix atan() const;
    Matrix atan2(const Matrix& other) const;
    static Matrix atan2(const Matrix& y, const Matrix& x);
    Matrix sinh() const;
    Matrix cosh() const;
    Matrix tanh() const;
    Matrix asinh() const;
    Matrix acosh() const;
    Matrix atanh() const;
    Matrix sech() const;
    Matrix csch() const;
    Matrix coth() const;
    Matrix sec() const;
    Matrix csc() const;
    Matrix cot() const;
    Matrix asec() const;
    Matrix acsc() const;
    Matrix acot() const;
    Matrix floor() const;
    Matrix ceil() const;
    Matrix round() const;
    Matrix fix() const;
    Matrix sign() const;
    Matrix expm1() const;
    Matrix log1p() const;
    static Matrix hypot(const Matrix& x, const Matrix& y);
    static int nextpow2(int n);

    static Matrix solve(const Matrix& A, const Matrix& b);
    
    Matrix lu() const;
    Matrix qr() const;
    Matrix svd() const;
    Matrix eig() const;
    Matrix chol() const;

    Matrix diag(int k = 0) const;
    static Matrix diag(const Matrix& v, int k = 0);
    double trace() const;
    Matrix tril(int k = 0) const;
    Matrix triu(int k = 0) const;

    static Matrix horzcat(const Matrix& A, const Matrix& B);
    static Matrix vertcat(const Matrix& A, const Matrix& B);

    static std::pair<Matrix, Matrix> meshgrid(const Matrix& x, const Matrix& y);
    static std::pair<Matrix, Matrix> meshgrid(const Matrix& x);

    std::string toString() const;
    void print(const std::string& name = "") const;
    std::vector<double> toStdVector() const;

    double* data() { return static_cast<double*>(data_); }
    const double* data() const { return static_cast<const double*>(data_); }

private:
    size_t rows_;
    size_t cols_;
    DataType type_;
    void* data_;
    size_t elementSize_;

    void allocate();
    void deallocate();
    void copyFrom(const Matrix& other);
    void checkDimensions(const Matrix& other, const std::string& op) const;
    void checkSquare() const;
    void checkIndex(size_t r, size_t c) const;
    size_t index(size_t r, size_t c) const { return r * cols_ + c; }
};

Matrix operator*(double scalar, const Matrix& mat);

Matrix zeros(size_t rows, size_t cols);
Matrix ones(size_t rows, size_t cols);
Matrix eye(size_t n);
Matrix rand(size_t rows, size_t cols);
Matrix randn(size_t rows, size_t cols);

Matrix abs(const Matrix& m);
Matrix sqrt(const Matrix& m);
Matrix exp(const Matrix& m);
Matrix log(const Matrix& m);
Matrix log10(const Matrix& m);
Matrix pow(const Matrix& m, double p);
Matrix sin(const Matrix& m);
Matrix cos(const Matrix& m);
Matrix tan(const Matrix& m);
Matrix asin(const Matrix& m);
Matrix acos(const Matrix& m);
Matrix atan(const Matrix& m);
Matrix sinh(const Matrix& m);
Matrix cosh(const Matrix& m);
Matrix tanh(const Matrix& m);
Matrix asinh(const Matrix& m);
Matrix acosh(const Matrix& m);
Matrix atanh(const Matrix& m);
Matrix sech(const Matrix& m);
Matrix csch(const Matrix& m);
Matrix coth(const Matrix& m);
Matrix sec(const Matrix& m);
Matrix csc(const Matrix& m);
Matrix cot(const Matrix& m);
Matrix asec(const Matrix& m);
Matrix acsc(const Matrix& m);
Matrix acot(const Matrix& m);
Matrix floor(const Matrix& m);
Matrix ceil(const Matrix& m);
Matrix round(const Matrix& m);
Matrix fix(const Matrix& m);
Matrix sign(const Matrix& m);
Matrix log2(const Matrix& m);
Matrix expm1(const Matrix& m);
Matrix log1p(const Matrix& m);
Matrix hypot(const Matrix& x, const Matrix& y);
int nextpow2(int n);

Matrix solve(const Matrix& A, const Matrix& b);
Matrix lu(const Matrix& A);
Matrix qr(const Matrix& A);
Matrix svd(const Matrix& A);
Matrix eig(const Matrix& A);
Matrix chol(const Matrix& A);

}
