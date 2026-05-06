#include "Matrix.hpp"
#include "MatrixOptimized.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace cnlab;
using namespace std::chrono;

template<typename Func>
double measureTime(Func&& func, int iterations = 10) {
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0 / iterations;
}

void testMatrixMultiplication() {
    std::cout << "=== Matrix Multiplication Performance ===" << std::endl;
    std::cout << std::setw(10) << "Size" << std::setw(15) << "Naive(ms)" 
              << std::setw(15) << "Blocked(ms)" << std::setw(15) << "Speedup" << std::endl;
    
    for (size_t n : {64, 128, 256, 512, 1024}) {
        Matrix A = Matrix::rand(n, n);
        Matrix B = Matrix::rand(n, n);
        
        double t1 = measureTime([&]() {
            Matrix C = A * B;
        }, 5);
        
        double t2 = measureTime([&]() {
            Matrix C = MatrixOptimized::multiplyBlocked(A, B);
        }, 5);
        
        std::cout << std::setw(10) << n 
                  << std::setw(15) << std::fixed << std::setprecision(2) << t1
                  << std::setw(15) << t2
                  << std::setw(15) << t1 / t2 << "x" << std::endl;
    }
}

void testTranspose() {
    std::cout << "\n=== Transpose Performance ===" << std::endl;
    std::cout << std::setw(10) << "Size" << std::setw(15) << "Original(ms)" 
              << std::setw(15) << "Optimized(ms)" << std::setw(15) << "Speedup" << std::endl;
    
    for (size_t n : {256, 512, 1024, 2048}) {
        Matrix A = Matrix::rand(n, n);
        
        double t1 = measureTime([&]() {
            Matrix T = A.transpose();
        }, 10);
        
        double t2 = measureTime([&]() {
            Matrix T = MatrixOptimized::transposeOptimized(A);
        }, 10);
        
        std::cout << std::setw(10) << n 
                  << std::setw(15) << std::fixed << std::setprecision(2) << t1
                  << std::setw(15) << t2
                  << std::setw(15) << t1 / t2 << "x" << std::endl;
    }
}

void testNorm() {
    std::cout << "\n=== Norm Calculation Performance ===" << std::endl;
    std::cout << std::setw(10) << "Size" << std::setw(15) << "Original(ms)" 
              << std::setw(15) << "Optimized(ms)" << std::setw(15) << "Speedup" << std::endl;
    
    for (size_t n : {1000, 2000, 5000, 10000}) {
        Matrix A = Matrix::rand(1, n);
        
        double t1 = measureTime([&]() {
            double n = A.norm();
        }, 100);
        
        double t2 = measureTime([&]() {
            double n = MatrixOptimized::normOptimized(A);
        }, 100);
        
        std::cout << std::setw(10) << n 
                  << std::setw(15) << std::fixed << std::setprecision(3) << t1
                  << std::setw(15) << t2
                  << std::setw(15) << t1 / t2 << "x" << std::endl;
    }
}

void testSIMD() {
    std::cout << "\n=== SIMD vs Blocked Performance ===" << std::endl;
    std::cout << std::setw(10) << "Size" << std::setw(15) << "Blocked(ms)" 
              << std::setw(15) << "SIMD(ms)" << std::setw(15) << "Speedup" << std::endl;
    
    for (size_t n : {64, 128, 256, 512}) {
        Matrix A = Matrix::rand(n, n);
        Matrix B = Matrix::rand(n, n);
        
        double t1 = measureTime([&]() {
            Matrix C = MatrixOptimized::multiplyBlocked(A, B);
        }, 5);
        
        double t2 = measureTime([&]() {
            Matrix C = MatrixOptimized::multiplySIMD(A, B);
        }, 5);
        
        std::cout << std::setw(10) << n 
                  << std::setw(15) << std::fixed << std::setprecision(2) << t1
                  << std::setw(15) << t2
                  << std::setw(15) << t1 / t2 << "x" << std::endl;
    }
}

void testCorrectness() {
    std::cout << "\n=== Correctness Verification ===" << std::endl;
    
    Matrix A = Matrix::rand(50, 50);
    Matrix B = Matrix::rand(50, 50);
    
    Matrix C1 = A * B;
    Matrix C2 = MatrixOptimized::multiplyBlocked(A, B);
    Matrix C3 = MatrixOptimized::multiplySIMD(A, B);
    
    bool correct = true;
    for (size_t i = 0; i < 50; ++i) {
        for (size_t j = 0; j < 50; ++j) {
            if (std::abs(C1(i, j) - C2(i, j)) > 1e-10 ||
                std::abs(C1(i, j) - C3(i, j)) > 1e-10) {
                correct = false;
                break;
            }
        }
    }
    
    std::cout << "Results match: " << (correct ? "PASS" : "FAIL") << std::endl;
}

int main() {
    std::cout << "CNLab Performance Test Suite" << std::endl;
    std::cout << "================================" << std::endl;
    
    testMatrixMultiplication();
    testTranspose();
    testNorm();
    testSIMD();
    testCorrectness();
    
    std::cout << "\nPerformance tests completed!" << std::endl;
    
    return 0;
}
