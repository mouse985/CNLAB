#pragma once

#include "Matrix.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <stdexcept>
#include <regex>

namespace cnlab {

// Forward declarations
using Complex = std::complex<double>;
class StructArray;
class Cell;
class NDArray;
class Regex;
class StringArray;
class SparseMatrix;
class LogicalArray;
class CharArray;
class FuncHandle;
class Map;
class FileHandle;
class MultiValue;
class Image;

// Value variant - defined here to avoid circular dependency
// We use std::shared_ptr to store Cell and StructArray to avoid incomplete type issues
class StructArray;
class Cell;
class NDArray;
class Regex;
class StringArray;
class SparseMatrix;
class LogicalArray;
class CharArray;
class FuncHandle;
class Map;
class FileHandle;
class MultiValue;
class Image;

class Table;
class Categorical;
class DateTime;
class Duration;

using Value = std::variant<std::monostate, double, Complex, Matrix, std::string, bool,
                           std::shared_ptr<StructArray>, std::shared_ptr<Cell>, std::shared_ptr<Table>,
                           std::shared_ptr<Categorical>, std::shared_ptr<DateTime>, std::shared_ptr<Duration>,
                           std::shared_ptr<NDArray>, std::shared_ptr<Regex>, std::shared_ptr<StringArray>,
                           std::shared_ptr<SparseMatrix>, std::shared_ptr<LogicalArray>, std::shared_ptr<CharArray>,
                           std::shared_ptr<FuncHandle>, std::shared_ptr<Map>, std::shared_ptr<FileHandle>,
                           std::shared_ptr<MultiValue>, std::shared_ptr<Image>>;

/**
 * StructArray - MATLAB-style structure array
 * Stores array of structs, each with named fields
 */
class StructArray {
public:
    StructArray() = default;
    explicit StructArray(size_t size) : elements_(size) {}
    
    // Get struct element at index (1-based like MATLAB)
    std::shared_ptr<StructArray> getElement(size_t index) const {
        if (index < 1 || index > elements_.size()) {
            throw std::runtime_error("Struct array index out of bounds: " + std::to_string(index));
        }
        auto elem = std::make_shared<StructArray>();
        elem->fields_ = elements_[index - 1];
        return elem;
    }
    
    // Set struct element at index (1-based like MATLAB)
    void setElement(size_t index, const std::shared_ptr<StructArray>& value) {
        if (index < 1) {
            throw std::runtime_error("Struct array index must be positive: " + std::to_string(index));
        }
        if (index > elements_.size()) {
            elements_.resize(index);
        }
        elements_[index - 1] = value->fields_;
    }
    
    // Field access on single struct
    bool hasField(const std::string& name) const {
        return fields_.find(name) != fields_.end();
    }
    
    Value getField(const std::string& name) const {
        auto it = fields_.find(name);
        if (it == fields_.end()) {
            throw std::runtime_error("Struct field not found: " + name);
        }
        return it->second;
    }
    
    void setField(const std::string& name, const Value& value) {
        fields_[name] = value;
    }
    
    // Get all field names
    std::vector<std::string> getFieldNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : fields_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Number of fields in single struct
    size_t numFields() const {
        return fields_.size();
    }
    
    // Array size
    size_t size() const {
        return elements_.size();
    }
    
    bool isEmpty() const {
        return elements_.empty() && fields_.empty();
    }
    
    // String representation
    std::string toString() const;
    
private:
    std::unordered_map<std::string, Value> fields_;  // For single struct
    std::vector<std::unordered_map<std::string, Value>> elements_;  // For struct array
};

/**
 * Cell - MATLAB-style cell array
 * Array that can hold elements of any type
 */
class Cell {
public:
    Cell() = default;
    explicit Cell(size_t size) : rows_(1), cols_(size) {
        elements_.resize(size);
    }
    Cell(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        elements_.resize(rows * cols);
    }
    
