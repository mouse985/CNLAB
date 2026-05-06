# GPU Demo for MatlabCN
# Demonstrates GPU acceleration capabilities

# Check GPU status
disp("=== GPU Information ===")
gpu_info()

# Try to enable GPU
enable_gpu()

# Set GPU threshold (matrices larger than 1M elements use GPU)
set_gpu_threshold(1000000)

# Create large matrices
A = rand(1000, 1000)
B = rand(1000, 1000)

# Matrix multiplication (will use GPU if available and large enough)
disp("Performing matrix multiplication...")
C = A * B

disp("GPU demo completed!")
