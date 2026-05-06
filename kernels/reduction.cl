/*
 * MatlabCN GPU 归约运算内核（国产化实现）
 * 纯 OpenCL C 标准编写，兼容 OpenCL 1.2+
 * 支持国产 GPU：摩尔线程、景嘉微、海光 DCU 等
 */

#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

#ifndef WG_SIZE
#define WG_SIZE 256
#endif

/*
 * 树形归约算法 - 求和
 * 使用 local memory 进行高效的并行归约
 */
__kernel void reduction_sum(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    // 加载数据到 local memory
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = 0.0f;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // 树形归约
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] += scratch[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // 将结果写回 global memory
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

/*
 * 树形归约算法 - 求最大值
 */
__kernel void reduction_max(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    // 加载数据到 local memory
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = -FLT_MAX;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // 树形归约
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] = max(scratch[lid], scratch[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // 将结果写回 global memory
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

/*
 * 树形归约算法 - 求最小值
 */
__kernel void reduction_min(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    // 加载数据到 local memory
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = FLT_MAX;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // 树形归约
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] = min(scratch[lid], scratch[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // 将结果写回 global memory
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

/*
 * 矩阵所有元素求和（二维归约）
 * 先对每个 work-group 内的元素求和，再对结果进行第二次归约
 */
__kernel void matrix_sum_all(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int rows,
    const int cols)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    int total = rows * cols;
    
    // 加载数据到 local memory
    if (gid < total) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = 0.0f;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // 树形归约
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] += scratch[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // 将结果写回 global memory
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

/*
 * 最终归约（用于多阶段归约的最后一步）
 * 对中间结果进行最终归约
 */
__kernel void reduction_final_sum(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    
    // 加载数据到 local memory
    if (lid < n) {
        scratch[lid] = input[lid];
    } else {
        scratch[lid] = 0.0f;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // 树形归约
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] += scratch[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // 将结果写回 global memory
    if (lid == 0) {
        output[0] = scratch[0];
    }
}
