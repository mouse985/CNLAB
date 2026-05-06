#include "LinearAlgebra.hpp"
#include <cmath>
#include <stdexcept>

namespace cnlab {

LUResult lu_decomposition(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) {
        throw std::invalid_argument("LU decomposition requires square matrix");
    }
    
    LUResult result(n);
    Matrix& L = result.L;
    Matrix& U = result.U;
    std::vector<size_t>& perm = result.perm;
    
    for (size_t i = 0; i < n; ++i) {
        perm[i] = i;
    }
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            U(i, j) = A(i, j);
        }
    }
    
    for (size_t k = 0; k < n; ++k) {
        size_t maxRow = k;
        double maxVal = std::abs(U(k, k));
        
        for (size_t i = k + 1; i < n; ++i) {
            double val = std::abs(U(i, k));
            if (val > maxVal) {
                maxVal = val;
                maxRow = i;
            }
        }
        
        if (maxRow != k) {
            for (size_t j = 0; j < n; ++j) {
                std::swap(U(k, j), U(maxRow, j));
            }
            std::swap(perm[k], perm[maxRow]);
            result.sign = -result.sign;
        }
        
        if (std::abs(U(k, k)) < 1e-15) {
            throw std::runtime_error("Matrix is singular");
        }
        
        for (size_t i = k + 1; i < n; ++i) {
            L(i, k) = U(i, k) / U(k, k);
            
            for (size_t j = k; j < n; ++j) {
                U(i, j) = U(i, j) - L(i, k) * U(k, j);
            }
        }
        
        L(k, k) = 1.0;
    }
    
    return result;
}

QRResult qr_decomposition(const Matrix& A) {
    size_t m = A.rows();
    size_t n = A.cols();
    
    if (m < n) {
        throw std::invalid_argument("QR decomposition requires m >= n");
    }
    
    QRResult result(m, n);
    Matrix& Q = result.Q;
    Matrix& R = result.R;
    
    for (size_t i = 0; i < m; ++i) {
        Q(i, i) = 1.0;
    }
    
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            R(i, j) = A(i, j);
        }
    }
    
    for (size_t k = 0; k < n; ++k) {
        double norm = 0.0;
        for (size_t i = k; i < m; ++i) {
            norm += R(i, k) * R(i, k);
        }
        norm = std::sqrt(norm);
        
        if (norm < 1e-15) {
            continue;
        }
        
        double alpha = (R(k, k) >= 0) ? -norm : norm;
        double r = std::sqrt(0.5 * (alpha * alpha - R(k, k) * alpha));
        
        std::vector<double> v(m - k);
        v[0] = (R(k, k) - alpha) / (2 * r);
        for (size_t i = 1; i < m - k; ++i) {
            v[i] = R(k + i, k) / (2 * r);
        }
        
        for (size_t j = k; j < n; ++j) {
            double dot = 0.0;
            for (size_t i = 0; i < m - k; ++i) {
                dot += v[i] * R(k + i, j);
            }
            for (size_t i = 0; i < m - k; ++i) {
                R(k + i, j) = R(k + i, j) - 2 * v[i] * dot;
            }
        }
        
        for (size_t j = 0; j < m; ++j) {
            double dot = 0.0;
            for (size_t i = 0; i < m - k; ++i) {
                dot += v[i] * Q(j, k + i);
            }
            for (size_t i = 0; i < m - k; ++i) {
                Q(j, k + i) = Q(j, k + i) - 2 * v[i] * dot;
            }
        }
    }
    
    Q = Q.transpose();
    
    return result;
}

