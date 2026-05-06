#include "LinearAlgebra.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace cnlab;

void test_lu_decomposition() {
    std::cout << "Testing LU decomposition..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 2; A(0, 1) = 1; A(0, 2) = 1;
    A(1, 0) = 4; A(1, 1) = 3; A(1, 2) = 3;
    A(2, 0) = 8; A(2, 1) = 7; A(2, 2) = 9;
    
    LUResult lu = lu_decomposition(A);
    
    Matrix L = lu.L;
    Matrix U = lu.U;
    
    Matrix LU = L * U;
    
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            double expected = A(lu.perm[i], j);
            assert(std::abs(LU(i, j) - expected) < 1e-10);
        }
    }
    
    std::cout << "  LU decomposition OK" << std::endl;
}

void test_lu_solve() {
    std::cout << "Testing LU solve..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 2; A(0, 1) = 1; A(0, 2) = -1;
    A(1, 0) = -3; A(1, 1) = -1; A(1, 2) = 2;
    A(2, 0) = -2; A(2, 1) = 1; A(2, 2) = 2;
    
    Matrix b(3, 1);
    b(0, 0) = 8;
    b(1, 0) = -11;
    b(2, 0) = -3;
    
    LUResult lu = lu_decomposition(A);
    Matrix x = solve_lu(lu, b);
    
    Matrix Ax = A * x;
    
    for (size_t i = 0; i < 3; ++i) {
        assert(std::abs(Ax(i, 0) - b(i, 0)) < 1e-10);
    }
    
    std::cout << "  LU solve OK" << std::endl;
}

void test_qr_decomposition() {
    std::cout << "Testing QR decomposition..." << std::endl;
    
    Matrix A(3, 2);
    A(0, 0) = 1; A(0, 1) = -1;
    A(1, 0) = 1; A(1, 1) = 0;
    A(2, 0) = 1; A(2, 1) = 1;
    
    QRResult qr = qr_decomposition(A);
    
    Matrix Q = qr.Q;
    Matrix R = qr.R;
    
    Matrix QR = Q * R;
    
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            assert(std::abs(QR(i, j) - A(i, j)) < 1e-10);
        }
    }
    
    Matrix QtQ = Q.transpose() * Q;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            double expected = (i == j) ? 1.0 : 0.0;
            assert(std::abs(QtQ(i, j) - expected) < 1e-10);
        }
    }
    
    std::cout << "  QR decomposition OK" << std::endl;
}

void test_qr_solve() {
    std::cout << "Testing QR solve..." << std::endl;
    
    Matrix A(3, 2);
    A(0, 0) = 1; A(0, 1) = 1;
    A(1, 0) = 1; A(1, 1) = 2;
    A(2, 0) = 1; A(2, 1) = 3;
    
    Matrix b(3, 1);
    b(0, 0) = 2;
    b(1, 0) = 3;
    b(2, 0) = 4;
    
    QRResult qr = qr_decomposition(A);
    Matrix x = solve_qr(qr, b);
    
    assert(x.rows() == 2);
    assert(x.cols() == 1);
    
    std::cout << "  QR solve OK" << std::endl;
}

void test_svd_decomposition() {
    std::cout << "Testing SVD decomposition..." << std::endl;
    
    Matrix A(2, 2);
    A(0, 0) = 4; A(0, 1) = 0;
    A(1, 0) = 0; A(1, 1) = 3;
    
    SVDResult svd = svd_decomposition(A);
    
    Matrix U = svd.U;
    Matrix S = svd.S;
    Matrix V = svd.V;
    
    Matrix USV = U * S * V.transpose();
    
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            assert(std::abs(USV(i, j) - A(i, j)) < 1e-6);
        }
    }
    
    std::cout << "  SVD decomposition OK" << std::endl;
}

