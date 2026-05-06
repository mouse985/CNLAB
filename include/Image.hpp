#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace cnlab {

enum class ImageFormat {
    Grayscale,
    RGB,
    RGBA
};

class Image {
public:
    Image();
    Image(size_t width, size_t height, ImageFormat format = ImageFormat::RGB);

    static Image load(const std::string& filename);
    static Image loadBMP(const std::string& filename);
    static Image loadPPM(const std::string& filename);
    static Image loadPGM(const std::string& filename);
    static Image loadSTB(const std::string& filename);

    bool save(const std::string& filename) const;
    bool saveBMP(const std::string& filename) const;
    bool savePPM(const std::string& filename) const;
    bool savePGM(const std::string& filename) const;
    bool saveASCII(const std::string& filename) const;

    size_t width() const { return width_; }
    size_t height() const { return height_; }
    size_t channels() const;
    ImageFormat format() const { return format_; }

    bool isEmpty() const { return data_.empty(); }

    unsigned char& at(size_t x, size_t y, size_t c = 0);
    unsigned char at(size_t x, size_t y, size_t c = 0) const;

    unsigned char* pixelPtr(size_t x, size_t y);
    const unsigned char* pixelPtr(size_t x, size_t y) const;

    Image toGrayscale() const;
    Image resize(size_t newWidth, size_t newHeight) const;
    Image crop(size_t x, size_t y, size_t w, size_t h) const;
    Image flipHorizontal() const;
    Image flipVertical() const;
    Image rotate90() const;
    Image rotate180() const;
    Image rotate270() const;

    void fill(unsigned char value);
    void clear();

    std::string toASCII(size_t maxWidth = 80, size_t maxHeight = 40) const;
    std::string toUnicodeBlocks(size_t maxWidth = 80, size_t maxHeight = 40) const;
    void displayInConsole(size_t maxWidth = 80, size_t maxHeight = 40) const;

    static Image createTestPattern(size_t width, size_t height);
    static Image createGradient(size_t width, size_t height);
    static Image createCheckerboard(size_t width, size_t height, size_t blockSize = 10);
    static Image createCircle(size_t width, size_t height, size_t radius);

    std::vector<double> toMatrixData() const;
    static Image fromMatrixData(const std::vector<double>& data, size_t width, size_t height, size_t channels = 1);

private:
    size_t width_;
    size_t height_;
    ImageFormat format_;
    std::vector<unsigned char> data_;

    size_t pixelIndex(size_t x, size_t y) const;
    size_t channelCount() const;
};

}