SVDResult svd_decomposition(const Matrix& A) {
    size_t m = A.rows();
    size_t n = A.cols();
    
    SVDResult result(m, n);
    Matrix& U = result.U;
    Matrix& S = result.S;
    Matrix& V = result.V;
    
    for (size_t i = 0; i < m; ++i) {
        U(i, i) = 1.0;
    }
    for (size_t i = 0; i < n; ++i) {
        V(i, i) = 1.0;
    }
    
    Matrix B = A;
    
    const int maxIterations = 100;
    const double epsilon = 1e-10;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        double maxOffDiag = 0.0;
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                double dot = 0.0;
                for (size_t k = 0; k < m; ++k) {
                    dot += B(k, i) * B(k, j);
                }
                
                if (std::abs(dot) > maxOffDiag) {
                    maxOffDiag = std::abs(dot);
                }
                
                if (std::abs(dot) < epsilon) {
                    continue;
                }
                
                double normi = 0.0;
                double normj = 0.0;
                for (size_t k = 0; k < m; ++k) {
                    normi += B(k, i) * B(k, i);
                    normj += B(k, j) * B(k, j);
                }
                
                double tau = (normj - normi) / (2 * dot);
                double t = (tau >= 0) ? 1.0 / (tau + std::sqrt(1 + tau * tau)) 
                                      : 1.0 / (tau - std::sqrt(1 + tau * tau));
                double c = 1.0 / std::sqrt(1 + t * t);
                double s = t * c;
                
                for (size_t k = 0; k < m; ++k) {
                    double Bik = B(k, i);
                    double Bjk = B(k, j);
                    B(k, i) = c * Bik - s * Bjk;
                    B(k, j) = s * Bik + c * Bjk;
                }
                
                for (size_t k = 0; k < n; ++k) {
                    double Vik = V(k, i);
                    double Vjk = V(k, j);
                    V(k, i) = c * Vik - s * Vjk;
                    V(k, j) = s * Vik + c * Vjk;
                }
            }
        }
        
        if (maxOffDiag < epsilon) {
            break;
        }
    }
    
    for (size_t i = 0; i < n; ++i) {
        double norm = 0.0;
        for (size_t k = 0; k < m; ++k) {
            norm += B(k, i) * B(k, i);
        }
        S(i, i) = std::sqrt(norm);
        
        if (S(i, i) > epsilon) {
            for (size_t k = 0; k < m; ++k) {
                U(k, i) = B(k, i) / S(i, i);
            }
        }
    }
    
    return result;
}

EigResult eig_decomposition(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) {
        throw std::invalid_argument("Matrix must be square");
    }
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (std::abs(A(i, j) - A(j, i)) > 1e-10) {
                throw std::invalid_argument("eig currently only supports symmetric matrices");
            }
        }
    }
    
    EigResult result(n);
    Matrix& V = result.V;
    Matrix& D = result.D;
    
    for (size_t i = 0; i < n; ++i) {
        V(i, i) = 1.0;
        for (size_t j = 0; j < n; ++j) {
            D(i, j) = A(i, j);
        }
    }
    
    const int maxIterations = 100;
    const double epsilon = 1e-10;
    
    for (int iter = 0; iter < maxIterations; ++iter) {
        double maxOffDiag = 0.0;
        size_t p = 0, q = 0;
        
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (std::abs(D(i, j)) > maxOffDiag) {
                    maxOffDiag = std::abs(D(i, j));
                    p = i;
                    q = j;
                }
            }
        }
        
        if (maxOffDiag < epsilon) {
            break;
        }
        
        double theta = (D(q, q) - D(p, p)) / (2.0 * D(p, q));
        double t = 1.0 / (std::abs(theta) + std::sqrt(1.0 + theta * theta));
        if (theta < 0) t = -t;
        
        double c = 1.0 / std::sqrt(1.0 + t * t);
        double s = t * c;
        
        for (size_t i = 0; i < n; ++i) {
            if (i != p && i != q) {
                double dip = D(i, p);
                double diq = D(i, q);
                D(i, p) = c * dip - s * diq;
                D(p, i) = D(i, p);
                D(i, q) = s * dip + c * diq;
                D(q, i) = D(i, q);
            }
        }
        
        double dpp = D(p, p);
        double dqq = D(q, q);
        double dpq = D(p, q);
        
        D(p, p) = c * c * dpp - 2.0 * s * c * dpq + s * s * dqq;
        D(q, q) = s * s * dpp + 2.0 * s * c * dpq + c * c * dqq;
        D(p, q) = 0.0;
        D(q, p) = 0.0;
        
        for (size_t i = 0; i < n; ++i) {
            double vip = V(i, p);
            double viq = V(i, q);
            V(i, p) = c * vip - s * viq;
            V(i, q) = s * vip + c * viq;
        }
    }
    
    // Ensure all off-diagonals are exactly 0
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (i != j) D(i, j) = 0.0;
        }
    }
    
    return result;
}