void test_svd_solve() {
    std::cout << "Testing SVD solve..." << std::endl;
    
    Matrix A(3, 2);
    A(0, 0) = 1; A(0, 1) = 1;
    A(1, 0) = 1; A(1, 1) = 2;
    A(2, 0) = 1; A(2, 1) = 3;
    
    Matrix b(3, 1);
    b(0, 0) = 2;
    b(1, 0) = 3;
    b(2, 0) = 4;
    
    SVDResult svd = svd_decomposition(A);
    Matrix x = solve_svd(svd, b);
    
    assert(x.rows() == 2);
    assert(x.cols() == 1);
    
    std::cout << "  SVD solve OK" << std::endl;
}

void test_chol_decomposition() {
    std::cout << "Testing Cholesky decomposition..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 4; A(0, 1) = 12; A(0, 2) = -16;
    A(1, 0) = 12; A(1, 1) = 37; A(1, 2) = -43;
    A(2, 0) = -16; A(2, 1) = -43; A(2, 2) = 98;
    
    Matrix R = chol_decomposition(A);
    
    Matrix RtR = R.transpose() * R;
    
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            assert(std::abs(RtR(i, j) - A(i, j)) < 1e-10);
        }
    }
    
    std::cout << "  Cholesky decomposition OK" << std::endl;
}

void test_eig_decomposition() {
    std::cout << "Testing Eigenvalue decomposition..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 2; A(0, 1) = -1; A(0, 2) = 0;
    A(1, 0) = -1; A(1, 1) = 2; A(1, 2) = -1;
    A(2, 0) = 0; A(2, 1) = -1; A(2, 2) = 2;
    
    EigResult eig = eig_decomposition(A);
    
    Matrix V = eig.V;
    Matrix D = eig.D;
    
    Matrix AV = A * V;
    Matrix VD = V * D;
    
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            assert(std::abs(AV(i, j) - VD(i, j)) < 1e-10);
        }
    }
    
    std::cout << "  Eigenvalue decomposition OK" << std::endl;
}

void test_solve_linear() {
    std::cout << "Testing solve_linear..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 3; A(0, 1) = 2; A(0, 2) = -1;
    A(1, 0) = 2; A(1, 1) = -2; A(1, 2) = 4;
    A(2, 0) = -1; A(2, 1) = 0.5; A(2, 2) = -1;
    
    Matrix b(3, 1);
    b(0, 0) = 1;
    b(1, 0) = -2;
    b(2, 0) = 0;
    
    Matrix x = solve_linear(A, b);
    
    Matrix Ax = A * x;
    
    for (size_t i = 0; i < 3; ++i) {
        assert(std::abs(Ax(i, 0) - b(i, 0)) < 1e-10);
    }
    
    std::cout << "  solve_linear OK" << std::endl;
}

void test_matrix_methods() {
    std::cout << "Testing Matrix methods..." << std::endl;
    
    Matrix A(3, 3);
    A(0, 0) = 2; A(0, 1) = 1; A(0, 2) = 1;
    A(1, 0) = 4; A(1, 1) = 3; A(1, 2) = 3;
    A(2, 0) = 8; A(2, 1) = 7; A(2, 2) = 9;
    
    Matrix lu_result = A.lu();
    assert(lu_result.rows() == 7);
    assert(lu_result.cols() == 3);
    
    Matrix B(3, 2);
    B(0, 0) = 1; B(0, 1) = -1;
    B(1, 0) = 1; B(1, 1) = 0;
    B(2, 0) = 1; B(2, 1) = 1;
    
    Matrix qr_result = B.qr();
    assert(qr_result.rows() == 5);
    assert(qr_result.cols() == 3);
    
    Matrix C(2, 2);
    C(0, 0) = 4; C(0, 1) = 0;
    C(1, 0) = 0; C(1, 1) = 3;
    
    Matrix svd_result = C.svd();
    assert(svd_result.rows() == 6);
    assert(svd_result.cols() == 2);
    
    std::cout << "  Matrix methods OK" << std::endl;
}

int main() {
    std::cout << "=== Linear Algebra Test Suite ===" << std::endl;
    
    test_lu_decomposition();
    test_lu_solve();
    test_qr_decomposition();
    test_qr_solve();
    test_svd_decomposition();
    test_svd_solve();
    test_chol_decomposition();
    test_eig_decomposition();
    test_solve_linear();
    test_matrix_methods();
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    
    return 0;
}
