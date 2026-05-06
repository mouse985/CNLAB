#pragma once

#include "Matrix.hpp"
#include "DataTypes.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace cnlab {

struct PathInfo {
    std::string path;
    std::string name;
    std::string ext;
    std::string filename;
    
    std::string toString() const;
};

struct FileInfo {
    std::string name;
    std::string folder;
    std::string date;
    double bytes;
    bool isdir;
    
    std::string toString() const;
};

class FileIOManager {
public:
    static Matrix csvread(const std::string& filename, size_t startRow = 0, size_t startCol = 0);
    
    static void csvwrite(const std::string& filename, const Matrix& data);
    
    static Matrix dlmread(const std::string& filename, const std::string& delimiter, 
                          size_t startRow = 0, size_t startCol = 0);
    
    static void dlmwrite(const std::string& filename, const Matrix& data, 
                         const std::string& delimiter = ",", 
                         const std::string& precision = "%.6f");
    
    static Matrix readmatrix(const std::string& filename, 
                             const std::string& delimiter = "auto");
    
    static void writematrix(const Matrix& data, const std::string& filename,
                            const std::string& delimiter = ",");
    
    static std::shared_ptr<Cell> textscan(FILE* file, const std::string& format);
    
    static void saveMAT(const std::string& filename, 
                       const std::unordered_map<std::string, Value>& variables);
    
    static std::unordered_map<std::string, Value> loadMAT(const std::string& filename);
    
    static std::vector<FileInfo> dir(const std::string& path = ".");
    
    static bool mkdir(const std::string& path);
    
    static bool rmdir(const std::string& path);
    
    static std::string pwd();
    
    static bool cd(const std::string& path);
    
    static std::string fullfile(const std::vector<std::string>& parts);
    
    static PathInfo fileparts(const std::string& filename);
    
    static std::string filesep();
    
    static bool exists(const std::string& path);
    
    static bool isfile(const std::string& path);
    
    static bool isdir(const std::string& path);
    
    static bool deletefile(const std::string& filename);
    
    static bool copyfile(const std::string& source, const std::string& destination);
    
    static bool movefile(const std::string& source, const std::string& destination);
    
private:
    static std::string currentDirectory_;
    
    static std::vector<std::string> splitLine(const std::string& line, const std::string& delimiter);
    
    static std::string trim(const std::string& str);
    
    static std::string detectDelimiter(const std::string& line);
    
    static bool parseNumber(const std::string& str, double& value);
};

}