Matrix chol_decomposition(const Matrix& A) {
    size_t n = A.rows();
    if (n != A.cols()) {
        throw std::invalid_argument("Matrix must be square");
    }
    
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (std::abs(A(i, j) - A(j, i)) > 1e-10) {
                throw std::invalid_argument("chol requires symmetric positive definite matrix");
            }
        }
    }
    
    Matrix L = Matrix::zeros(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < j; ++k) {
                sum += L(i, k) * L(j, k);
            }
            
            if (i == j) {
                double diff = A(i, i) - sum;
                if (diff <= 0.0) {
                    throw std::runtime_error("Matrix is not positive definite");
                }
                L(i, j) = std::sqrt(diff);
            } else {
                L(i, j) = (A(i, j) - sum) / L(j, j);
            }
        }
    }
    
    return L.transpose();
}

Matrix solve_lu(const LUResult& lu, const Matrix& b) {
    size_t n = lu.U.rows();
    
    if (b.rows() != n || b.cols() != 1) {
        throw std::invalid_argument("b must be n×1 vector");
    }
    
    Matrix y(n, 1);
    for (size_t i = 0; i < n; ++i) {
        y(i, 0) = b(lu.perm[i], 0);
        for (size_t j = 0; j < i; ++j) {
            y(i, 0) -= lu.L(i, j) * y(j, 0);
        }
    }
    
    Matrix x(n, 1);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        x(i, 0) = y(i, 0);
        for (size_t j = i + 1; j < n; ++j) {
            x(i, 0) -= lu.U(i, j) * x(j, 0);
        }
        x(i, 0) /= lu.U(i, i);
    }
    
    return x;
}

Matrix solve_qr(const QRResult& qr, const Matrix& b) {
    size_t m = qr.Q.rows();
    size_t n = qr.R.cols();
    
    if (b.rows() != m || b.cols() != 1) {
        throw std::invalid_argument("b must be m×1 vector");
    }
    
    Matrix Qtb(n, 1);
    for (size_t i = 0; i < n; ++i) {
        for (size_t k = 0; k < m; ++k) {
            Qtb(i, 0) += qr.Q(k, i) * b(k, 0);
        }
    }
    
    Matrix x(n, 1);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        x(i, 0) = Qtb(i, 0);
        for (size_t j = i + 1; j < n; ++j) {
            x(i, 0) -= qr.R(i, j) * x(j, 0);
        }
        x(i, 0) /= qr.R(i, i);
    }
    
    return x;
}

Matrix solve_svd(const SVDResult& svd, const Matrix& b) {
    size_t m = svd.U.rows();
    size_t n = svd.V.rows();
    
    if (b.rows() != m || b.cols() != 1) {
        throw std::invalid_argument("b must be m×1 vector");
    }
    
    Matrix Utb(n, 1);
    for (size_t i = 0; i < n; ++i) {
        for (size_t k = 0; k < m; ++k) {
            Utb(i, 0) += svd.U(k, i) * b(k, 0);
        }
    }
    
    Matrix SinvUtb(n, 1);
    for (size_t i = 0; i < n; ++i) {
        if (svd.S(i, i) > 1e-15) {
            SinvUtb(i, 0) = Utb(i, 0) / svd.S(i, i);
        }
    }
    
    Matrix x(n, 1);
    for (size_t i = 0; i < n; ++i) {
        for (size_t k = 0; k < n; ++k) {
            x(i, 0) += svd.V(i, k) * SinvUtb(k, 0);
        }
    }
    
    return x;
}

Matrix solve_linear(const Matrix& A, const Matrix& b) {
    LUResult lu = lu_decomposition(A);
    return solve_lu(lu, b);
}

}
