#include "Image.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <cstring>

// STB Image for JPG/PNG support
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace cnlab {

Image::Image() : width_(0), height_(0), format_(ImageFormat::RGB) {}

Image::Image(size_t width, size_t height, ImageFormat format)
    : width_(width), height_(height), format_(format) {
    data_.resize(width * height * channelCount(), 0);
}

size_t Image::channelCount() const {
    switch (format_) {
        case ImageFormat::Grayscale: return 1;
        case ImageFormat::RGB: return 3;
        case ImageFormat::RGBA: return 4;
    }
    return 3;
}

size_t Image::channels() const {
    return channelCount();
}

size_t Image::pixelIndex(size_t x, size_t y) const {
    return (y * width_ + x) * channelCount();
}

unsigned char& Image::at(size_t x, size_t y, size_t c) {
    if (x >= width_ || y >= height_ || c >= channelCount()) {
        throw std::out_of_range("Image index out of bounds");
    }
    return data_[pixelIndex(x, y) + c];
}

unsigned char Image::at(size_t x, size_t y, size_t c) const {
    if (x >= width_ || y >= height_ || c >= channelCount()) {
        throw std::out_of_range("Image index out of bounds");
    }
    return data_[pixelIndex(x, y) + c];
}

unsigned char* Image::pixelPtr(size_t x, size_t y) {
    return &data_[pixelIndex(x, y)];
}

const unsigned char* Image::pixelPtr(size_t x, size_t y) const {
    return &data_[pixelIndex(x, y)];
}

void Image::fill(unsigned char value) {
    std::fill(data_.begin(), data_.end(), value);
}

void Image::clear() {
    data_.clear();
    width_ = 0;
    height_ = 0;
}

Image Image::load(const std::string& filename) {
    size_t dotPos = filename.rfind('.');
    if (dotPos == std::string::npos) {
        throw std::runtime_error("Cannot determine image format from filename");
    }

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "bmp") {
        return loadBMP(filename);
    } else if (ext == "ppm" || ext == "pbm" || ext == "pgm") {
        return loadPPM(filename);
    } else if (ext == "jpg" || ext == "jpeg" || ext == "png") {
        return loadSTB(filename);
    } else {
        throw std::runtime_error("Unsupported image format: " + ext);
    }
}

Image Image::loadSTB(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        const char* error = stbi_failure_reason();
        throw std::runtime_error("Failed to load image: " + filename + " - " + (error ? error : "unknown error"));
    }
    
    ImageFormat format;
    if (channels == 1) {
        format = ImageFormat::Grayscale;
    } else if (channels == 4) {
        format = ImageFormat::RGBA;
    } else {
        format = ImageFormat::RGB;
        channels = 3;
    }
    
    Image img(width, height, format);
    
    // Copy data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                img.at(x, y, c) = data[(y * width + x) * channels + c];
            }
        }
    }
    
    stbi_image_free(data);
    return img;
}

Image Image::loadBMP(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open BMP file: " + filename);
    }

    unsigned char header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    if (header[0] != 'B' || header[1] != 'M') {
        throw std::runtime_error("Invalid BMP file");
    }

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    short bitsPerPixel = *(short*)&header[28];

    if (bitsPerPixel != 24 && bitsPerPixel != 32) {
        throw std::runtime_error("Only 24-bit and 32-bit BMP files are supported");
    }

    ImageFormat format = (bitsPerPixel == 32) ? ImageFormat::RGBA : ImageFormat::RGB;
    Image img(width, std::abs(height), format);

    int rowSize = ((bitsPerPixel * width + 31) / 32) * 4;
    std::vector<unsigned char> row(rowSize);

    bool topDown = height < 0;
    height = std::abs(height);

    for (int y = 0; y < height; ++y) {
        file.read(reinterpret_cast<char*>(row.data()), rowSize);
        int targetY = topDown ? y : (height - 1 - y);

        for (int x = 0; x < width; ++x) {
            unsigned char b = row[x * (bitsPerPixel / 8)];
            unsigned char g = row[x * (bitsPerPixel / 8) + 1];
            unsigned char r = row[x * (bitsPerPixel / 8) + 2];

            img.at(x, targetY, 0) = r;
            img.at(x, targetY, 1) = g;
            img.at(x, targetY, 2) = b;
        }
    }

    return img;
}

Image Image::loadPPM(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open PPM file: " + filename);
    }

    std::string magic;
    file >> magic;

    if (magic != "P6" && magic != "P5") {
        throw std::runtime_error("Only binary PPM (P6) and PGM (P5) formats are supported");
    }

    int width, height, maxVal;
    file >> width >> height >> maxVal;
    file.get();

    bool isRGB = (magic == "P6");
    ImageFormat format = isRGB ? ImageFormat::RGB : ImageFormat::Grayscale;
    Image img(width, height, format);

    size_t pixelSize = isRGB ? 3 : 1;
    std::vector<unsigned char> buffer(width * height * pixelSize);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (y * width + x) * pixelSize;
            if (isRGB) {
                img.at(x, y, 0) = buffer[idx];
                img.at(x, y, 1) = buffer[idx + 1];
                img.at(x, y, 2) = buffer[idx + 2];
            } else {
                img.at(x, y, 0) = buffer[idx];
            }
        }
    }

    return img;
}