    // Element access (1-based indexing like MATLAB)
    Value getElement(size_t index) const {
        if (index < 1 || index > elements_.size()) {
            throw std::runtime_error("Cell index out of bounds: " + std::to_string(index));
        }
        return elements_[index - 1];
    }
    
    // Multidimensional element access (1-based indexing like MATLAB)
    Value getElement(size_t row, size_t col) const {
        if (row < 1 || row > rows_ || col < 1 || col > cols_) {
            throw std::runtime_error("Cell index out of bounds: (" + std::to_string(row) + ", " + std::to_string(col) + ")");
        }
        return elements_[(row - 1) * cols_ + (col - 1)];
    }
    
    void setElement(size_t index, const Value& value) {
        if (index < 1) {
            throw std::runtime_error("Cell index must be positive: " + std::to_string(index));
        }
        if (index > elements_.size()) {
            elements_.resize(index);
            if (rows_ == 1) {
                cols_ = elements_.size();
            }
        }
        elements_[index - 1] = value;
    }
    
    // Multidimensional element set (1-based indexing like MATLAB)
    void setElement(size_t row, size_t col, const Value& value) {
        if (row < 1 || col < 1) {
            throw std::runtime_error("Cell index must be positive");
        }
        if (row > rows_ || col > cols_) {
            // Auto-expand the cell array
            size_t newRows = std::max(row, rows_);
            size_t newCols = std::max(col, cols_);
            resize(newRows, newCols);
        }
        elements_[(row - 1) * cols_ + (col - 1)] = value;
    }
    
    // Resize the cell array
    void resize(size_t rows, size_t cols) {
        std::vector<Value> newElements(rows * cols);
        for (size_t r = 1; r <= std::min(rows, rows_); ++r) {
            for (size_t c = 1; c <= std::min(cols, cols_); ++c) {
                newElements[(r - 1) * cols + (c - 1)] = getElement(r, c);
            }
        }
        elements_ = std::move(newElements);
        rows_ = rows;
        cols_ = cols;
    }
    
    // Append element
    void append(const Value& value) {
        elements_.push_back(value);
        if (rows_ == 1) {
            cols_ = elements_.size();
        }
    }
    
    // Size
    size_t size() const {
        return elements_.size();
    }
    
    size_t rows() const {
        return rows_;
    }
    
    size_t cols() const {
        return cols_;
    }
    
    bool isEmpty() const {
        return elements_.empty();
    }
    
    // String representation
    std::string toString() const;
    
private:
    std::vector<Value> elements_;
    size_t rows_ = 1;
    size_t cols_ = 0;
};

/**
 * Table - MATLAB-style table
 * Stores column-oriented data with variable names
 */
class Table {
public:
    Table() = default;
    
    // Add a column with data
    void addColumn(const std::string& name, const Value& data) {
        columnNames_.push_back(name);
        columns_[name] = data;
    }
    
    // Get column by name
    Value getColumn(const std::string& name) const {
        auto it = columns_.find(name);
        if (it == columns_.end()) {
            throw std::runtime_error("Table column not found: " + name);
        }
        return it->second;
    }
    
    // Check if column exists
    bool hasColumn(const std::string& name) const {
        return columns_.find(name) != columns_.end();
    }
    
    // Get all column names
    std::vector<std::string> getColumnNames() const {
        return columnNames_;
    }
    
    // Get number of columns
    size_t numColumns() const {
        return columns_.size();
    }
    
    // Get number of rows (based on first column)
    size_t numRows() const {
        if (columns_.empty()) return 0;
        auto it = columns_.begin();
        if (std::holds_alternative<Matrix>(it->second)) {
            return std::get<Matrix>(it->second).rows();
        }
        return 0;
    }
    
    // Add a row to the table
    void addRow(const std::vector<Value>& rowData);
    
    // Get a row as a cell array (1-based index)
    std::shared_ptr<Cell> getRow(size_t rowIndex) const;
    
    // Remove a row (1-based index)
    void removeRow(size_t rowIndex);
    
