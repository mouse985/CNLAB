#include "Graphics/GraphicsManager.hpp"
#include "Graphics/Figure.hpp"
#include "Graphics/ImageDisplay.hpp"
#include "Image.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace cnlab;
using namespace cnlab::graphics;
namespace fs = std::filesystem;

// 获取测试图集中的所有图片
std::vector<std::string> getTestImages() {
    std::vector<std::string> images;
    std::string testDir = "测试图集";
    
    if (!fs::exists(testDir)) {
        std::cerr << "测试图集目录不存在: " << testDir << std::endl;
        return images;
    }
    
    for (const auto& entry : fs::directory_iterator(testDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            // 转换为小写
            for (auto& c : ext) c = std::tolower(c);
            
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || 
                ext == ".bmp" || ext == ".pgm" || ext == ".ppm") {
                images.push_back(entry.path().string());
            }
        }
    }
    
    return images;
}

// 获取当前目录下的测试图片
std::vector<std::string> getRootImages() {
    std::vector<std::string> images;
    
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            // 转换为小写
            for (auto& c : ext) c = std::tolower(c);
            
            if (ext == ".pgm" || ext == ".ppm") {
                images.push_back(entry.path().string());
            }
        }
    }
    
    return images;
}

int main() {
    std::cout << "=== CNLab 随机图片显示测试 ===" << std::endl;
    std::cout << std::endl;
    
    // 初始化随机数
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    
    // 初始化图形系统
    GraphicsManager::getInstance().initialize();
    
    // 收集所有可用图片
    std::vector<std::string> allImages;
    
    auto testImages = getTestImages();
    allImages.insert(allImages.end(), testImages.begin(), testImages.end());
    
    auto rootImages = getRootImages();
    allImages.insert(allImages.end(), rootImages.begin(), rootImages.end());
    
    if (allImages.empty()) {
        std::cerr << "没有找到任何图片!" << std::endl;
        return 1;
    }
    
    std::cout << "找到 " << allImages.size() << " 张图片" << std::endl;
    
    // 随机选择一张图片
    int randomIndex = std::rand() % allImages.size();
    std::string selectedImage = allImages[randomIndex];
    
    std::cout << "随机选择的图片: " << selectedImage << std::endl;
    
    try {
        // 使用 ImageDisplay 加载图片
        auto imgDisplay = std::make_unique<ImageDisplay>();
        
        std::cout << "正在加载图片..." << std::endl;
        bool loaded = false;
        
        // 首先尝试 GDI+ 直接加载
        if (imgDisplay->loadFromFile(selectedImage)) {
            loaded = true;
            std::cout << "使用 GDI+ 加载成功!" << std::endl;
        } else {
            // 尝试使用 CNLab 的 Image 类加载
            std::cout << "GDI+ 加载失败，尝试使用 Image 类..." << std::endl;
            auto img = Image::imread(selectedImage);
            if (img) {
                std::cout << "Image 类加载成功! 尺寸: " << img->width() << "x" << img->height() << std::endl;
                
                // 转换为 ImageDisplay
                std::vector<uint8_t> pixelData;
                int width = img->width();
                int height = img->height();
                
                if (img->channels() == 1) {
                    // 灰度图
                    pixelData.resize(width * height);
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            pixelData[y * width + x] = static_cast<uint8_t>(img->at(y, x) * 255);
                        }
                    }
                    loaded = imgDisplay->loadFromData(pixelData, width, height, 1);
                } else {
                    // 彩色图
                    pixelData.resize(width * height * 3);
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            int idx = (y * width + x) * 3;
                            pixelData[idx + 0] = static_cast<uint8_t>(img->at(y, x, 0) * 255);
                            pixelData[idx + 1] = static_cast<uint8_t>(img->at(y, x, 1) * 255);
                            pixelData[idx + 2] = static_cast<uint8_t>(img->at(y, x, 2) * 255);
                        }
                    }
                    loaded = imgDisplay->loadFromData(pixelData, width, height, 3);
                }
            }
        }
        
        if (!loaded) {
            std::cerr << "无法加载图片!" << std::endl;
            return 1;
        }
        
        int imgWidth = imgDisplay->getWidth();
        int imgHeight = imgDisplay->getHeight();
        std::cout << "图片尺寸: " << imgWidth << "x" << imgHeight << std::endl;
        
        // 计算窗口尺寸 (最大 1200x900，保持比例)
        int windowWidth = imgWidth;
        int windowHeight = imgHeight;
        
        if (windowWidth > 1200) {
            double scale = 1200.0 / windowWidth;
            windowWidth = 1200;
            windowHeight = static_cast<int>(windowHeight * scale);
        }
        if (windowHeight > 900) {
            double scale = 900.0 / windowHeight;
            windowHeight = 900;
            windowWidth = static_cast<int>(windowWidth * scale);
        }
        
        // 最小尺寸
        windowWidth = std::max(windowWidth, 400);
        windowHeight = std::max(windowHeight, 300);
        
        // 添加边距
        windowWidth += 100;
        windowHeight += 100;
        
        std::cout << "窗口尺寸: " << windowWidth << "x" << windowHeight << std::endl;
        
        // 创建图窗并显示图片
        Figure* fig = figure(1);
        fig->create("图片显示: " + selectedImage, windowWidth, windowHeight);
        
        // 使用 imshow 显示图片
        fig->imshow(imgDisplay.get());
        
        // 显示图窗
        fig->show();
        
        std::cout << "图片已显示!" << std::endl;
        std::cout << "按 Enter 键关闭窗口..." << std::endl;
        std::cin.get();
        
        // 保存显示的截图
        std::string outputFile = "displayed_image.png";
        if (fig->save(outputFile)) {
            std::cout << "截图已保存到: " << outputFile << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    // 关闭所有图窗
    closeAll();
    
    std::cout << "测试完成!" << std::endl;
    
    return 0;
}
