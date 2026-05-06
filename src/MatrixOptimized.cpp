#include "MatrixOptimized.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __AVX__
#include <immintrin.h>
#endif

namespace cnlab {

Matrix MatrixOptimized::multiply(const Matrix& A, const Matrix& B) {
    return multiplyBlocked(A, B);
}

Matrix MatrixOptimized::multiplyBlocked(const Matrix& A, const Matrix& B, size_t blockSize) {
    size_t M = A.rows();
    size_t N = B.cols();
    size_t K = A.cols();
    
    if (K != B.rows()) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }
    
    Matrix C(M, N);
    const double* a = A.data();
    const double* b = B.data();
    double* c = C.data();
    
    std::memset(c, 0, M * N * sizeof(double));
    
    for (size_t i0 = 0; i0 < M; i0 += blockSize) {
        size_t iMax = std::min(i0 + blockSize, M);
        
        for (size_t j0 = 0; j0 < N; j0 += blockSize) {
            size_t jMax = std::min(j0 + blockSize, N);
            
            for (size_t k0 = 0; k0 < K; k0 += blockSize) {
                size_t kMax = std::min(k0 + blockSize, K);
                
                for (size_t i = i0; i < iMax; ++i) {
                    for (size_t k = k0; k < kMax; ++k) {
                        double aik = a[i * K + k];
                        
                        for (size_t j = j0; j < jMax; ++j) {
                            c[i * N + j] += aik * b[k * N + j];
                        }
                    }
                }
            }
        }
    }
    
    return C;
}

Matrix MatrixOptimized::multiplySIMD(const Matrix& A, const Matrix& B) {
    size_t M = A.rows();
    size_t N = B.cols();
    size_t K = A.cols();
    
    if (K != B.rows()) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }
    
    if (M < 256 || N < 256 || K < 256) {
        return multiplyBlocked(A, B);
    }
    
    return multiplyBlockedSIMD(A, B);
}

Matrix MatrixOptimized::multiplyBlockedSIMD(const Matrix& A, const Matrix& B) {
    size_t M = A.rows();
    size_t N = B.cols();
    size_t K = A.cols();
    
    Matrix C(M, N);
    const double* a = A.data();
    const double* b = B.data();
    double* c = C.data();
    
    std::memset(c, 0, M * N * sizeof(double));
    
    constexpr size_t BLOCK = 64;
    
    for (size_t i0 = 0; i0 < M; i0 += BLOCK) {
        size_t iMax = std::min(i0 + BLOCK, M);
        
        for (size_t j0 = 0; j0 < N; j0 += BLOCK) {
            size_t jMax = std::min(j0 + BLOCK, N);
            
            for (size_t k0 = 0; k0 < K; k0 += BLOCK) {
                size_t kMax = std::min(k0 + BLOCK, K);
                
                for (size_t i = i0; i < iMax; ++i) {
                    for (size_t k = k0; k < kMax; ++k) {
                        double aik = a[i * K + k];
                        
#ifdef __AVX__
                        __m256d a_vec = _mm256_set1_pd(aik);
                        size_t j = j0;
                        
                        for (; j + 4 <= jMax; j += 4) {
                            __m256d b_vec = _mm256_loadu_pd(&b[k * N + j]);
                            __m256d c_vec = _mm256_loadu_pd(&c[i * N + j]);
                            c_vec = _mm256_add_pd(c_vec, _mm256_mul_pd(a_vec, b_vec));
                            _mm256_storeu_pd(&c[i * N + j], c_vec);
                        }
                        
                        for (; j < jMax; ++j) {
                            c[i * N + j] += aik * b[k * N + j];
                        }
#else
                        for (size_t j = j0; j < jMax; ++j) {
                            c[i * N + j] += aik * b[k * N + j];
                        }
#endif
                    }
                }
            }
        }
    }
    
    return C;
}

Matrix MatrixOptimized::multiplyParallel(const Matrix& A, const Matrix& B) {
    size_t M = A.rows();
    size_t N = B.cols();
    size_t K = A.cols();
    
    if (K != B.rows()) {
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");
    }
    
    Matrix C(M, N);
    const double* a = A.data();
    const double* b = B.data();
    double* c = C.data();
    
    std::memset(c, 0, M * N * sizeof(double));
    
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < static_cast<int>(M); ++i) {
        for (size_t k = 0; k < K; ++k) {
            double aik = a[i * K + k];
            
            for (size_t j = 0; j < N; ++j) {
                c[i * N + j] += aik * b[k * N + j];
            }
        }
    }
    
    return C;
}

void MatrixOptimized::transposeInPlace(Matrix& A) {
    if (!A.isSquare()) {
        throw std::invalid_argument("In-place transpose requires square matrix");
    }
    
    size_t n = A.rows();
    double* data = A.data();
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            std::swap(data[i * n + j], data[j * n + i]);
        }
    }
}

Matrix MatrixOptimized::transposeOptimized(const Matrix& A) {
    size_t rows = A.rows();
    size_t cols = A.cols();
    
    Matrix result(cols, rows);
    const double* src = A.data();
    double* dst = result.data();
    
    constexpr size_t BLOCK = 64;
    
    for (size_t i0 = 0; i0 < rows; i0 += BLOCK) {
        size_t iMax = std::min(i0 + BLOCK, rows);
        
        for (size_t j0 = 0; j0 < cols; j0 += BLOCK) {
            size_t jMax = std::min(j0 + BLOCK, cols);
            
            for (size_t i = i0; i < iMax; ++i) {
                for (size_t j = j0; j < jMax; ++j) {
                    dst[j * rows + i] = src[i * cols + j];
                }
            }
        }
    }
    
    return result;
}

double MatrixOptimized::dotProduct(const Matrix& a, const Matrix& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("Vectors must have same size");
    }
    
    const double* pa = a.data();
    const double* pb = b.data();
    size_t n = a.size();
    
    double sum = 0.0;
    
#ifdef __AVX__
    __m256d sum_vec = _mm256_setzero_pd();
    size_t i = 0;
    
    for (; i + 4 <= n; i += 4) {
        __m256d a_vec = _mm256_loadu_pd(&pa[i]);
        __m256d b_vec = _mm256_loadu_pd(&pb[i]);
        sum_vec = _mm256_add_pd(sum_vec, _mm256_mul_pd(a_vec, b_vec));
    }
    
    double temp[4];
    _mm256_storeu_pd(temp, sum_vec);
    sum = temp[0] + temp[1] + temp[2] + temp[3];
    
    for (; i < n; ++i) {
        sum += pa[i] * pb[i];
    }
#else
    for (size_t i = 0; i < n; ++i) {
        sum += pa[i] * pb[i];
    }
#endif
    
    return sum;
}

double MatrixOptimized::normOptimized(const Matrix& A) {
    const double* data = A.data();
    size_t n = A.size();
    
    double sum = 0.0;
    
#ifdef __AVX__
    __m256d sum_vec = _mm256_setzero_pd();
    size_t i = 0;
    
    for (; i + 4 <= n; i += 4) {
        __m256d vec = _mm256_loadu_pd(&data[i]);
        sum_vec = _mm256_add_pd(sum_vec, _mm256_mul_pd(vec, vec));
    }
    
    double temp[4];
    _mm256_storeu_pd(temp, sum_vec);
    sum = temp[0] + temp[1] + temp[2] + temp[3];
    
    for (; i < n; ++i) {
        sum += data[i] * data[i];
    }
#else
    for (size_t i = 0; i < n; ++i) {
        sum += data[i] * data[i];
    }
#endif
    
    return std::sqrt(sum);
}

}
