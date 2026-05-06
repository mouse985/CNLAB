#pragma once

#include "Matrix.hpp"
#include <string>
#include <functional>

namespace cnlab {
namespace gpu {

class GPUContext {
public:
    static GPUContext& getInstance();
    
    bool initialize();
    void shutdown();
    bool isAvailable() const;
    
    void setAutoMode(bool autoMode);
    void setThreshold(size_t minElements);
    void setForceGPU(bool force);
    
    bool shouldUseGPU(size_t matrixElements) const;
    
    std::string getInfo() const;
    
private:
    GPUContext();
    ~GPUContext();
    
    bool initialized_;
    bool autoMode_;
    size_t threshold_;
    bool forceGPU_;
};

Matrix multiply(const Matrix& A, const Matrix& B);
Matrix add(const Matrix& A, const Matrix& B);
Matrix transpose(const Matrix& A);

void enableGPU();
void disableGPU();
void setGPUThreshold(size_t minElements);
std::string getGPUInfo();

}
}