    // Get a specific cell value (row and column are 1-based)
    Value getValue(size_t row, const std::string& colName) const;
    
    // Set a specific cell value (row and column are 1-based)
    void setValue(size_t row, const std::string& colName, const Value& value);
    
    // String representation
    std::string toString() const;
    
private:
    std::vector<std::string> columnNames_;  // Preserve insertion order
    std::unordered_map<std::string, Value> columns_;
};

/**
 * Categorical - MATLAB-style categorical array
 * Stores finite set of discrete values efficiently
 */
class Categorical {
public:
    Categorical() = default;
    
    // Create from string cell array
    explicit Categorical(const std::vector<std::string>& values) {
        for (const auto& val : values) {
            addValue(val);
        }
    }
    
    // Add a value
    void addValue(const std::string& value) {
        auto it = categoryMap_.find(value);
        if (it == categoryMap_.end()) {
            // New category
            size_t idx = categories_.size();
            categories_.push_back(value);
            categoryMap_[value] = idx;
            data_.push_back(idx);
        } else {
            // Existing category
            data_.push_back(it->second);
        }
    }
    
    // Get category name by index
    std::string getCategory(size_t index) const {
        if (index >= categories_.size()) {
            throw std::runtime_error("Category index out of bounds");
        }
        return categories_[index];
    }
    
    // Get all unique categories
    std::vector<std::string> getCategories() const {
        return categories_;
    }
    
    // Get number of categories
    size_t numCategories() const {
        return categories_.size();
    }
    
    // Get data size
    size_t size() const {
        return data_.size();
    }
    
    // Get value at index (1-based like MATLAB)
    std::string getValue(size_t index) const {
        if (index < 1 || index > data_.size()) {
            throw std::runtime_error("Index out of bounds");
        }
        return categories_[data_[index - 1]];
    }
    
    // String representation
    std::string toString() const;
    
private:
    std::vector<std::string> categories_;  // Unique category names
    std::unordered_map<std::string, size_t> categoryMap_;  // Name to index
    std::vector<size_t> data_;  // Data stored as category indices
};

/**
 * DateTime - MATLAB-style datetime
 * Stores date and time information
 */
class DateTime {
public:
    DateTime() = default;
    
    // Create from string (e.g., "2024-01-15")
    explicit DateTime(const std::string& dateStr);
    
    // Create from Unix timestamp (seconds since 1970-01-01)
    explicit DateTime(double unixTimestamp);
    
    // Get year, month, day
    int getYear() const { return year_; }
    int getMonth() const { return month_; }
    int getDay() const { return day_; }
    int getHour() const { return hour_; }
    int getMinute() const { return minute_; }
    int getSecond() const { return second_; }
    
    // Convert to Unix timestamp (seconds since 1970-01-01)
    double toUnixTimestamp() const;
    
    // Arithmetic with Duration
    DateTime operator+(const Duration& duration) const;
    DateTime operator-(const Duration& duration) const;
    
    // Difference between two DateTimes returns Duration
    Duration operator-(const DateTime& other) const;
    
    // String representation
    std::string toString() const;
    
private:
    int year_ = 0;
    int month_ = 0;
    int day_ = 0;
    int hour_ = 0;
    int minute_ = 0;
    int second_ = 0;
};

/**
 * Duration - MATLAB-style duration
 * Stores time duration
 */
class Duration {
public:
    Duration() = default;
    
    // Create from total seconds
    explicit Duration(double totalSeconds) {
        totalSeconds_ = totalSeconds;
    }
    
    // Factory methods
    static Duration fromYears(double years);
    static Duration fromMonths(double months);
    static Duration fromDays(double days);
    static Duration fromHours(double hours);
    static Duration fromMinutes(double minutes);
    static Duration fromSeconds(double seconds);
    
    // Get total seconds
    double totalSeconds() const { return totalSeconds_; }
    
    // Get components
    double days() const { return totalSeconds_ / 86400.0; }
    double hours() const { return totalSeconds_ / 3600.0; }
    double minutes() const { return totalSeconds_ / 60.0; }
    double seconds() const { return totalSeconds_; }
    
