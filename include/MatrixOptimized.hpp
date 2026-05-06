#pragma once

#include "Matrix.hpp"
#include <cstddef>

namespace cnlab {

class MatrixOptimized {
public:
    static Matrix multiply(const Matrix& A, const Matrix& B);
    static Matrix multiplyBlocked(const Matrix& A, const Matrix& B, size_t blockSize = 64);
    static Matrix multiplySIMD(const Matrix& A, const Matrix& B);
    static Matrix multiplyBlockedSIMD(const Matrix& A, const Matrix& B);
    static Matrix multiplyParallel(const Matrix& A, const Matrix& B);
    
    static void transposeInPlace(Matrix& A);
    static Matrix transposeOptimized(const Matrix& A);
    
    static double dotProduct(const Matrix& a, const Matrix& b);
    static double normOptimized(const Matrix& A);
    
private:
    static constexpr size_t BLOCK_SIZE = 64;
    static constexpr size_t SIMD_WIDTH = 4;
    
    static void multiplyBlock(const double* A, const double* B, double* C,
                              size_t i0, size_t j0, size_t k0,
                              size_t M, size_t N, size_t K);
};

}
