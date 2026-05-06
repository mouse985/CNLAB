#include "Graphics/GraphicsManager.hpp"
#include "Graphics/Figure.hpp"
#include "Graphics/Axes.hpp"
#include <iostream>
#include <cmath>
#include <vector>

using namespace cnlab::graphics;

void testBasicPlot() {
    std::cout << "Testing basic plot..." << std::endl;
    
    // 创建简单数据
    std::vector<double> x(100);
    std::vector<double> y(100);
    
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.1;
        y[i] = std::sin(x[i]);
    }
    
    // 创建图窗并绘图
    Figure* fig = figure(1);
    fig->create("Test Plot - Sine Wave", 800, 600);
    
    Axes* axes = fig->gca();
    axes->plot(x, y, "b-");
    axes->setTitle("Sine Wave");
    axes->setXLabel("x");
    axes->setYLabel("sin(x)");
    axes->setGrid(true);
    
    fig->show();
    
    std::cout << "Plot displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testMultipleLines() {
    std::cout << "Testing multiple lines..." << std::endl;
    
    std::vector<double> x(100);
    std::vector<double> y1(100), y2(100), y3(100);
    
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.1;
        y1[i] = std::sin(x[i]);
        y2[i] = std::cos(x[i]);
        y3[i] = std::sin(x[i]) * std::cos(x[i]);
    }
    
    Figure* fig = figure(2);
    fig->create("Multiple Lines", 800, 600);
    
    Axes* axes = fig->gca();
    axes->hold(true);
    axes->plot(x, y1, "r-");
    axes->plot(x, y2, "g--");
    axes->plot(x, y3, "b:");
    axes->setGrid(true);
    axes->legend({"sin(x)", "cos(x)", "sin(x)*cos(x)"});
    
    fig->show();
    
    std::cout << "Multiple lines displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testScatter() {
    std::cout << "Testing scatter plot..." << std::endl;
    
    std::vector<double> x(50);
    std::vector<double> y(50);
    
    for (int i = 0; i < 50; ++i) {
        x[i] = i;
        y[i] = std::rand() % 100;
    }
    
    Figure* fig = figure(3);
    fig->create("Scatter Plot", 800, 600);
    
    fig->scatter(x, y, "o");
    
    fig->show();
    
    std::cout << "Scatter plot displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testBar() {
    std::cout << "Testing bar chart..." << std::endl;
    
    std::vector<double> y = {23, 45, 56, 78, 32, 67, 89, 45, 23, 67};
    
    Figure* fig = figure(4);
    fig->create("Bar Chart", 800, 600);
    
    fig->bar(y);
    
    fig->show();
    
    std::cout << "Bar chart displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testSubplot() {
    std::cout << "Testing subplot..." << std::endl;
    
    std::vector<double> x(100);
    std::vector<double> y1(100), y2(100), y3(100), y4(100);
    
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.1;
        y1[i] = std::sin(x[i]);
        y2[i] = std::cos(x[i]);
        y3[i] = std::tan(x[i]);
        y4[i] = std::exp(-x[i]);
    }
    
    Figure* fig = figure(5);
    fig->create("Subplot Demo", 1000, 800);
    
    fig->subplot(2, 2, 1);
    fig->plot(x, y1, "r-");
    title("sin(x)");
    
    fig->subplot(2, 2, 2);
    fig->plot(x, y2, "g-");
    title("cos(x)");
    
    fig->subplot(2, 2, 3);
    fig->plot(x, y3, "b-");
    ylim(-5, 5);
    title("tan(x)");
    
    fig->subplot(2, 2, 4);
    fig->plot(x, y4, "m-");
    title("exp(-x)");
    
    fig->show();
    
    std::cout << "Subplot displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testHistogram() {
    std::cout << "Testing histogram..." << std::endl;
    
    std::vector<double> data(1000);
    for (int i = 0; i < 1000; ++i) {
        // 生成正态分布数据
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        data[i] = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    }
    
    Figure* fig = figure(6);
    fig->create("Histogram", 800, 600);
    
    Axes* axes = fig->gca();
    axes->hist(data, 20);
    axes->setTitle("Normal Distribution");
    
    fig->show();
    
    std::cout << "Histogram displayed. Press Enter to continue..." << std::endl;
    std::cin.get();
}

void testSaveFigure() {
    std::cout << "Testing save figure..." << std::endl;
    
    std::vector<double> x(100);
    std::vector<double> y(100);
    
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.1;
        y[i] = std::sin(x[i]) * std::exp(-x[i] * 0.1);
    }
    
    Figure* fig = figure(7);
    fig->create("Save Test", 800, 600);
    
    fig->plot(x, y, "r-");
    title("Damped Sine Wave");
    grid(true);
    
    fig->show();
    
    // 保存为图片
    if (fig->save("test_figure.png")) {
        std::cout << "Figure saved to test_figure.png" << std::endl;
    } else {
        std::cout << "Failed to save figure" << std::endl;
    }
    
    std::cout << "Press Enter to continue..." << std::endl;
    std::cin.get();
}

int main() {
    std::cout << "=== CNLab Graphics Library Test ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化图形系统
    GraphicsManager::getInstance().initialize();
    
    try {
        testBasicPlot();
        testMultipleLines();
        testScatter();
        testBar();
        testSubplot();
        testHistogram();
        testSaveFigure();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // 关闭所有图窗
    closeAll();
    
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}
