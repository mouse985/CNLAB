#include <iostream>
#include <cassert>
#include <fstream>
#include <cmath>
#include "Matrix.hpp"
#include "FileIO.hpp"

using namespace cnlab;

void testCSVReadWrite() {
    std::cout << "Testing CSV read/write..." << std::endl;
    
    Matrix data(3, 3);
    data(0, 0) = 1.0; data(0, 1) = 2.0; data(0, 2) = 3.0;
    data(1, 0) = 4.0; data(1, 1) = 5.0; data(1, 2) = 6.0;
    data(2, 0) = 7.0; data(2, 1) = 8.0; data(2, 2) = 9.0;
    
    FileIOManager::csvwrite("test_data.csv", data);
    
    Matrix loaded = FileIOManager::csvread("test_data.csv");
    
    assert(loaded.rows() == data.rows());
    assert(loaded.cols() == data.cols());
    
    for (size_t i = 0; i < data.rows(); ++i) {
        for (size_t j = 0; j < data.cols(); ++j) {
            assert(std::abs(loaded(i, j) - data(i, j)) < 1e-6);
        }
    }
    
    std::cout << "CSV read/write test passed!" << std::endl;
}

void testDLMReadWrite() {
    std::cout << "Testing DLM read/write..." << std::endl;
    
    Matrix data(2, 4);
    data(0, 0) = 1.5; data(0, 1) = 2.5; data(0, 2) = 3.5; data(0, 3) = 4.5;
    data(1, 0) = 5.5; data(1, 1) = 6.5; data(1, 2) = 7.5; data(1, 3) = 8.5;
    
    FileIOManager::dlmwrite("test_data.txt", data, "\t", "%.2f");
    
    Matrix loaded = FileIOManager::dlmread("test_data.txt", "\t");
    
    assert(loaded.rows() == data.rows());
    assert(loaded.cols() == data.cols());
    
    for (size_t i = 0; i < data.rows(); ++i) {
        for (size_t j = 0; j < data.cols(); ++j) {
            assert(std::abs(loaded(i, j) - data(i, j)) < 0.01);
        }
    }
    
    std::cout << "DLM read/write test passed!" << std::endl;
}

void testReadMatrix() {
    std::cout << "Testing readmatrix..." << std::endl;
    
    std::ofstream file("test_matrix.txt");
    file << "1 2 3\n4 5 6\n7 8 9\n";
    file.close();
    
    Matrix data = FileIOManager::readmatrix("test_matrix.txt", " ");
    
    assert(data.rows() == 3);
    assert(data.cols() == 3);
    assert(std::abs(data(1, 1) - 5.0) < 1e-6);
    
    std::cout << "readmatrix test passed!" << std::endl;
}

void testMATFile() {
    std::cout << "Testing MAT file save/load..." << std::endl;
    
    Matrix A = Matrix::ones(3, 3) * 2.0;
    Matrix B = Matrix::eye(4);
    double value = 42.0;
    std::string str = "test string";
    
    std::unordered_map<std::string, Value> variables;
    variables["A"] = A;
    variables["B"] = B;
    variables["value"] = value;
    variables["str"] = str;
    
    FileIOManager::saveMAT("test.mat", variables);
    
    auto loaded = FileIOManager::loadMAT("test.mat");
    
    assert(loaded.size() == 4);
    assert(loaded.count("A") == 1);
    assert(loaded.count("B") == 1);
    assert(loaded.count("value") == 1);
    assert(loaded.count("str") == 1);
    
    Matrix loadedA = std::get<Matrix>(loaded["A"]);
    assert(loadedA.rows() == 3);
    assert(loadedA.cols() == 3);
    
    Matrix loadedB = std::get<Matrix>(loaded["B"]);
    assert(loadedB.rows() == 4);
    assert(loadedB.cols() == 4);
    
    double loadedValue = std::get<double>(loaded["value"]);
    assert(std::abs(loadedValue - 42.0) < 1e-6);
    
    std::string loadedStr = std::get<std::string>(loaded["str"]);
    assert(loadedStr == "test string");
    
    std::cout << "MAT file test passed!" << std::endl;
}

void testDirectoryOperations() {
    std::cout << "Testing directory operations..." << std::endl;
    
    std::string currentDir = FileIOManager::pwd();
    assert(!currentDir.empty());
    
    bool created = FileIOManager::mkdir("test_dir");
    assert(created || FileIOManager::exists("test_dir"));
    
    auto files = FileIOManager::dir(".");
    assert(!files.empty());
    
    bool removed = FileIOManager::rmdir("test_dir");
    
    std::cout << "Directory operations test passed!" << std::endl;
}

void testPathOperations() {
    std::cout << "Testing path operations..." << std::endl;
    
    std::vector<std::string> parts = {"data", "results", "output.csv"};
    std::string fullPath = FileIOManager::fullfile(parts);
    assert(!fullPath.empty());
    
    PathInfo info = FileIOManager::fileparts("data/results/output.csv");
    assert(!info.filename.empty());
    
    std::string sep = FileIOManager::filesep();
    assert(!sep.empty());
    
    std::cout << "Path operations test passed!" << std::endl;
}

void testFileOperations() {
    std::cout << "Testing file operations..." << std::endl;
    
    std::ofstream file("test_file.txt");
    file << "test content";
    file.close();
    
    assert(FileIOManager::exists("test_file.txt"));
    assert(FileIOManager::isfile("test_file.txt"));
    assert(!FileIOManager::isdir("test_file.txt"));
    
    bool copied = FileIOManager::copyfile("test_file.txt", "test_file_copy.txt");
    assert(copied);
    assert(FileIOManager::exists("test_file_copy.txt"));
    
    bool moved = FileIOManager::movefile("test_file_copy.txt", "test_file_moved.txt");
    assert(moved);
    assert(FileIOManager::exists("test_file_moved.txt"));
    assert(!FileIOManager::exists("test_file_copy.txt"));
    
    FileIOManager::deletefile("test_file.txt");
    FileIOManager::deletefile("test_file_moved.txt");
    
    assert(!FileIOManager::exists("test_file.txt"));
    assert(!FileIOManager::exists("test_file_moved.txt"));
    
    std::cout << "File operations test passed!" << std::endl;
}

int main() {
    std::cout << "=== CNLab File I/O Tests ===" << std::endl;
    
    try {
        testCSVReadWrite();
        testDLMReadWrite();
        testReadMatrix();
        testMATFile();
        testDirectoryOperations();
        testPathOperations();
        testFileOperations();
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
