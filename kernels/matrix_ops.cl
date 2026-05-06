/*
 * MatlabCN GPU 矩阵运算内核（国产化实现）
 * 纯 OpenCL C 标准编写，兼容 OpenCL 1.2+
 * 支持国产 GPU：摩尔线程、景嘉微、海光 DCU 等
 */

#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

/*
 * 矩阵加法：C = A + B
 */
__kernel void matrix_add(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] + B[idx];
    }
}

/*
 * 矩阵减法：C = A - B
 */
__kernel void matrix_sub(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] - B[idx];
    }
}

/*
 * 矩阵与标量乘法：B = alpha * A
 */
__kernel void matrix_scale(
    __global const FLOAT_TYPE* A,
    __global FLOAT_TYPE* B,
    const FLOAT_TYPE alpha,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        B[idx] = alpha * A[idx];
    }
}

/*
 * 矩阵转置：B = A^T
 * A: rows x cols
 * B: cols x rows
 */
__kernel void matrix_transpose(
    __global const FLOAT_TYPE* A,
    __global FLOAT_TYPE* B,
    const int rows,
    const int cols)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    
    if (row < rows && col < cols) {
        B[col * rows + row] = A[row * cols + col];
    }
}

/*
 * 矩阵元素乘法（Hadamard积）：C = A .* B
 */
__kernel void matrix_element_mul(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] * B[idx];
    }
}

/*
 * 矩阵元素除法：C = A ./ B
 */
__kernel void matrix_element_div(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] / B[idx];
    }
}

/*
 * 矩阵填充：将所有元素设为指定值
 */
__kernel void matrix_fill(
    __global FLOAT_TYPE* A,
    const FLOAT_TYPE value,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        A[idx] = value;
    }
}

/*
 * 矩阵复制：B = A
 */
__kernel void matrix_copy(
    __global const FLOAT_TYPE* A,
    __global FLOAT_TYPE* B,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        B[idx] = A[idx];
    }
}
