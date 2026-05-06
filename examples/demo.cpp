#include "Matrix.hpp"
#include <iostream>

using namespace cnlab;

int main() {
    std::cout << "=== MatlabCN Matrix Demo ===" << std::endl;
    std::cout << std::endl;

    std::cout << "1. Creating matrices:" << std::endl;
    Matrix A = Matrix::zeros(3, 3);
    A.print("A (zeros)");
    std::cout << std::endl;

    Matrix B = Matrix::ones(2, 4);
    B.print("B (ones)");
    std::cout << std::endl;

    Matrix I = Matrix::eye(4);
    I.print("I (identity)");
    std::cout << std::endl;

    std::cout << "2. Matrix arithmetic:" << std::endl;
    Matrix M1(2, 2);
    M1(0, 0) = 1; M1(0, 1) = 2;
    M1(1, 0) = 3; M1(1, 1) = 4;
    M1.print("M1");

    Matrix M2(2, 2);
    M2(0, 0) = 5; M2(0, 1) = 6;
    M2(1, 0) = 7; M2(1, 1) = 8;
    M2.print("M2");

    Matrix M3 = M1 + M2;
    M3.print("M1 + M2");

    Matrix M4 = M1 * M2;
    M4.print("M1 * M2");
    std::cout << std::endl;

    std::cout << "3. Transpose and inverse:" << std::endl;
    Matrix T = M1.transpose();
    T.print("M1'");

    Matrix Inv = M1.inverse();
    Inv.print("inv(M1)");

    Matrix Check = M1 * Inv;
    Check.print("M1 * inv(M1)");
    std::cout << std::endl;

    std::cout << "4. Determinant and norm:" << std::endl;
    std::cout << "det(M1) = " << M1.det() << std::endl;
    std::cout << "norm(M1) = " << M1.norm() << std::endl;
    std::cout << std::endl;

    std::cout << "5. Math functions:" << std::endl;
    Matrix X(2, 2);
    X(0, 0) = 1; X(0, 1) = 4;
    X(1, 0) = 9; X(1, 1) = 16;
    X.print("X");

    Matrix S = X.sqrt();
    S.print("sqrt(X)");

    Matrix E = X.exp();
    E.print("exp(X)");
    std::cout << std::endl;

    std::cout << "6. Reductions:" << std::endl;
    std::cout << "sum(X) = " << X.sum() << std::endl;
    std::cout << "mean(X) = " << X.mean() << std::endl;
    std::cout << "max(X) = " << X.max() << std::endl;
    std::cout << "min(X) = " << X.min() << std::endl;
    std::cout << std::endl;

    std::cout << "7. Random matrices:" << std::endl;
    Matrix R = Matrix::rand(3, 3);
    R.print("R (uniform random)");

    Matrix N = Matrix::randn(2, 3);
    N.print("N (normal random)");
    std::cout << std::endl;

    std::cout << "8. Row/Column operations:" << std::endl;
    Matrix Big(4, 5);
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            Big(i, j) = static_cast<double>(i * 5 + j);
        }
    }
    Big.print("Big");

    Big.row(1).print("Big(2,:)");
    Big.col(2).print("Big(:,3)");
    std::cout << std::endl;

    std::cout << "=== Demo completed! ===" << std::endl;

    return 0;
}
