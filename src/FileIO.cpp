#include "FileIO.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #define PATH_SEPARATOR '\\'
    #define mkdir_func(path, mode) _mkdir(path)
    #define rmdir_func _rmdir
    #define getcwd_func _getcwd
    #define chdir_func _chdir
    #define access_func _access
    #define F_OK 0
    #define W_OK 2
    #define R_OK 4
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <cstring>
    #define PATH_SEPARATOR '/'
    #define mkdir_func mkdir
    #define rmdir_func rmdir
    #define getcwd_func getcwd
    #define chdir_func chdir
    #define access_func access
#endif

namespace cnlab {

std::string FileIOManager::currentDirectory_ = "";

std::string PathInfo::toString() const {
    std::ostringstream oss;
    oss << "PathInfo:\n";
    oss << "  path: " << path << "\n";
    oss << "  name: " << name << "\n";
    oss << "  ext: " << ext << "\n";
    oss << "  filename: " << filename << "\n";
    return oss.str();
}

std::string FileInfo::toString() const {
    std::ostringstream oss;
    oss << name;
    if (isdir) oss << "/";
    oss << " (" << bytes << " bytes, " << date << ")";
    return oss.str();
}

std::string FileIOManager::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> FileIOManager::splitLine(const std::string& line, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = line.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(trim(line.substr(start, end - start)));
        start = end + delimiter.length();
        end = line.find(delimiter, start);
    }
    tokens.push_back(trim(line.substr(start)));
    
    return tokens;
}

std::string FileIOManager::detectDelimiter(const std::string& line) {
    std::vector<std::pair<std::string, int>> delimiters = {
        {",", 0}, {"\t", 0}, {";", 0}, {" ", 0}
    };
    
    for (char c : line) {
        for (auto& delim : delimiters) {
            if (std::string(1, c) == delim.first) {
                delim.second++;
            }
        }
    }
    
    auto maxDelim = std::max_element(delimiters.begin(), delimiters.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return maxDelim->first;
}

bool FileIOManager::parseNumber(const std::string& str, double& value) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) return false;
    
    try {
        size_t pos;
        value = std::stod(trimmed, &pos);
        return pos == trimmed.length();
    } catch (...) {
        return false;
    }
}

Matrix FileIOManager::csvread(const std::string& filename, size_t startRow, size_t startCol) {
    return dlmread(filename, ",", startRow, startCol);
}

void FileIOManager::csvwrite(const std::string& filename, const Matrix& data) {
    dlmwrite(filename, data, ",", "%.6f");
}

Matrix FileIOManager::dlmread(const std::string& filename, const std::string& delimiter,
                              size_t startRow, size_t startCol) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::vector<std::vector<double>> data;
    std::string line;
    size_t lineNum = 0;
    
    while (std::getline(file, line)) {
        if (lineNum < startRow) {
            lineNum++;
            continue;
        }
        
        line = trim(line);
        if (line.empty()) {
            lineNum++;
            continue;
        }
        
        std::vector<std::string> tokens = splitLine(line, delimiter);
        std::vector<double> row;
        
        for (size_t i = startCol; i < tokens.size(); ++i) {
            double value;
            if (parseNumber(tokens[i], value)) {
                row.push_back(value);
            } else {
                row.push_back(std::nan(""));
            }
        }
        
        if (!row.empty()) {
            data.push_back(row);
        }
        
        lineNum++;
    }
    
    file.close();
    
    if (data.empty()) {
        return Matrix();
    }
    
    size_t maxCols = 0;
    for (const auto& row : data) {
        maxCols = (std::max)(maxCols, row.size());
    }
    
    Matrix result(data.size(), maxCols);
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            result(i, j) = data[i][j];
        }
    }
    
    return result;
}

void FileIOManager::dlmwrite(const std::string& filename, const Matrix& data,
                             const std::string& delimiter, const std::string& precision) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    file << std::setprecision(6) << std::fixed;
    
    for (size_t i = 0; i < data.rows(); ++i) {
        for (size_t j = 0; j < data.cols(); ++j) {
            if (j > 0) file << delimiter;
            file << data(i, j);
        }
        file << "\n";
    }
    
    file.close();
}

Matrix FileIOManager::readmatrix(const std::string& filename, const std::string& delimiter) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string firstLine;
    std::getline(file, firstLine);
    file.close();
    
    std::string delim = delimiter;
    if (delim == "auto") {
        delim = detectDelimiter(firstLine);
    }
    
    return dlmread(filename, delim, 0, 0);
}

void FileIOManager::writematrix(const Matrix& data, const std::string& filename,
                                const std::string& delimiter) {
    dlmwrite(filename, data, delimiter, "%.6f");
}

