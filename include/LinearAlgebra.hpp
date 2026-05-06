#pragma once

#include "Matrix.hpp"
#include <vector>

namespace cnlab {

struct LUResult {
    Matrix L;
    Matrix U;
    std::vector<size_t> perm;
    int sign;
    
    LUResult(size_t n) : L(n, n), U(n, n), perm(n), sign(1) {}
};

struct QRResult {
    Matrix Q;
    Matrix R;
    
    QRResult(size_t m, size_t n) : Q(m, m), R(m, n) {}
};

struct SVDResult {
    Matrix U;
    Matrix S;
    Matrix V;
    
    SVDResult(size_t m, size_t n) : U(m, m), S(m, n), V(n, n) {}
};

struct EigResult {
    Matrix V;
    Matrix D;
    
    EigResult(size_t n) : V(n, n), D(n, n) {}
};

LUResult lu_decomposition(const Matrix& A);

QRResult qr_decomposition(const Matrix& A);

SVDResult svd_decomposition(const Matrix& A);

EigResult eig_decomposition(const Matrix& A);

Matrix chol_decomposition(const Matrix& A);

Matrix solve_lu(const LUResult& lu, const Matrix& b);

Matrix solve_qr(const QRResult& qr, const Matrix& b);

Matrix solve_svd(const SVDResult& svd, const Matrix& b);

Matrix solve_linear(const Matrix& A, const Matrix& b);

}