    // Arithmetic
    Duration operator+(const Duration& other) const {
        return Duration(totalSeconds_ + other.totalSeconds_);
    }
    
    Duration operator-(const Duration& other) const {
        return Duration(totalSeconds_ - other.totalSeconds_);
    }
    
    // String representation
    std::string toString() const;
    
private:
    double totalSeconds_ = 0.0;
};

/**
 * NDArray - N-dimensional array support
 * Supports 3D and higher dimensional arrays
 */
class NDArray {
public:
    NDArray() = default;
    
    // Create with specified dimensions
    explicit NDArray(const std::vector<size_t>& dims);
    
    // Create from Matrix (2D)
    explicit NDArray(const Matrix& mat);
    
    // Get number of dimensions
    size_t ndims() const { return dims_.size(); }
    
    // Get dimensions
    std::vector<size_t> dims() const { return dims_; }
    
    // Get size of specific dimension (1-based like MATLAB)
    size_t size(size_t dim) const {
        if (dim < 1 || dim > dims_.size()) return 1;
        return dims_[dim - 1];
    }
    
    // Get total number of elements
    size_t numel() const { return data_.size(); }
    
    // Check if empty
    bool isempty() const { return data_.empty(); }
    
    // Element access (linear index, 0-based)
    double& operator()(size_t idx) { return data_[idx]; }
    const double& operator()(size_t idx) const { return data_[idx]; }
    
    // Element access (multi-dimensional indices, 1-based like MATLAB)
    double& at(const std::vector<size_t>& indices);
    const double& at(const std::vector<size_t>& indices) const;
    
    // Linear indexing (1-based like MATLAB)
    double& linearAt(size_t idx);
    const double& linearAt(size_t idx) const;
    
    // Convert linear index to subscripts
    std::vector<size_t> ind2sub(size_t linearIdx) const;
    
    // Convert subscripts to linear index
    size_t sub2ind(const std::vector<size_t>& sub) const;
    
    // Reshape to new dimensions
    NDArray reshape(const std::vector<size_t>& newDims) const;
    
    // Squeeze singleton dimensions
    NDArray squeeze() const;
    
    // Permute dimensions
    NDArray permute(const std::vector<size_t>& order) const;
    
    // Get slice (for indexing)
    NDArray slice(size_t dim, size_t idx) const;
    
    // Convert to Matrix (if 2D)
    Matrix toMatrix() const;
    
    // Check if can be converted to Matrix
    bool isMatrix() const { return dims_.size() == 2; }
    
    // Static factory methods
    static NDArray zeros(const std::vector<size_t>& dims);
    static NDArray ones(const std::vector<size_t>& dims);
    static NDArray rand(const std::vector<size_t>& dims);
    
    // String representation
    std::string toString() const;
    
private:
    std::vector<size_t> dims_;  // Dimensions
    std::vector<double> data_;  // Flat data storage
    
    void computeStrides();
    std::vector<size_t> strides_;
};

/**
 * Regex - MATLAB-style regular expression
 * Wrapper around C++ std::regex
 */
class Regex {
public:
    Regex() = default;
    
    // Create from pattern string
    explicit Regex(const std::string& pattern);
    
    // Create with case-insensitive flag
    Regex(const std::string& pattern, bool caseInsensitive);
    
    // Get pattern
    std::string pattern() const { return pattern_; }
    
    // Test if string matches pattern
    bool test(const std::string& str) const;
    
    // Find matches in string
    std::vector<std::string> match(const std::string& str) const;
    
    // Find all matches with positions
    struct MatchResult {
        std::string match;
        size_t start;
        size_t end;
        std::vector<std::string> groups;  // Capture groups
    };
    std::vector<MatchResult> matchAll(const std::string& str) const;
    
    // Find all matches with capture groups
    std::vector<MatchResult> matchAllWithGroups(const std::string& str) const;
    
