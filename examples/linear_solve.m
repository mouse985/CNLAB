# Linear Algebra Solver Demo
# Demonstrates LU, QR, SVD decomposition and linear equation solving

# 1. LU Decomposition Example
A = [2, 1, -1; -3, -1, 2; -2, 1, 2]
b = [8; -11; -3]

# Solve using LU decomposition
x = solve(A, b)

# Verify solution
Ax = A * x
disp("Verification A*x = b:")
disp(Ax)

# 2. LU decomposition details
lu_result = lu(A)
disp("LU decomposition result (L, U, and permutation)")

# 3. QR Decomposition Example
B = [1, -1; 1, 0; 1, 1]
qr_result = qr(B)
disp("QR decomposition result")

# 4. SVD Decomposition Example
C = [4, 0; 0, 3]
svd_result = svd(C)
disp("SVD decomposition result")

# 5. Larger system example
D = [3, 2, -1; 2, -2, 4; -1, 0.5, -1]
e = [1; -2; 0]

y = solve(D, e)
disp("Solution for 3x3 system:")
disp(y)

# Verify
Dy = D * y
disp("Verification D*y = e:")
disp(Dy)
