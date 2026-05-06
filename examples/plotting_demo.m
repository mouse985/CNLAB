# CNLab 绘图功能演示

# 1. 基础线图
disp("=== 基础线图 ===")
x = 0:0.1:2*pi
y = sin(x)
plot(x, y, "r-")
title("正弦函数")
xlabel("x")
ylabel("sin(x)")
grid on
show()

# 2. 多曲线绘图
disp("=== 多曲线绘图 ===")
x = 0:0.1:2*pi
y1 = sin(x)
y2 = cos(x)
plot(x, y1, "r-", x, y2, "b--")
title("正弦和余弦函数")
xlabel("x")
ylabel("y")
legend("sin(x)", "cos(x)")
grid on
show()

# 3. 不同线型
disp("=== 不同线型 ===")
x = 0:0.2:10
y1 = x
y2 = x.^2
y3 = sqrt(x)
plot(x, y1, "r-", x, y2, "g--", x, y3, "b-.")
title("不同函数曲线")
xlabel("x")
ylabel("y")
legend("y = x", "y = x^2", "y = sqrt(x)")
grid on
show()

# 4. 散点图
disp("=== 散点图 ===")
x = rand(1, 50)
y = rand(1, 50)
scatter(x, y, "ro")
title("随机散点图")
xlabel("x")
ylabel("y")
grid on
show()

# 5. 柱状图
disp("=== 柱状图 ===")
categories = [1, 2, 3, 4, 5]
values = [10, 25, 15, 30, 20]
bar(categories, values)
title("柱状图示例")
xlabel("类别")
ylabel("数值")
grid on
show()

# 6. 直方图
disp("=== 直方图 ===")
data = randn(1, 1000)
hist(data, 30)
title("正态分布直方图")
xlabel("数值")
ylabel("频数")
grid on
show()

# 7. 子图功能
disp("=== 子图功能 ===")
x = 0:0.1:2*pi

subplot(2, 2, 1)
y1 = sin(x)
plot(x, y1, "r-")
title("sin(x)")
grid on

subplot(2, 2, 2)
y2 = cos(x)
plot(x, y2, "b--")
title("cos(x)")
grid on

subplot(2, 2, 3)
y3 = tan(x)
plot(x, y3, "g-.")
title("tan(x)")
grid on

subplot(2, 2, 4)
y4 = sin(x) .* cos(x)
plot(x, y4, "m:")
title("sin(x)*cos(x)")
grid on

show()

# 8. 坐标轴控制
disp("=== 坐标轴控制 ===")
x = 0:0.1:10
y = exp(-x) .* sin(x)
plot(x, y, "b-")
title("衰减振荡")
xlabel("x")
ylabel("y")
xlim(0, 10)
ylim(-0.5, 0.5)
grid on
show()

# 9. hold功能
disp("=== hold功能 ===")
x = 0:0.1:2*pi
y1 = sin(x)
y2 = cos(x)

plot(x, y1, "r-")
hold on
plot(x, y2, "b--")
hold off

title("使用hold叠加曲线")
xlabel("x")
ylabel("y")
legend("sin(x)", "cos(x)")
grid on
show()

# 10. 图形窗口管理
disp("=== 图形窗口管理 ===")
figure(1)
x = 0:0.1:2*pi
plot(x, sin(x), "r-")
title("Figure 1: sin(x)")
show()

figure(2)
plot(x, cos(x), "b--")
title("Figure 2: cos(x)")
show()

disp("绘图功能演示完成！")