    // Replace matches in string
    std::string replace(const std::string& str, const std::string& replacement) const;
    
    // Split string by pattern
    std::vector<std::string> split(const std::string& str) const;
    
    // Escape special regex characters
    static std::string escape(const std::string& str);
    
    // String representation
    std::string toString() const;
    
private:
    std::string pattern_;
    std::shared_ptr<std::regex> regex_;
    bool valid_ = false;
    std::string error_;
    
    void compile(bool caseInsensitive = false);
};

/**
 * StringArray - MATLAB-style string array
 * Array of strings with efficient storage
 */
class StringArray {
public:
    StringArray() = default;
    
    // Create from vector of strings
    explicit StringArray(const std::vector<std::string>& strings);
    
    // Create with size and default value
    StringArray(size_t size, const std::string& defaultValue = "");
    
    // Get size
    size_t size() const { return strings_.size(); }
    
    // Check if empty
    bool isempty() const { return strings_.empty(); }
    
    // Element access (1-based like MATLAB)
    std::string get(size_t index) const;
    
    // Element set (1-based like MATLAB)
    void set(size_t index, const std::string& value);
    
    // Append string
    void append(const std::string& value);
    
    // Get all strings
    std::vector<std::string> strings() const { return strings_; }
    
    // Join all strings with delimiter
    std::string join(const std::string& delimiter = "") const;
    
    // String representation
    std::string toString() const;
    
    // Static factory methods
    static StringArray fromStrings(const std::vector<std::string>& strings);
    
private:
    std::vector<std::string> strings_;
};

/**
 * SparseMatrix - MATLAB-style sparse matrix using CSR format
 * Efficient storage for matrices with mostly zero elements
 */
class SparseMatrix {
public:
    SparseMatrix() = default;
    SparseMatrix(size_t rows, size_t cols);
    
    // Get dimensions
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    std::pair<size_t, size_t> size() const { return {rows_, cols_}; }
    
    // Get number of non-zero elements
    size_t nnz() const { return values_.size(); }
    
    // Check if empty
    bool isempty() const { return rows_ == 0 || cols_ == 0; }
    
    // Element access (1-based like MATLAB)
    double get(size_t row, size_t col) const;
    
    // Element set (1-based like MATLAB)
    void set(size_t row, size_t col, double value);
    
    // Convert to dense matrix
    Matrix toDense() const;
    
    // Create from dense matrix
    static std::shared_ptr<SparseMatrix> fromDense(const Matrix& mat);
    
    // Create from coordinate format (1-based indices)
    static std::shared_ptr<SparseMatrix> fromCOO(size_t rows, size_t cols,
                                                  const std::vector<size_t>& row_indices,
                                                  const std::vector<size_t>& col_indices,
                                                  const std::vector<double>& values);
    
    // Transpose
    std::shared_ptr<SparseMatrix> transpose() const;
    
    // Scalar multiplication
    std::shared_ptr<SparseMatrix> multiply(double scalar) const;
    
    // Matrix addition (sparse + sparse)
    std::shared_ptr<SparseMatrix> add(const SparseMatrix& other) const;
    
    // Matrix subtraction (sparse - sparse)
    std::shared_ptr<SparseMatrix> subtract(const SparseMatrix& other) const;
    
    // Matrix multiplication (sparse * dense)
    Matrix multiply(const Matrix& other) const;
    
    // String representation
    std::string toString() const;
    
    // Get internal data (for advanced operations)
    const std::vector<double>& values() const { return values_; }
    const std::vector<size_t>& colIndices() const { return col_indices_; }
    const std::vector<size_t>& rowPointers() const { return row_pointers_; }
    
private:
    size_t rows_ = 0;
    size_t cols_ = 0;
    std::vector<double> values_;        // Non-zero values
    std::vector<size_t> col_indices_;   // Column indices for each value
    std::vector<size_t> row_pointers_;  // Row pointers (size = rows + 1)
    