std::shared_ptr<Cell> FileIOManager::textscan(FILE* file, const std::string& format) {
    if (!file) {
        throw std::runtime_error("Invalid file handle");
    }
    
    auto result = std::make_shared<Cell>();
    
    std::vector<std::string> formatSpecs;
    size_t i = 0;
    while (i < format.length()) {
        if (format[i] == '%' && i + 1 < format.length()) {
            char spec = format[i + 1];
            if (spec == 'd' || spec == 'f' || spec == 's' || spec == 'c') {
                formatSpecs.push_back(std::string(1, spec));
            }
            i += 2;
        } else {
            i++;
        }
    }
    
    if (formatSpecs.empty()) {
        return result;
    }
    
    std::vector<std::vector<Value>> columns(formatSpecs.size());
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file)) {
        std::string line(buffer);
        std::istringstream iss(line);
        
        for (size_t col = 0; col < formatSpecs.size(); ++col) {
            if (formatSpecs[col] == "d") {
                int val;
                if (iss >> val) {
                    columns[col].push_back(static_cast<double>(val));
                }
            } else if (formatSpecs[col] == "f") {
                double val;
                if (iss >> val) {
                    columns[col].push_back(val);
                }
            } else if (formatSpecs[col] == "s") {
                std::string val;
                if (iss >> val) {
                    columns[col].push_back(val);
                }
            } else if (formatSpecs[col] == "c") {
                char val;
                if (iss >> val) {
                    columns[col].push_back(std::string(1, val));
                }
            }
        }
    }
    
    for (size_t col = 0; col < columns.size(); ++col) {
        if (!columns[col].empty()) {
            // Create a cell array for this column
            auto colCell = std::make_shared<Cell>(columns[col].size());
            for (size_t i = 0; i < columns[col].size(); ++i) {
                colCell->setElement(i + 1, columns[col][i]);
            }
            result->setElement(col + 1, colCell);
        }
    }
    
    return result;
}

void FileIOManager::saveMAT(const std::string& filename,
                           const std::unordered_map<std::string, Value>& variables) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    const char* header = "CNLAB_MAT_v1.0\n";
    file.write(header, strlen(header));
    
    uint32_t varCount = static_cast<uint32_t>(variables.size());
    file.write(reinterpret_cast<const char*>(&varCount), sizeof(varCount));
    
    for (const auto& [name, value] : variables) {
        uint32_t nameLen = static_cast<uint32_t>(name.length());
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(name.c_str(), nameLen);
        
        if (std::holds_alternative<Matrix>(value)) {
            const Matrix& mat = std::get<Matrix>(value);
            char type = 'M';
            file.write(&type, sizeof(type));
            
            uint32_t rows = static_cast<uint32_t>(mat.rows());
            uint32_t cols = static_cast<uint32_t>(mat.cols());
            file.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
            file.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
            
            for (size_t i = 0; i < mat.rows(); ++i) {
                for (size_t j = 0; j < mat.cols(); ++j) {
                    double val = mat(i, j);
                    file.write(reinterpret_cast<const char*>(&val), sizeof(val));
                }
            }
        } else if (std::holds_alternative<double>(value)) {
            char type = 'D';
            file.write(&type, sizeof(type));
            
            double val = std::get<double>(value);
            file.write(reinterpret_cast<const char*>(&val), sizeof(val));
        } else if (std::holds_alternative<std::string>(value)) {
            const std::string& str = std::get<std::string>(value);
            char type = 'S';
            file.write(&type, sizeof(type));
            
            uint32_t len = static_cast<uint32_t>(str.length());
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(str.c_str(), len);
        }
    }
    
    file.close();
}

std::unordered_map<std::string, Value> FileIOManager::loadMAT(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    char header[16];
    file.getline(header, sizeof(header));
    
    if (std::string(header) != "CNLAB_MAT_v1.0") {
        throw std::runtime_error("Invalid MAT file format");
    }
    
    uint32_t varCount;
    file.read(reinterpret_cast<char*>(&varCount), sizeof(varCount));
    
    std::unordered_map<std::string, Value> variables;
    
    for (uint32_t i = 0; i < varCount; ++i) {
        uint32_t nameLen;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        
        std::string name(nameLen, '\0');
        file.read(&name[0], nameLen);
        
        char type;
        file.read(&type, sizeof(type));
        
        if (type == 'M') {
            uint32_t rows, cols;
            file.read(reinterpret_cast<char*>(&rows), sizeof(rows));
            file.read(reinterpret_cast<char*>(&cols), sizeof(cols));
            
            Matrix mat(rows, cols);
            for (size_t r = 0; r < rows; ++r) {
                for (size_t c = 0; c < cols; ++c) {
                    double val;
                    file.read(reinterpret_cast<char*>(&val), sizeof(val));
                    mat(r, c) = val;
                }
            }
            
            variables[name] = mat;
        } else if (type == 'D') {
            double val;
            file.read(reinterpret_cast<char*>(&val), sizeof(val));
            variables[name] = val;
        } else if (type == 'S') {
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            
            std::string str(len, '\0');
            file.read(&str[0], len);
            variables[name] = str;
        }
    }
    
    file.close();
    return variables;
}

