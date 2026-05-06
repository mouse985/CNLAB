# CNLab 文件I/O功能演示

# 1. CSV文件读写
disp("=== CSV文件读写 ===")
A = [1, 2, 3; 4, 5, 6; 7, 8, 9]
csvwrite("test_matrix.csv", A)
disp("矩阵已写入 test_matrix.csv")

B = csvread("test_matrix.csv")
disp("从CSV读取的矩阵:")
disp(B)

# 2. 分隔符文本读写
disp("=== 分隔符文本读写 ===")
C = [1.5, 2.5; 3.5, 4.5]
dlmwrite("test_data.txt", C, "\t", "%.2f")
disp("数据已写入 test_data.txt (制表符分隔)")

D = dlmread("test_data.txt", "\t")
disp("从分隔符文本读取的数据:")
disp(D)

# 3. 智能矩阵文件读写
disp("=== 智能矩阵文件读写 ===")
E = [10, 20, 30; 40, 50, 60]
writematrix(E, "smart_data.csv")
disp("矩阵已智能写入 smart_data.csv")

F = readmatrix("smart_data.csv")
disp("智能读取的矩阵:")
disp(F)

# 4. MAT文件操作
disp("=== MAT文件操作 ===")
X = [1, 2; 3, 4]
Y = [5, 6; 7, 8]
Z = "hello"
save("variables.mat", X, Y, Z)
disp("变量已保存到 variables.mat")

# 5. 目录操作
disp("=== 目录操作 ===")
current = pwd()
disp("当前目录: " + current)

files = dir(".")
disp("当前目录下的文件:")
disp(files)

# 6. 路径操作
disp("=== 路径操作 ===")
path = fullfile("data", "results", "output.csv")
disp("拼接的路径: " + path)

info = fileparts("data/results/output.csv")
disp("路径分解:")
disp(info)

sep = filesep()
disp("文件分隔符: " + sep)

# 7. 文件操作
disp("=== 文件操作 ===")
exists = exist("test_matrix.csv")
disp("test_matrix.csv 是否存在: " + str(exists))

isFile = isfile("test_matrix.csv")
disp("test_matrix.csv 是否为文件: " + str(isFile))

# 8. 清理测试文件
disp("=== 清理测试文件 ===")
deletefile("test_matrix.csv")
deletefile("test_data.txt")
deletefile("smart_data.csv")
deletefile("variables.mat")
disp("测试文件已清理")

disp("=== 文件I/O演示完成 ===")
