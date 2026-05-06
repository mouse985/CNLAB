/*
 * MatlabCN GPU 矩阵乘法内核（国产化实现）
 * 纯 OpenCL C 标准编写，兼容 OpenCL 1.2+
 * 支持国产 GPU：摩尔线程、景嘉微、海光 DCU 等
 */

#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

/*
 * 基础矩阵乘法内核
 * C = A * B
 * A: M x K 矩阵
 * B: K x N 矩阵
 * C: M x N 矩阵
 */
__kernel void gemm_basic(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int M,
    const int K,
    const int N)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    
    if (row < M && col < N) {
        FLOAT_TYPE sum = 0.0f;
        for (int k = 0; k < K; k++) {
            sum += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}

/*
 * 优化矩阵乘法内核 - 使用 local memory 进行 tile 优化
 * TILE_SIZE 应在编译时定义，默认 16（兼容多数国产 GPU）
 */
#ifndef TILE_SIZE
#define TILE_SIZE 16
#endif

__kernel void gemm_tiled(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int M,
    const int K,
    const int N)
{
    __local FLOAT_TYPE tileA[TILE_SIZE][TILE_SIZE];
    __local FLOAT_TYPE tileB[TILE_SIZE][TILE_SIZE];
    
    int localRow = get_local_id(0);
    int localCol = get_local_id(1);
    int globalRow = get_global_id(0);
    int globalCol = get_global_id(1);
    
    FLOAT_TYPE sum = 0.0f;
    
    int numTiles = (K + TILE_SIZE - 1) / TILE_SIZE;
    
    for (int t = 0; t < numTiles; t++) {
        int tiledRow = t * TILE_SIZE + localCol;
        int tiledCol = t * TILE_SIZE + localRow;
        
        // 加载 A 的 tile 到 local memory
        if (globalRow < M && tiledRow < K) {
            tileA[localRow][localCol] = A[globalRow * K + tiledRow];
        } else {
            tileA[localRow][localCol] = 0.0f;
        }
        
        // 加载 B 的 tile 到 local memory
        if (tiledCol < K && globalCol < N) {
            tileB[localRow][localCol] = B[tiledCol * N + globalCol];
        } else {
            tileB[localRow][localCol] = 0.0f;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
        
        // 计算 tile 内的部分和
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += tileA[localRow][k] * tileB[k][localCol];
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (globalRow < M && globalCol < N) {
        C[globalRow * N + globalCol] = sum;
    }
}

/*
 * 带 alpha/beta 缩放的矩阵乘法
 * C = alpha * A * B + beta * C
 */
__kernel void gemm_scaled(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int M,
    const int K,
    const int N,
    const FLOAT_TYPE alpha,
    const FLOAT_TYPE beta)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    
    if (row < M && col < N) {
        FLOAT_TYPE sum = 0.0f;
        for (int k = 0; k < K; k++) {
            sum += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = alpha * sum + beta * C[row * N + col];
    }
}
