#include "Matrix.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace cnlab;

void test_constructors() {
    std::cout << "Testing constructors..." << std::endl;
    
    Matrix m1;
    assert(m1.isEmpty());
    
    Matrix m2(3, 4);
    assert(m2.rows() == 3);
    assert(m2.cols() == 4);
    assert(m2.size() == 12);
    
    Matrix m3 = Matrix::zeros(2, 2);
    assert(m3(0, 0) == 0.0);
    assert(m3(1, 1) == 0.0);
    
    Matrix m4 = Matrix::ones(2, 3);
    assert(m4(0, 0) == 1.0);
    assert(m4(1, 2) == 1.0);
    
    Matrix m5 = Matrix::eye(3);
    assert(m5(0, 0) == 1.0);
    assert(m5(0, 1) == 0.0);
    assert(m5(1, 1) == 1.0);
    assert(m5(2, 2) == 1.0);
    
    std::cout << "  Constructors OK" << std::endl;
}

void test_element_access() {
    std::cout << "Testing element access..." << std::endl;
    
    Matrix m(3, 3);
    m(0, 0) = 1.0;
    m(1, 1) = 5.0;
    m(2, 2) = 9.0;
    
    assert(m(0, 0) == 1.0);
    assert(m(1, 1) == 5.0);
    assert(m(2, 2) == 9.0);
    
    m(0) = 10.0;
    assert(m(0, 0) == 10.0);
    
    std::cout << "  Element access OK" << std::endl;
}

void test_arithmetic() {
    std::cout << "Testing arithmetic operations..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;
    
    Matrix b(2, 2);
    b(0, 0) = 5; b(0, 1) = 6;
    b(1, 0) = 7; b(1, 1) = 8;
    
    Matrix c = a + b;
    assert(c(0, 0) == 6);
    assert(c(0, 1) == 8);
    assert(c(1, 0) == 10);
    assert(c(1, 1) == 12);
    
    Matrix d = b - a;
    assert(d(0, 0) == 4);
    assert(d(0, 1) == 4);
    assert(d(1, 0) == 4);
    assert(d(1, 1) == 4);
    
    Matrix e = a * b;
    assert(e(0, 0) == 19);
    assert(e(0, 1) == 22);
    assert(e(1, 0) == 43);
    assert(e(1, 1) == 50);
    
    Matrix f = a * 2.0;
    assert(f(0, 0) == 2);
    assert(f(0, 1) == 4);
    assert(f(1, 0) == 6);
    assert(f(1, 1) == 8);
    
    Matrix g = 3.0 * a;
    assert(g(0, 0) == 3);
    assert(g(0, 1) == 6);
    assert(g(1, 0) == 9);
    assert(g(1, 1) == 12);
    
    Matrix h(2, 2);
    h(0, 0) = 5; h(0, 1) = 8;
    h(1, 0) = 10; h(1, 1) = 15;
    
    Matrix i = h % 3.0;
    assert(std::abs(i(0, 0) - 2.0) < 1e-10);
    assert(std::abs(i(0, 1) - 2.0) < 1e-10);
    assert(std::abs(i(1, 0) - 1.0) < 1e-10);
    assert(std::abs(i(1, 1) - 0.0) < 1e-10);
    
    Matrix j(2, 2);
    j(0, 0) = 2; j(0, 1) = 3;
    j(1, 0) = 4; j(1, 1) = 5;
    
    Matrix k = h % j;
    assert(std::abs(k(0, 0) - 1.0) < 1e-10);
    assert(std::abs(k(0, 1) - 2.0) < 1e-10);
    assert(std::abs(k(1, 0) - 2.0) < 1e-10);
    assert(std::abs(k(1, 1) - 0.0) < 1e-10);
    
    std::cout << "  Arithmetic OK" << std::endl;
}

void test_transpose() {
    std::cout << "Testing transpose..." << std::endl;
    
    Matrix a(2, 3);
    a(0, 0) = 1; a(0, 1) = 2; a(0, 2) = 3;
    a(1, 0) = 4; a(1, 1) = 5; a(1, 2) = 6;
    
    Matrix t = a.transpose();
    assert(t.rows() == 3);
    assert(t.cols() == 2);
    assert(t(0, 0) == 1);
    assert(t(0, 1) == 4);
    assert(t(1, 0) == 2);
    assert(t(1, 1) == 5);
    assert(t(2, 0) == 3);
    assert(t(2, 1) == 6);
    
    std::cout << "  Transpose OK" << std::endl;
}

void test_determinant() {
    std::cout << "Testing determinant..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 4; a(0, 1) = 7;
    a(1, 0) = 2; a(1, 1) = 6;
    
    double det_a = a.det();
    assert(std::abs(det_a - 10.0) < 1e-10);
    
    Matrix b(3, 3);
    b(0, 0) = 1; b(0, 1) = 2; b(0, 2) = 3;
    b(1, 0) = 0; b(1, 1) = 1; b(1, 2) = 4;
    b(2, 0) = 5; b(2, 1) = 6; b(2, 2) = 0;
    
    double det_b = b.det();
    assert(std::abs(det_b - 1.0) < 1e-10);
    
    std::cout << "  Determinant OK" << std::endl;
}