Image Image::loadPGM(const std::string& filename) {
    return loadPPM(filename);
}

bool Image::save(const std::string& filename) const {
    size_t dotPos = filename.rfind('.');
    if (dotPos == std::string::npos) {
        return false;
    }

    std::string ext = filename.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "bmp") {
        return saveBMP(filename);
    } else if (ext == "ppm") {
        return savePPM(filename);
    } else if (ext == "pgm") {
        return savePGM(filename);
    } else if (ext == "txt") {
        return saveASCII(filename);
    }
    return false;
}

bool Image::saveBMP(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    int rowSize = ((3 * static_cast<int>(width_) + 3) / 4) * 4;
    int imageSize = rowSize * static_cast<int>(height_);
    int fileSize = 54 + imageSize;

    unsigned char header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    *(int*)&header[2] = fileSize;
    *(int*)&header[10] = 54;
    *(int*)&header[14] = 40;
    *(int*)&header[18] = static_cast<int>(width_);
    *(int*)&header[22] = static_cast<int>(height_);
    header[26] = 1;
    header[28] = 24;
    *(int*)&header[34] = imageSize;

    file.write(reinterpret_cast<char*>(header), 54);

    std::vector<unsigned char> row(rowSize, 0);
    for (int y = static_cast<int>(height_) - 1; y >= 0; --y) {
        for (size_t x = 0; x < width_; ++x) {
            unsigned char r = at(x, y, 0);
            unsigned char g = (channels() > 1) ? at(x, y, 1) : r;
            unsigned char b = (channels() > 2) ? at(x, y, 2) : r;
            row[x * 3] = b;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = r;
        }
        file.write(reinterpret_cast<char*>(row.data()), rowSize);
    }

    return true;
}

bool Image::savePPM(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    file << "P6\n" << width_ << " " << height_ << "\n255\n";

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            unsigned char r = at(x, y, 0);
            unsigned char g = (channels() > 1) ? at(x, y, 1) : r;
            unsigned char b = (channels() > 2) ? at(x, y, 2) : r;
            file.put(r);
            file.put(g);
            file.put(b);
        }
    }

    return true;
}

bool Image::savePGM(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    file << "P5\n" << width_ << " " << height_ << "\n255\n";

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            unsigned char gray = at(x, y, 0);
            file.put(gray);
        }
    }

    return true;
}

bool Image::saveASCII(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << toASCII(80, 40);
    return true;
}

Image Image::toGrayscale() const {
    Image img(width_, height_, ImageFormat::Grayscale);

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            if (channels() == 1) {
                img.at(x, y, 0) = at(x, y, 0);
            } else {
                unsigned char r = at(x, y, 0);
                unsigned char g = at(x, y, 1);
                unsigned char b = at(x, y, 2);
                img.at(x, y, 0) = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
            }
        }
    }

    return img;
}

Image Image::resize(size_t newWidth, size_t newHeight) const {
    Image img(newWidth, newHeight, format_);

    double xScale = static_cast<double>(width_) / newWidth;
    double yScale = static_cast<double>(height_) / newHeight;

    for (size_t y = 0; y < newHeight; ++y) {
        for (size_t x = 0; x < newWidth; ++x) {
            size_t srcX = static_cast<size_t>(x * xScale);
            size_t srcY = static_cast<size_t>(y * yScale);
            srcX = std::min(srcX, width_ - 1);
            srcY = std::min(srcY, height_ - 1);

            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(x, y, c) = at(srcX, srcY, c);
            }
        }
    }

    return img;
}

Image Image::crop(size_t x, size_t y, size_t w, size_t h) const {
    Image img(w, h, format_);

    for (size_t dy = 0; dy < h; ++dy) {
        for (size_t dx = 0; dx < w; ++dx) {
            size_t srcX = std::min(x + dx, width_ - 1);
            size_t srcY = std::min(y + dy, height_ - 1);

            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(dx, dy, c) = at(srcX, srcY, c);
            }
        }
    }

    return img;
}

Image Image::flipHorizontal() const {
    Image img(width_, height_, format_);

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(x, y, c) = at(width_ - 1 - x, y, c);
            }
        }
    }

    return img;
}

Image Image::flipVertical() const {
    Image img(width_, height_, format_);

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(x, y, c) = at(x, height_ - 1 - y, c);
            }
        }
    }

    return img;
}

Image Image::rotate90() const {
    Image img(height_, width_, format_);

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(height_ - 1 - y, x, c) = at(x, y, c);
            }
        }
    }

    return img;
}

