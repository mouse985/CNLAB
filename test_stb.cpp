#include <iostream>
#include "stb_image.h"

int main() {
    int width, height, channels;
    const char* filename = "测试图集/true.jpg";
    
    std::cout << "Loading: " << filename << std::endl;
    
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    
    if (data) {
        std::cout << "Success! " << width << "x" << height << " channels: " << channels << std::endl;
        stbi_image_free(data);
        return 0;
    } else {
        std::cout << "Failed to load image" << std::endl;
        return 1;
    }
}