    // Rebuild CSR structure after modifications
    void rebuildCSR();
};

/**
 * LogicalArray - MATLAB-style logical array
 * Array of boolean values with efficient storage
 */
class LogicalArray {
public:
    LogicalArray() = default;
    explicit LogicalArray(size_t size);
    LogicalArray(size_t rows, size_t cols);
    
    // Get dimensions
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t size() const { return values_.size(); }
    std::pair<size_t, size_t> dims() const { return {rows_, cols_}; }
    
    // Check if empty
    bool isempty() const { return values_.empty(); }
    
    // Element access (1-based like MATLAB)
    bool get(size_t index) const;
    bool get(size_t row, size_t col) const;
    
    // Element set (1-based like MATLAB)
    void set(size_t index, bool value);
    void set(size_t row, size_t col, bool value);
    
    // Get all values
    const std::vector<bool>& values() const { return values_; }
    
    // Convert to dense matrix (1.0 for true, 0.0 for false)
    Matrix toDense() const;
    
    // Create from dense matrix (non-zero -> true, zero -> false)
    static std::shared_ptr<LogicalArray> fromDense(const Matrix& mat);
    
    // Create from vector
    static std::shared_ptr<LogicalArray> fromVector(const std::vector<bool>& values);
    static std::shared_ptr<LogicalArray> fromVector(const std::vector<double>& values);
    
    // Logical operations
    std::shared_ptr<LogicalArray> logicalNot() const;
    std::shared_ptr<LogicalArray> logicalAnd(const LogicalArray& other) const;
    std::shared_ptr<LogicalArray> logicalOr(const LogicalArray& other) const;
    std::shared_ptr<LogicalArray> logicalXor(const LogicalArray& other) const;
    
    // Find true indices (1-based)
    std::vector<double> find() const;
    
    // Count true values
    size_t nnz() const;
    
    // String representation
    std::string toString() const;
    
private:
    size_t rows_ = 0;
    size_t cols_ = 0;
    std::vector<bool> values_;
};

/**
 * CharArray - MATLAB-style character array
 * Stores characters as a matrix (unlike StringArray which stores strings as elements)
 * Similar to MATLAB's 'char' type
 */
class CharArray {
public:
    CharArray() = default;
    explicit CharArray(const std::string& str);
    CharArray(size_t rows, size_t cols, char fillChar = ' ');
    
    // Get dimensions
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t size() const { return chars_.size(); }
    std::pair<size_t, size_t> dims() const { return {rows_, cols_}; }
    
    // Check if empty
    bool isempty() const { return chars_.empty(); }
    
    // Element access (1-based like MATLAB)
    char get(size_t index) const;
    char get(size_t row, size_t col) const;
    
    // Element set (1-based like MATLAB)
    void set(size_t index, char value);
    void set(size_t row, size_t col, char value);
    
    // Get as string (row concatenated)
    std::string toString() const;
    
    // Get specific row as string
    std::string getRow(size_t row) const;
    
    // Set row from string
    void setRow(size_t row, const std::string& str);
    
    // Create from string (1 row)
    static std::shared_ptr<CharArray> fromString(const std::string& str);
    
    // Create from vector of strings (each string is a row)
    static std::shared_ptr<CharArray> fromStrings(const std::vector<std::string>& strings);
    
    // Convert to StringArray (each row becomes a string element)
    std::shared_ptr<StringArray> toStringArray() const;
    
    // Transpose
    std::shared_ptr<CharArray> transpose() const;
    
    // Vertical concatenation
    std::shared_ptr<CharArray> vertcat(const CharArray& other) const;
    
    // Horizontal concatenation
    std::shared_ptr<CharArray> horzcat(const CharArray& other) const;
    
    // Comparison
    std::shared_ptr<LogicalArray> equals(const CharArray& other) const;
    
    // Find character positions
    std::vector<double> find(char c) const;
    
