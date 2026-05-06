# CNLab 绘图示例 - 衰减振荡

# 创建时间向量
x = 0:0.01:5

# 第一个衰减振荡
y = exp(-1.5*x).*sin(10*x)
subplot(1, 2, 1)
plot(x, y)
xlabel('x')
ylabel('exp(-1.5x)*sin(10x)')
title('衰减振荡 (alpha=1.5)')
axis([0, 5, -1, 1])
grid on

# 第二个衰减振荡
y = exp(-2*x).*sin(10*x)
subplot(1, 2, 2)
plot(x, y)
xlabel('x')
ylabel('exp(-2x)*sin(10x)')
title('衰减振荡 (alpha=2.0)')
axis([0, 5, -1, 1])
grid on

# 显示图形
show()