void test_inverse() {
    std::cout << "Testing inverse..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 4; a(0, 1) = 7;
    a(1, 0) = 2; a(1, 1) = 6;
    
    Matrix inv_a = a.inverse();
    Matrix identity = a * inv_a;
    
    assert(std::abs(identity(0, 0) - 1.0) < 1e-10);
    assert(std::abs(identity(0, 1) - 0.0) < 1e-10);
    assert(std::abs(identity(1, 0) - 0.0) < 1e-10);
    assert(std::abs(identity(1, 1) - 1.0) < 1e-10);
    
    std::cout << "  Inverse OK" << std::endl;
}

void test_math_functions() {
    std::cout << "Testing math functions..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 1; a(0, 1) = 4;
    a(1, 0) = 9; a(1, 1) = 16;
    
    Matrix s = a.sqrt();
    assert(std::abs(s(0, 0) - 1.0) < 1e-10);
    assert(std::abs(s(0, 1) - 2.0) < 1e-10);
    assert(std::abs(s(1, 0) - 3.0) < 1e-10);
    assert(std::abs(s(1, 1) - 4.0) < 1e-10);
    
    Matrix p = a.pow(0.5);
    assert(std::abs(p(0, 0) - 1.0) < 1e-10);
    assert(std::abs(p(0, 1) - 2.0) < 1e-10);
    
    std::cout << "  Math functions OK" << std::endl;
}

void test_reductions() {
    std::cout << "Testing reductions..." << std::endl;
    
    Matrix a(2, 3);
    a(0, 0) = 1; a(0, 1) = 2; a(0, 2) = 3;
    a(1, 0) = 4; a(1, 1) = 5; a(1, 2) = 6;
    
    assert(a.sum() == 21.0);
    assert(a.mean() == 3.5);
    assert(a.max() == 6.0);
    assert(a.min() == 1.0);
    assert(a.prod() == 720.0);
    
    std::cout << "  Reductions OK" << std::endl;
}

void test_norm() {
    std::cout << "Testing norm..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 3; a(0, 1) = 4;
    a(1, 0) = 0; a(1, 1) = 0;
    
    double n = a.norm();
    assert(std::abs(n - 5.0) < 1e-10);
    
    std::cout << "  Norm OK" << std::endl;
}

void test_row_col() {
    std::cout << "Testing row/col access..." << std::endl;
    
    Matrix a(3, 4);
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            a(i, j) = static_cast<double>(i * 4 + j);
        }
    }
    
    Matrix r = a.row(1);
    assert(r.rows() == 1);
    assert(r.cols() == 4);
    assert(r(0, 0) == 4);
    assert(r(0, 3) == 7);
    
    Matrix c = a.col(2);
    assert(c.rows() == 3);
    assert(c.cols() == 1);
    assert(c(0, 0) == 2);
    assert(c(1, 0) == 6);
    assert(c(2, 0) == 10);
    
    std::cout << "  Row/Col OK" << std::endl;
}

void test_reshape() {
    std::cout << "Testing reshape..." << std::endl;
    
    Matrix a(2, 6);
    for (size_t i = 0; i < 12; ++i) {
        a(i) = static_cast<double>(i);
    }
    
    Matrix b = a.reshape(3, 4);
    assert(b.rows() == 3);
    assert(b.cols() == 4);
    assert(b(0, 0) == 0);
    assert(b(2, 3) == 11);
    
    Matrix f = a.flatten();
    assert(f.rows() == 1);
    assert(f.cols() == 12);
    
    std::cout << "  Reshape OK" << std::endl;
}

void test_repmat() {
    std::cout << "Testing repmat..." << std::endl;
    
    Matrix a(2, 2);
    a(0, 0) = 1; a(0, 1) = 2;
    a(1, 0) = 3; a(1, 1) = 4;
    
    Matrix b = a.repmat(2, 3);
    assert(b.rows() == 4);
    assert(b.cols() == 6);
    assert(b(0, 0) == 1);
    assert(b(0, 2) == 1);
    assert(b(2, 0) == 1);
    
    std::cout << "  Repmat OK" << std::endl;
}

int main() {
    std::cout << "=== Matrix Library Test Suite ===" << std::endl;
    
    test_constructors();
    test_element_access();
    test_arithmetic();
    test_transpose();
    test_determinant();
    test_inverse();
    test_math_functions();
    test_reductions();
    test_norm();
    test_row_col();
    test_reshape();
    test_repmat();
    
    std::cout << "\n=== All tests passed! ===" << std::endl;
    
    return 0;
}