Image Image::rotate180() const {
    return flipHorizontal().flipVertical();
}

Image Image::rotate270() const {
    Image img(height_, width_, format_);

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t c = 0; c < channelCount(); ++c) {
                img.at(y, width_ - 1 - x, c) = at(x, y, c);
            }
        }
    }

    return img;
}

std::string Image::toASCII(size_t maxWidth, size_t maxHeight) const {
    const char* asciiChars = " .:-=+*#%@";
    const size_t numChars = strlen(asciiChars);

    size_t displayWidth = std::min(width_, maxWidth);
    size_t displayHeight = std::min(height_, maxHeight);

    Image resized = resize(displayWidth, displayHeight);
    Image gray = resized.toGrayscale();

    std::ostringstream oss;
    oss << "Image (" << width_ << "x" << height_ << ")\n";
    oss << "+" << std::string(displayWidth, '-') << "+\n";

    for (size_t y = 0; y < displayHeight; ++y) {
        oss << "|";
        for (size_t x = 0; x < displayWidth; ++x) {
            unsigned char pixel = gray.at(x, y, 0);
            size_t charIndex = (pixel * (numChars - 1)) / 255;
            oss << asciiChars[charIndex];
        }
        oss << "|\n";
    }

    oss << "+" << std::string(displayWidth, '-') << "+\n";
    return oss.str();
}

std::string Image::toUnicodeBlocks(size_t maxWidth, size_t maxHeight) const {
    size_t displayWidth = std::min(width_, maxWidth);
    size_t displayHeight = std::min(height_, maxHeight * 2);

    Image resized = resize(displayWidth, displayHeight);
    Image gray = resized.toGrayscale();

    const char* blocks = " .:-=+*#%@";

    std::ostringstream oss;
    oss << "Image (" << width_ << "x" << height_ << ")\n";

    for (size_t y = 0; y < displayHeight; y += 2) {
        for (size_t x = 0; x < displayWidth; ++x) {
            unsigned char top = gray.at(x, y, 0);
            unsigned char bottom = (y + 1 < displayHeight) ? gray.at(x, y + 1, 0) : 0;

            int intensity = ((top + bottom) / 2) * 10 / 255;
            oss << blocks[intensity];
        }
        oss << "\n";
    }

    return oss.str();
}

void Image::displayInConsole(size_t maxWidth, size_t maxHeight) const {
    std::cout << toASCII(maxWidth, maxHeight);
}

Image Image::createTestPattern(size_t width, size_t height) {
    Image img(width, height, ImageFormat::RGB);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            img.at(x, y, 0) = static_cast<unsigned char>((x * 255) / width);
            img.at(x, y, 1) = static_cast<unsigned char>((y * 255) / height);
            img.at(x, y, 2) = 128;
        }
    }

    return img;
}

Image Image::createGradient(size_t width, size_t height) {
    Image img(width, height, ImageFormat::Grayscale);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            unsigned char val = static_cast<unsigned char>((x + y) * 255 / (width + height));
            img.at(x, y, 0) = val;
        }
    }

    return img;
}

Image Image::createCheckerboard(size_t width, size_t height, size_t blockSize) {
    Image img(width, height, ImageFormat::Grayscale);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            bool isWhite = ((x / blockSize) + (y / blockSize)) % 2 == 0;
            img.at(x, y, 0) = isWhite ? 255 : 0;
        }
    }

    return img;
}

Image Image::createCircle(size_t width, size_t height, size_t radius) {
    Image img(width, height, ImageFormat::Grayscale);
    img.fill(0);

    size_t cx = width / 2;
    size_t cy = height / 2;

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            double dx = static_cast<double>(x) - cx;
            double dy = static_cast<double>(y) - cy;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= radius) {
                img.at(x, y, 0) = 255;
            }
        }
    }

    return img;
}

std::vector<double> Image::toMatrixData() const {
    std::vector<double> data;
    data.reserve(width_ * height_ * channelCount());

    for (size_t y = 0; y < height_; ++y) {
        for (size_t x = 0; x < width_; ++x) {
            for (size_t c = 0; c < channelCount(); ++c) {
                data.push_back(static_cast<double>(at(x, y, c)));
            }
        }
    }

    return data;
}

Image Image::fromMatrixData(const std::vector<double>& data, size_t width, size_t height, size_t channels) {
    ImageFormat format = (channels == 1) ? ImageFormat::Grayscale : ImageFormat::RGB;
    Image img(width, height, format);

    size_t idx = 0;
    for (size_t y = 0; y < height && idx < data.size(); ++y) {
        for (size_t x = 0; x < width && idx < data.size(); ++x) {
            for (size_t c = 0; c < channels && idx < data.size(); ++c) {
                img.at(x, y, c) = static_cast<unsigned char>(std::clamp(data[idx++], 0.0, 255.0));
            }
        }
    }

    return img;
}

}