std::vector<FileInfo> FileIOManager::dir(const std::string& path) {
    std::vector<FileInfo> result;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = path + "\\*";
    
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return result;
    }
    
    do {
        FileInfo info;
        info.name = findData.cFileName;
        info.isdir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        if (!info.isdir) {
            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            info.bytes = static_cast<double>(fileSize.QuadPart);
        } else {
            info.bytes = 0;
        }
        
        FILETIME ft = findData.ftLastWriteTime;
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        char dateStr[64];
        snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d %02d:%02d:%02d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        info.date = dateStr;
        
        info.folder = path;
        
        result.push_back(info);
    } while (FindNextFileA(hFind, &findData) != 0);
    
    FindClose(hFind);
#else
    DIR* dirp = opendir(path.c_str());
    if (!dirp) {
        return result;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dirp)) != nullptr) {
        FileInfo info;
        info.name = entry->d_name;
        
        std::string fullPath = path + "/" + info.name;
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0) {
            info.isdir = S_ISDIR(statbuf.st_mode);
            info.bytes = info.isdir ? 0 : static_cast<double>(statbuf.st_size);
            
            char dateStr[64];
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S",
                    localtime(&statbuf.st_mtime));
            info.date = dateStr;
        }
        
        info.folder = path;
        result.push_back(info);
    }
    
    closedir(dirp);
#endif
    
    return result;
}

bool FileIOManager::mkdir(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return ::mkdir(path.c_str(), 0755) == 0;
#endif
}

bool FileIOManager::rmdir(const std::string& path) {
#ifdef _WIN32
    return _rmdir(path.c_str()) == 0;
#else
    return ::rmdir(path.c_str()) == 0;
#endif
}

std::string FileIOManager::pwd() {
    if (currentDirectory_.empty()) {
        char buffer[4096];
        if (getcwd_func(buffer, sizeof(buffer))) {
            currentDirectory_ = buffer;
        }
    }
    return currentDirectory_;
}

bool FileIOManager::cd(const std::string& path) {
    if (chdir_func(path.c_str()) == 0) {
        currentDirectory_ = pwd();
        return true;
    }
    return false;
}

std::string FileIOManager::fullfile(const std::vector<std::string>& parts) {
    if (parts.empty()) return "";
    
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        if (!result.empty() && result.back() != PATH_SEPARATOR) {
            result += PATH_SEPARATOR;
        }
        result += parts[i];
    }
    
    return result;
}

PathInfo FileIOManager::fileparts(const std::string& filename) {
    PathInfo info;
    info.filename = filename;
    
    size_t lastSep = filename.find_last_of("/\\");
    if (lastSep != std::string::npos) {
        info.path = filename.substr(0, lastSep);
        info.name = filename.substr(lastSep + 1);
    } else {
        info.path = "";
        info.name = filename;
    }
    
    size_t lastDot = info.name.find_last_of('.');
    if (lastDot != std::string::npos && lastDot > 0) {
        info.ext = info.name.substr(lastDot);
        info.name = info.name.substr(0, lastDot);
    } else {
        info.ext = "";
    }
    
    return info;
}

std::string FileIOManager::filesep() {
    return std::string(1, PATH_SEPARATOR);
}

bool FileIOManager::exists(const std::string& path) {
    return access_func(path.c_str(), F_OK) == 0;
}

bool FileIOManager::isfile(const std::string& path) {
#ifdef _WIN32
    struct _stat statbuf;
    if (_stat(path.c_str(), &statbuf) != 0) return false;
    return (statbuf.st_mode & _S_IFREG) != 0;
#else
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) return false;
    return S_ISREG(statbuf.st_mode);
#endif
}

bool FileIOManager::isdir(const std::string& path) {
#ifdef _WIN32
    struct _stat statbuf;
    if (_stat(path.c_str(), &statbuf) != 0) return false;
    return (statbuf.st_mode & _S_IFDIR) != 0;
#else
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0) return false;
    return S_ISDIR(statbuf.st_mode);
#endif
}

bool FileIOManager::deletefile(const std::string& filename) {
    return std::remove(filename.c_str()) == 0;
}

bool FileIOManager::copyfile(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    if (!src.is_open()) return false;
    
    std::ofstream dst(destination, std::ios::binary);
    if (!dst.is_open()) {
        src.close();
        return false;
    }
    
    dst << src.rdbuf();
    
    src.close();
    dst.close();
    
    return true;
}

bool FileIOManager::movefile(const std::string& source, const std::string& destination) {
    if (copyfile(source, destination)) {
        return deletefile(source);
    }
    return false;
}

}