    // String representation
    std::string toDisplayString() const;
    
private:
    size_t rows_ = 0;
    size_t cols_ = 0;
    std::vector<char> chars_;
};

/**
 * FuncHandle - MATLAB-style function handle
 * Represents a reference to a function that can be called later
 * Syntax: @function_name or @(args) expression
 */
class FuncHandle {
public:
    // Type of function handle
    enum class Type {
        NAMED_FUNCTION,    // @sin, @cos, etc.
        ANONYMOUS_FUNCTION // @(x) x^2
    };
    
    FuncHandle() = default;
    explicit FuncHandle(const std::string& name);
    FuncHandle(const std::vector<std::string>& params, const std::string& expression);
    
    // Get type
    Type type() const { return type_; }
    
    // Get function name (for named functions)
    const std::string& name() const { return name_; }
    
    // Get parameters (for anonymous functions)
    const std::vector<std::string>& parameters() const { return parameters_; }
    
    // Get expression (for anonymous functions)
    const std::string& expression() const { return expression_; }
    
    // Check if empty
    bool isempty() const { return name_.empty() && expression_.empty(); }
    
    // String representation
    std::string toString() const;
    
    // Comparison
    bool equals(const FuncHandle& other) const;
    
private:
    Type type_ = Type::NAMED_FUNCTION;
    std::string name_;                    // For named functions
    std::vector<std::string> parameters_; // For anonymous functions
    std::string expression_;              // For anonymous functions
};

/**
 * FileHandle - MATLAB-style file handle
 * Manages file I/O operations
 */
class FileHandle {
public:
    FileHandle() = default;
    FileHandle(const std::string& filename, const std::string& mode);
    ~FileHandle();
    
    // Disable copy
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    
    // Enable move
    FileHandle(FileHandle&& other) noexcept;
    FileHandle& operator=(FileHandle&& other) noexcept;
    
    // Check if file is open
    bool isOpen() const { return file_ != nullptr; }
    
    // Get file pointer
    FILE* handle() const { return file_; }
    
    // Get filename
    std::string filename() const { return filename_; }
    
    // Get mode
    std::string mode() const { return mode_; }
    
    // Close file
    void close();
    
    // String representation
    std::string toString() const;
    
private:
    FILE* file_ = nullptr;
    std::string filename_;
    std::string mode_;
};

/**
 * Map - MATLAB-style Map container
 * Key-value store with string keys
 */
class Map {
public:
    Map() = default;
    
    // Check if empty
    bool isempty() const { return data_.empty(); }
    
    // Get size
    size_t size() const { return data_.size(); }
    
    // Check if key exists
    bool isKey(const std::string& key) const;
    
    // Get value (returns monostate if not found)
    Value get(const std::string& key) const;
    
    // Set value
    void set(const std::string& key, const Value& value);
    
    // Remove key
    void remove(const std::string& key);
    
    // Get all keys
    std::vector<std::string> keys() const;
    
    // Get all values
    std::vector<Value> values() const;
    
    // String representation
    std::string toString() const;
    
private:
    std::unordered_map<std::string, Value> data_;
};

/**
 * MultiValue - Container for multiple return values
 * Used for functions that return multiple values like size()
 */
class MultiValue {
public:
    MultiValue() = default;
    explicit MultiValue(const std::vector<Value>& values) : values_(values) {}
    explicit MultiValue(std::vector<Value>&& values) : values_(std::move(values)) {}
    
    // Get number of values
    size_t size() const { return values_.size(); }
    
    // Check if empty
    bool empty() const { return values_.empty(); }
    
    // Get value at index (0-based)
    const Value& get(size_t index) const {
        if (index >= values_.size()) {
            throw std::runtime_error("MultiValue index out of bounds: " + std::to_string(index));
        }
        return values_[index];
    }
    
    // Get all values
    const std::vector<Value>& values() const { return values_; }
    
    // Add a value
    void push_back(const Value& value) { values_.push_back(value); }
    void push_back(Value&& value) { values_.push_back(std::move(value)); }
    
    // String representation
    std::string toString() const;
    
private:
    std::vector<Value> values_;
};

} // namespace cnlab
