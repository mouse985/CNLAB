#include "DataTypes.hpp"
#include <sstream>
#include <regex>

namespace cnlab {

std::string StructArray::toString() const {
    std::ostringstream oss;
    if (!elements_.empty()) {
        oss << "struct array with " << elements_.size() << " elements";
    } else {
        oss << "struct with " << fields_.size() << " fields:";
        for (const auto& [name, value] : fields_) {
            oss << "\n    " << name;
        }
    }
    return oss.str();
}

std::string Cell::toString() const {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < elements_.size(); ++i) {
        if (i > 0) oss << ", ";
        // For now, just show type info
        if (std::holds_alternative<double>(elements_[i])) {
            oss << std::get<double>(elements_[i]);
        } else if (std::holds_alternative<std::string>(elements_[i])) {
            oss << "'" << std::get<std::string>(elements_[i]) << "'";
        } else {
            oss << "[...]";
        }
    }
    oss << "}";
    return oss.str();
}

void Table::addRow(const std::vector<Value>& rowData) {
    if (rowData.size() != columnNames_.size()) {
        throw std::runtime_error("Row data size does not match number of columns");
    }
    
    for (size_t i = 0; i < columnNames_.size(); ++i) {
        const std::string& colName = columnNames_[i];
        Value colValue = columns_[colName];
        
        if (std::holds_alternative<Matrix>(colValue)) {
            Matrix& mat = std::get<Matrix>(colValue);
            // Append new value to the column
            size_t newRows = mat.rows() + 1;
            Matrix newMat(newRows, 1);
            
            // Copy existing data
            for (size_t r = 0; r < mat.rows(); ++r) {
                newMat(r, 0) = mat(r, 0);
            }
            
            // Add new value
            if (std::holds_alternative<double>(rowData[i])) {
                newMat(mat.rows(), 0) = std::get<double>(rowData[i]);
            } else {
                newMat(mat.rows(), 0) = 0.0; // Default for non-numeric
            }
            
            columns_[colName] = newMat;
        }
    }
}

std::shared_ptr<Cell> Table::getRow(size_t rowIndex) const {
    if (rowIndex < 1 || rowIndex > numRows()) {
        throw std::runtime_error("Row index out of bounds: " + std::to_string(rowIndex));
    }
    
    auto rowCell = std::make_shared<Cell>(columnNames_.size());
    size_t idx = 1;
    
    for (const auto& colName : columnNames_) {
        Value colValue = columns_.at(colName);
        
        if (std::holds_alternative<Matrix>(colValue)) {
            const Matrix& mat = std::get<Matrix>(colValue);
            rowCell->setElement(idx, mat(rowIndex - 1, 0));
        } else {
            rowCell->setElement(idx, 0.0);
        }
        ++idx;
    }
    
    return rowCell;
}

void Table::removeRow(size_t rowIndex) {
    if (rowIndex < 1 || rowIndex > numRows()) {
        throw std::runtime_error("Row index out of bounds: " + std::to_string(rowIndex));
    }
    
    for (const auto& colName : columnNames_) {
        Value colValue = columns_[colName];
        
        if (std::holds_alternative<Matrix>(colValue)) {
            Matrix& mat = std::get<Matrix>(colValue);
            size_t newRows = mat.rows() - 1;
            Matrix newMat(newRows, 1);
            
            size_t newR = 0;
            for (size_t r = 0; r < mat.rows(); ++r) {
                if (r != rowIndex - 1) {
                    newMat(newR, 0) = mat(r, 0);
                    ++newR;
                }
            }
            
            columns_[colName] = newMat;
        }
    }
}

Value Table::getValue(size_t row, const std::string& colName) const {
    if (row < 1 || row > numRows()) {
        throw std::runtime_error("Row index out of bounds: " + std::to_string(row));
    }
    
    auto it = columns_.find(colName);
    if (it == columns_.end()) {
        throw std::runtime_error("Column not found: " + colName);
    }
    
    Value colValue = it->second;
    if (std::holds_alternative<Matrix>(colValue)) {
        const Matrix& mat = std::get<Matrix>(colValue);
        return mat(row - 1, 0);
    }
    
    return 0.0;
}

void Table::setValue(size_t row, const std::string& colName, const Value& value) {
    if (row < 1 || row > numRows()) {
        throw std::runtime_error("Row index out of bounds: " + std::to_string(row));
    }
    
    auto it = columns_.find(colName);
    if (it == columns_.end()) {
        throw std::runtime_error("Column not found: " + colName);
    }
    
    Value colValue = it->second;
    if (std::holds_alternative<Matrix>(colValue)) {
        Matrix mat = std::get<Matrix>(colValue);
        if (std::holds_alternative<double>(value)) {
            mat(row - 1, 0) = std::get<double>(value);
        }
        columns_[colName] = mat;
    }
}

std::string Table::toString() const {
    std::ostringstream oss;
    oss << "table with " << numColumns() << " columns and " << numRows() << " rows:";
    for (const auto& name : columnNames_) {
        oss << "\n    " << name;
    }
    return oss.str();
}

std::string Categorical::toString() const {
    std::ostringstream oss;
    oss << "categorical with " << numCategories() << " categories:";
    for (const auto& cat : categories_) {
        oss << "\n    " << cat;
    }
    return oss.str();
}

// Helper function to convert date to days since 1970-01-01
static int daysSinceEpoch(int year, int month, int day) {
    // Simplified calculation - assumes Gregorian calendar
    int days = 0;
    
    // Days from years
    for (int y = 1970; y < year; ++y) {
        days += ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 366 : 365;
    }
    
    // Days from months
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    for (int m = 1; m < month; ++m) {
        days += daysInMonth[m - 1];
    }
    
    // Days from day of month
    days += day - 1;
    
    return days;
}

// Helper function to convert days since 1970-01-01 to date
static void daysToDate(int days, int& year, int& month, int& day) {
    year = 1970;
    month = 1;
    day = 1;
    
    // Add years
    while (days >= 365) {
        int daysInYear = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
        if (days >= daysInYear) {
            days -= daysInYear;
            year++;
        } else {
            break;
        }
    }
    
    // Add months
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    while (days >= daysInMonth[month - 1]) {
        days -= daysInMonth[month - 1];
        month++;
    }
    
    // Add days
    day += days;
}

DateTime::DateTime(const std::string& dateStr) {
    // Simple parser for "YYYY-MM-DD" or "YYYY-MM-DD HH:MM:SS"
    // For simplicity, just parse the date part
    if (dateStr.length() >= 10) {
        year_ = std::stoi(dateStr.substr(0, 4));
        month_ = std::stoi(dateStr.substr(5, 2));
        day_ = std::stoi(dateStr.substr(8, 2));
    }
    // Parse time if present
    if (dateStr.length() >= 19) {
        hour_ = std::stoi(dateStr.substr(11, 2));
        minute_ = std::stoi(dateStr.substr(14, 2));
        second_ = std::stoi(dateStr.substr(17, 2));
    }
}

DateTime::DateTime(double unixTimestamp) {
    int days = static_cast<int>(unixTimestamp / 86400.0);
    int seconds = static_cast<int>(unixTimestamp - days * 86400.0);
    
    daysToDate(days, year_, month_, day_);
    
    hour_ = seconds / 3600;
    seconds %= 3600;
    minute_ = seconds / 60;
    second_ = seconds % 60;
}

double DateTime::toUnixTimestamp() const {
    double days = daysSinceEpoch(year_, month_, day_);
    double seconds = hour_ * 3600 + minute_ * 60 + second_;
    return days * 86400.0 + seconds;
}

DateTime DateTime::operator+(const Duration& duration) const {
    double newTimestamp = toUnixTimestamp() + duration.totalSeconds();
    return DateTime(newTimestamp);
}

DateTime DateTime::operator-(const Duration& duration) const {
    double newTimestamp = toUnixTimestamp() - duration.totalSeconds();
    return DateTime(newTimestamp);
}

Duration DateTime::operator-(const DateTime& other) const {
    double diff = toUnixTimestamp() - other.toUnixTimestamp();
    return Duration(diff);
}

std::string DateTime::toString() const {
    std::ostringstream oss;
    oss << year_ << "-" << month_ << "-" << day_;
    if (hour_ != 0 || minute_ != 0 || second_ != 0) {
        oss << " " << hour_ << ":" << minute_ << ":" << second_;
    }
    return oss.str();
}

Duration Duration::fromYears(double years) {
    return Duration(years * 365.25 * 86400.0);
}

Duration Duration::fromMonths(double months) {
    return Duration(months * 30.44 * 86400.0);
}

Duration Duration::fromDays(double days) {
    return Duration(days * 86400.0);
}

Duration Duration::fromHours(double hours) {
    return Duration(hours * 3600.0);
}

Duration Duration::fromMinutes(double minutes) {
    return Duration(minutes * 60.0);
}

Duration Duration::fromSeconds(double seconds) {
    return Duration(seconds);
}

std::string Duration::toString() const {
    std::ostringstream oss;
    if (totalSeconds_ >= 86400.0) {
        oss << days() << " days";
    } else if (totalSeconds_ >= 3600.0) {
        oss << hours() << " hours";
    } else if (totalSeconds_ >= 60.0) {
        oss << minutes() << " minutes";
    } else {
        oss << totalSeconds_ << " seconds";
    }
    return oss.str();
}

// NDArray implementation
NDArray::NDArray(const std::vector<size_t>& dims) : dims_(dims) {
    size_t totalSize = 1;
    for (size_t d : dims_) {
        totalSize *= d;
    }
    data_.resize(totalSize, 0.0);
    computeStrides();
}

NDArray::NDArray(const Matrix& mat) {
    dims_ = {mat.rows(), mat.cols()};
    data_.resize(mat.size());
    for (size_t i = 0; i < mat.size(); ++i) {
        data_[i] = mat(i);
    }
    computeStrides();
}

void NDArray::computeStrides() {
    strides_.resize(dims_.size());
    size_t stride = 1;
    for (int i = static_cast<int>(dims_.size()) - 1; i >= 0; --i) {
        strides_[i] = stride;
        stride *= dims_[i];
    }
}

double& NDArray::at(const std::vector<size_t>& indices) {
    if (indices.size() != dims_.size()) {
        throw std::runtime_error("Index dimensions mismatch");
    }
    return data_[sub2ind(indices)];
}

const double& NDArray::at(const std::vector<size_t>& indices) const {
    if (indices.size() != dims_.size()) {
        throw std::runtime_error("Index dimensions mismatch");
    }
    return data_[sub2ind(indices)];
}

double& NDArray::linearAt(size_t idx) {
    if (idx < 1 || idx > data_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return data_[idx - 1];
}

const double& NDArray::linearAt(size_t idx) const {
    if (idx < 1 || idx > data_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return data_[idx - 1];
}

std::vector<size_t> NDArray::ind2sub(size_t linearIdx) const {
    // linearIdx is 0-based
    std::vector<size_t> sub(dims_.size());
    for (size_t i = 0; i < dims_.size(); ++i) {
        sub[i] = (linearIdx / strides_[i]) % dims_[i] + 1; // 1-based
    }
    return sub;
}

size_t NDArray::sub2ind(const std::vector<size_t>& sub) const {
    // sub is 1-based
    size_t idx = 0;
    for (size_t i = 0; i < dims_.size(); ++i) {
        if (sub[i] < 1 || sub[i] > dims_[i]) {
            throw std::runtime_error("Index out of bounds");
        }
        idx += (sub[i] - 1) * strides_[i];
    }
    return idx;
}

NDArray NDArray::reshape(const std::vector<size_t>& newDims) const {
    size_t newTotalSize = 1;
    for (size_t d : newDims) {
        newTotalSize *= d;
    }
    if (newTotalSize != data_.size()) {
        throw std::runtime_error("Reshape size mismatch");
    }
    NDArray result;
    result.dims_ = newDims;
    result.data_ = data_;
    result.computeStrides();
    return result;
}

NDArray NDArray::squeeze() const {
    std::vector<size_t> newDims;
    for (size_t d : dims_) {
        if (d != 1) {
            newDims.push_back(d);
        }
    }
    if (newDims.empty()) {
        newDims.push_back(1);
    }
    return reshape(newDims);
}

NDArray NDArray::permute(const std::vector<size_t>& order) const {
    if (order.size() != dims_.size()) {
        throw std::runtime_error("Permute order dimensions mismatch");
    }
    
    std::vector<size_t> newDims(dims_.size());
    for (size_t i = 0; i < order.size(); ++i) {
        newDims[i] = dims_[order[i] - 1]; // order is 1-based
    }
    
    NDArray result(newDims);
    
    // Copy data with permutation
    for (size_t i = 0; i < data_.size(); ++i) {
        auto oldSub = ind2sub(i);
        std::vector<size_t> newSub(oldSub.size());
        for (size_t j = 0; j < order.size(); ++j) {
            newSub[j] = oldSub[order[j] - 1];
        }
        result.at(newSub) = data_[i];
    }
    
    return result;
}

NDArray NDArray::slice(size_t dim, size_t idx) const {
    if (dim < 1 || dim > dims_.size()) {
        throw std::runtime_error("Dimension out of bounds");
    }
    if (idx < 1 || idx > dims_[dim - 1]) {
        throw std::runtime_error("Index out of bounds");
    }
    
    std::vector<size_t> newDims = dims_;
    newDims.erase(newDims.begin() + (dim - 1));
    
    NDArray result(newDims);
    
    // Copy slice data
    size_t destIdx = 0;
    for (size_t i = 0; i < data_.size(); ++i) {
        auto sub = ind2sub(i);
        if (sub[dim - 1] == idx) {
            result.data_[destIdx++] = data_[i];
        }
    }
    
    return result;
}

Matrix NDArray::toMatrix() const {
    if (!isMatrix()) {
        throw std::runtime_error("Cannot convert NDArray to Matrix: not 2D");
    }
    Matrix result(dims_[0], dims_[1]);
    for (size_t i = 0; i < data_.size(); ++i) {
        result(i) = data_[i];
    }
    return result;
}

NDArray NDArray::zeros(const std::vector<size_t>& dims) {
    return NDArray(dims);
}

NDArray NDArray::ones(const std::vector<size_t>& dims) {
    NDArray result(dims);
    std::fill(result.data_.begin(), result.data_.end(), 1.0);
    return result;
}

NDArray NDArray::rand(const std::vector<size_t>& dims) {
    NDArray result(dims);
    for (size_t i = 0; i < result.data_.size(); ++i) {
        result.data_[i] = static_cast<double>(::rand()) / RAND_MAX;
    }
    return result;
}

std::string NDArray::toString() const {
    std::ostringstream oss;
    oss << dims_.size() << "D array [";
    for (size_t i = 0; i < dims_.size(); ++i) {
        if (i > 0) oss << " x ";
        oss << dims_[i];
    }
    oss << "] with " << numel() << " elements";
    return oss.str();
}

// Regex implementation
Regex::Regex(const std::string& pattern) : pattern_(pattern) {
    compile(false);
}

Regex::Regex(const std::string& pattern, bool caseInsensitive) : pattern_(pattern) {
    compile(caseInsensitive);
}

void Regex::compile(bool caseInsensitive) {
    try {
        auto flags = std::regex::ECMAScript;
        if (caseInsensitive) {
            flags |= std::regex::icase;
        }
        regex_ = std::make_shared<std::regex>(pattern_, flags);
        valid_ = true;
        error_.clear();
    } catch (const std::regex_error& e) {
        valid_ = false;
        error_ = e.what();
        regex_.reset();
    }
}

bool Regex::test(const std::string& str) const {
    if (!valid_ || !regex_) return false;
    try {
        return std::regex_search(str, *regex_);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Regex::match(const std::string& str) const {
    std::vector<std::string> results;
    if (!valid_ || !regex_) return results;
    
    try {
        std::smatch match;
        if (std::regex_search(str, match, *regex_)) {
            for (size_t i = 0; i < match.size(); ++i) {
                results.push_back(match[i].str());
            }
        }
    } catch (...) {
        // Return empty results on error
    }
    return results;
}

std::vector<Regex::MatchResult> Regex::matchAll(const std::string& str) const {
    std::vector<MatchResult> results;
    if (!valid_ || !regex_) return results;
    
    try {
        auto begin = std::sregex_iterator(str.begin(), str.end(), *regex_);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            MatchResult result;
            result.match = it->str();
            result.start = it->position();
            result.end = it->position() + it->length();
            results.push_back(result);
        }
    } catch (...) {
        // Return empty results on error
    }
    return results;
}

std::vector<Regex::MatchResult> Regex::matchAllWithGroups(const std::string& str) const {
    std::vector<MatchResult> results;
    if (!valid_ || !regex_) return results;
    
    try {
        auto begin = std::sregex_iterator(str.begin(), str.end(), *regex_);
        auto end = std::sregex_iterator();
        
        for (auto it = begin; it != end; ++it) {
            MatchResult result;
            result.match = it->str();
            result.start = it->position();
            result.end = it->position() + it->length();
            // Extract capture groups
            for (size_t i = 1; i < it->size(); ++i) {
                result.groups.push_back((*it)[i].str());
            }
            results.push_back(result);
        }
    } catch (...) {
        // Return empty results on error
    }
    return results;
}

std::string Regex::replace(const std::string& str, const std::string& replacement) const {
    if (!valid_ || !regex_) return str;
    
    try {
        return std::regex_replace(str, *regex_, replacement);
    } catch (...) {
        return str;
    }
}

std::vector<std::string> Regex::split(const std::string& str) const {
    std::vector<std::string> results;
    if (!valid_ || !regex_) {
        results.push_back(str);
        return results;
    }
    
    try {
        std::sregex_token_iterator it(str.begin(), str.end(), *regex_, -1);
        std::sregex_token_iterator end;
        
        while (it != end) {
            results.push_back(*it);
            ++it;
        }
    } catch (...) {
        results.push_back(str);
    }
    return results;
}

std::string Regex::escape(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2);
    
    for (char c : str) {
        switch (c) {
            case '.': case '^': case '$': case '*':
            case '+': case '?': case '(': case ')':
            case '[': case ']': case '{': case '}':
            case '|': case '\\':
                result += '\\';
                result += c;
                break;
            default:
                result += c;
        }
    }
    return result;
}

std::string Regex::toString() const {
    if (!valid_) {
        return "regex(invalid: " + error_ + ")";
    }
    return "regex(/" + pattern_ + "/)";
}

// StringArray implementation
StringArray::StringArray(const std::vector<std::string>& strings) : strings_(strings) {}

StringArray::StringArray(size_t size, const std::string& defaultValue) : strings_(size, defaultValue) {}

std::string StringArray::get(size_t index) const {
    if (index < 1 || index > strings_.size()) {
        throw std::runtime_error("StringArray index out of bounds: " + std::to_string(index));
    }
    return strings_[index - 1];
}

void StringArray::set(size_t index, const std::string& value) {
    if (index < 1) {
        throw std::runtime_error("StringArray index must be positive: " + std::to_string(index));
    }
    if (index > strings_.size()) {
        strings_.resize(index);
    }
    strings_[index - 1] = value;
}

void StringArray::append(const std::string& value) {
    strings_.push_back(value);
}

std::string StringArray::join(const std::string& delimiter) const {
    std::string result;
    for (size_t i = 0; i < strings_.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += strings_[i];
    }
    return result;
}

std::string StringArray::toString() const {
    std::ostringstream oss;
    oss << "string array [" << strings_.size() << "]:[";
    for (size_t i = 0; i < strings_.size() && i < 5; ++i) {
        if (i > 0) oss << ", ";
        oss << "\"" << strings_[i] << "\"";
    }
    if (strings_.size() > 5) {
        oss << ", ...";
    }
    oss << "]";
    return oss.str();
}

StringArray StringArray::fromStrings(const std::vector<std::string>& strings) {
    return StringArray(strings);
}

// SparseMatrix implementation
SparseMatrix::SparseMatrix(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
    row_pointers_.resize(rows + 1, 0);
}

double SparseMatrix::get(size_t row, size_t col) const {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    // Convert to 0-based
    row--;
    col--;
    
    // Binary search in the row
    size_t start = row_pointers_[row];
    size_t end = row_pointers_[row + 1];
    
    for (size_t i = start; i < end; ++i) {
        if (col_indices_[i] == col) {
            return values_[i];
        }
    }
    return 0.0;
}

void SparseMatrix::set(size_t row, size_t col, double value) {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    // Convert to 0-based
    row--;
    col--;
    
    size_t start = row_pointers_[row];
    size_t end = row_pointers_[row + 1];
    
    // Check if element already exists
    for (size_t i = start; i < end; ++i) {
        if (col_indices_[i] == col) {
            if (value == 0.0) {
                // Remove element
                values_.erase(values_.begin() + i);
                col_indices_.erase(col_indices_.begin() + i);
                for (size_t j = row + 1; j < row_pointers_.size(); ++j) {
                    row_pointers_[j]--;
                }
            } else {
                values_[i] = value;
            }
            return;
        }
    }
    
    // Insert new element
    if (value != 0.0) {
        values_.insert(values_.begin() + end, value);
        col_indices_.insert(col_indices_.begin() + end, col);
        for (size_t j = row + 1; j < row_pointers_.size(); ++j) {
            row_pointers_[j]++;
        }
    }
}

Matrix SparseMatrix::toDense() const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = row_pointers_[i]; j < row_pointers_[i + 1]; ++j) {
            result(i, col_indices_[j]) = values_[j];
        }
    }
    return result;
}

std::shared_ptr<SparseMatrix> SparseMatrix::fromDense(const Matrix& mat) {
    auto sparse = std::make_shared<SparseMatrix>(mat.rows(), mat.cols());
    
    for (size_t i = 0; i < mat.rows(); ++i) {
        for (size_t j = 0; j < mat.cols(); ++j) {
            double val = mat(i, j);
            if (val != 0.0) {
                sparse->values_.push_back(val);
                sparse->col_indices_.push_back(j);
            }
        }
        sparse->row_pointers_[i + 1] = sparse->values_.size();
    }
    
    return sparse;
}

std::shared_ptr<SparseMatrix> SparseMatrix::fromCOO(size_t rows, size_t cols,
                                                     const std::vector<size_t>& row_indices,
                                                     const std::vector<size_t>& col_indices,
                                                     const std::vector<double>& values) {
    auto sparse = std::make_shared<SparseMatrix>(rows, cols);
    
    // Count elements per row
    std::vector<size_t> row_counts(rows, 0);
    for (size_t r : row_indices) {
        if (r >= 1 && r <= rows) {
            row_counts[r - 1]++;
        }
    }
    
    // Build row pointers
    sparse->row_pointers_[0] = 0;
    for (size_t i = 0; i < rows; ++i) {
        sparse->row_pointers_[i + 1] = sparse->row_pointers_[i] + row_counts[i];
    }
    
    // Fill values and column indices
    sparse->values_.resize(values.size());
    sparse->col_indices_.resize(values.size());
    
    std::vector<size_t> current_pos = sparse->row_pointers_;
    for (size_t i = 0; i < values.size(); ++i) {
        size_t row = row_indices[i] - 1; // Convert to 0-based
        size_t pos = current_pos[row]++;
        sparse->values_[pos] = values[i];
        sparse->col_indices_[pos] = col_indices[i] - 1; // Convert to 0-based
    }
    
    return sparse;
}

std::shared_ptr<SparseMatrix> SparseMatrix::transpose() const {
    auto result = std::make_shared<SparseMatrix>(cols_, rows_);
    
    // Count elements per column (which becomes row in transposed)
    std::vector<size_t> col_counts(cols_, 0);
    for (size_t col : col_indices_) {
        col_counts[col]++;
    }
    
    // Build row pointers for transposed matrix
    result->row_pointers_[0] = 0;
    for (size_t i = 0; i < cols_; ++i) {
        result->row_pointers_[i + 1] = result->row_pointers_[i] + col_counts[i];
    }
    
    // Fill values
    result->values_.resize(values_.size());
    result->col_indices_.resize(values_.size());
    
    std::vector<size_t> current_pos = result->row_pointers_;
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = row_pointers_[i]; j < row_pointers_[i + 1]; ++j) {
            size_t col = col_indices_[j];
            size_t pos = current_pos[col]++;
            result->values_[pos] = values_[j];
            result->col_indices_[pos] = i;
        }
    }
    
    return result;
}

std::shared_ptr<SparseMatrix> SparseMatrix::multiply(double scalar) const {
    auto result = std::make_shared<SparseMatrix>(rows_, cols_);
    result->values_ = values_;
    result->col_indices_ = col_indices_;
    result->row_pointers_ = row_pointers_;
    
    for (double& val : result->values_) {
        val *= scalar;
    }
    
    return result;
}

std::shared_ptr<SparseMatrix> SparseMatrix::add(const SparseMatrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Matrix dimensions must agree");
    }
    
    auto result = std::make_shared<SparseMatrix>(rows_, cols_);
    result->row_pointers_.resize(rows_ + 1);
    result->row_pointers_[0] = 0;
    
    for (size_t i = 0; i < rows_; ++i) {
        // Merge rows from both matrices
        size_t j1 = row_pointers_[i];
        size_t j2 = other.row_pointers_[i];
        size_t end1 = row_pointers_[i + 1];
        size_t end2 = other.row_pointers_[i + 1];
        
        while (j1 < end1 || j2 < end2) {
            if (j1 < end1 && (j2 >= end2 || col_indices_[j1] < other.col_indices_[j2])) {
                result->values_.push_back(values_[j1]);
                result->col_indices_.push_back(col_indices_[j1]);
                j1++;
            } else if (j2 < end2 && (j1 >= end1 || other.col_indices_[j2] < col_indices_[j1])) {
                result->values_.push_back(other.values_[j2]);
                result->col_indices_.push_back(other.col_indices_[j2]);
                j2++;
            } else {
                // Same column, add values
                double sum = values_[j1] + other.values_[j2];
                if (sum != 0.0) {
                    result->values_.push_back(sum);
                    result->col_indices_.push_back(col_indices_[j1]);
                }
                j1++;
                j2++;
            }
        }
        result->row_pointers_[i + 1] = result->values_.size();
    }
    
    return result;
}

std::shared_ptr<SparseMatrix> SparseMatrix::subtract(const SparseMatrix& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Matrix dimensions must agree");
    }
    
    auto result = std::make_shared<SparseMatrix>(rows_, cols_);
    result->row_pointers_.resize(rows_ + 1);
    result->row_pointers_[0] = 0;
    
    for (size_t i = 0; i < rows_; ++i) {
        size_t j1 = row_pointers_[i];
        size_t j2 = other.row_pointers_[i];
        size_t end1 = row_pointers_[i + 1];
        size_t end2 = other.row_pointers_[i + 1];
        
        while (j1 < end1 || j2 < end2) {
            if (j1 < end1 && (j2 >= end2 || col_indices_[j1] < other.col_indices_[j2])) {
                result->values_.push_back(values_[j1]);
                result->col_indices_.push_back(col_indices_[j1]);
                j1++;
            } else if (j2 < end2 && (j1 >= end1 || other.col_indices_[j2] < col_indices_[j1])) {
                result->values_.push_back(-other.values_[j2]);
                result->col_indices_.push_back(other.col_indices_[j2]);
                j2++;
            } else {
                double diff = values_[j1] - other.values_[j2];
                if (diff != 0.0) {
                    result->values_.push_back(diff);
                    result->col_indices_.push_back(col_indices_[j1]);
                }
                j1++;
                j2++;
            }
        }
        result->row_pointers_[i + 1] = result->values_.size();
    }
    
    return result;
}

Matrix SparseMatrix::multiply(const Matrix& other) const {
    if (cols_ != other.rows()) {
        throw std::runtime_error("Matrix dimensions must agree");
    }
    
    Matrix result(rows_, other.cols());
    
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = row_pointers_[i]; j < row_pointers_[i + 1]; ++j) {
            double val = values_[j];
            size_t col = col_indices_[j];
            for (size_t k = 0; k < other.cols(); ++k) {
                result(i, k) += val * other(col, k);
            }
        }
    }
    
    return result;
}

std::string SparseMatrix::toString() const {
    std::ostringstream oss;
    oss << "sparse matrix (" << rows_ << "x" << cols_ << ") with " << nnz() << " nonzeros:\n";
    oss << "  (" << rows_ << "," << cols_ << ") " << nnz() << " nonzeros\n";
    
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = row_pointers_[i]; j < row_pointers_[i + 1]; ++j) {
            oss << "    (" << (i + 1) << "," << (col_indices_[j] + 1) << ") -> " << values_[j] << "\n";
        }
    }
    
    return oss.str();
}

// LogicalArray implementation
LogicalArray::LogicalArray(size_t size) : rows_(1), cols_(size), values_(size, false) {}

LogicalArray::LogicalArray(size_t rows, size_t cols) : rows_(rows), cols_(cols), values_(rows * cols, false) {}

bool LogicalArray::get(size_t index) const {
    if (index < 1 || index > values_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return values_[index - 1];
}

bool LogicalArray::get(size_t row, size_t col) const {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    return values_[(row - 1) * cols_ + (col - 1)];
}

void LogicalArray::set(size_t index, bool value) {
    if (index < 1 || index > values_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    values_[index - 1] = value;
}

void LogicalArray::set(size_t row, size_t col, bool value) {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    values_[(row - 1) * cols_ + (col - 1)] = value;
}

Matrix LogicalArray::toDense() const {
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < values_.size(); ++i) {
        result(i) = values_[i] ? 1.0 : 0.0;
    }
    return result;
}

std::shared_ptr<LogicalArray> LogicalArray::fromDense(const Matrix& mat) {
    auto logical = std::make_shared<LogicalArray>(mat.rows(), mat.cols());
    for (size_t i = 0; i < mat.rows(); ++i) {
        for (size_t j = 0; j < mat.cols(); ++j) {
            logical->values_[i * mat.cols() + j] = (mat(i, j) != 0.0);
        }
    }
    return logical;
}

std::shared_ptr<LogicalArray> LogicalArray::fromVector(const std::vector<bool>& values) {
    auto logical = std::make_shared<LogicalArray>();
    logical->rows_ = 1;
    logical->cols_ = values.size();
    logical->values_ = values;
    return logical;
}

std::shared_ptr<LogicalArray> LogicalArray::fromVector(const std::vector<double>& values) {
    auto logical = std::make_shared<LogicalArray>();
    logical->rows_ = 1;
    logical->cols_ = values.size();
    logical->values_.resize(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        logical->values_[i] = (values[i] != 0.0);
    }
    return logical;
}

std::shared_ptr<LogicalArray> LogicalArray::logicalNot() const {
    auto result = std::make_shared<LogicalArray>(rows_, cols_);
    for (size_t i = 0; i < values_.size(); ++i) {
        result->values_[i] = !values_[i];
    }
    return result;
}

std::shared_ptr<LogicalArray> LogicalArray::logicalAnd(const LogicalArray& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Array dimensions must agree");
    }
    auto result = std::make_shared<LogicalArray>(rows_, cols_);
    for (size_t i = 0; i < values_.size(); ++i) {
        result->values_[i] = values_[i] && other.values_[i];
    }
    return result;
}

std::shared_ptr<LogicalArray> LogicalArray::logicalOr(const LogicalArray& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Array dimensions must agree");
    }
    auto result = std::make_shared<LogicalArray>(rows_, cols_);
    for (size_t i = 0; i < values_.size(); ++i) {
        result->values_[i] = values_[i] || other.values_[i];
    }
    return result;
}

std::shared_ptr<LogicalArray> LogicalArray::logicalXor(const LogicalArray& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Array dimensions must agree");
    }
    auto result = std::make_shared<LogicalArray>(rows_, cols_);
    for (size_t i = 0; i < values_.size(); ++i) {
        result->values_[i] = values_[i] != other.values_[i];
    }
    return result;
}

std::vector<double> LogicalArray::find() const {
    std::vector<double> indices;
    for (size_t i = 0; i < values_.size(); ++i) {
        if (values_[i]) {
            indices.push_back(static_cast<double>(i + 1)); // 1-based
        }
    }
    return indices;
}

size_t LogicalArray::nnz() const {
    size_t count = 0;
    for (bool val : values_) {
        if (val) count++;
    }
    return count;
}

std::string LogicalArray::toString() const {
    std::ostringstream oss;
    oss << "logical array (" << rows_ << "x" << cols_ << "):\n";
    
    if (rows_ == 1) {
        oss << "  [";
        for (size_t i = 0; i < values_.size(); ++i) {
            if (i > 0) oss << " ";
            oss << (values_[i] ? "true" : "false");
        }
        oss << "]";
    } else {
        for (size_t i = 0; i < rows_; ++i) {
            oss << "  [";
            for (size_t j = 0; j < cols_; ++j) {
                if (j > 0) oss << " ";
                oss << (values_[i * cols_ + j] ? "true" : "false");
            }
            oss << "]\n";
        }
    }
    
    return oss.str();
}

// CharArray implementation
CharArray::CharArray(const std::string& str) : rows_(1), cols_(str.length()) {
    chars_ = std::vector<char>(str.begin(), str.end());
}

CharArray::CharArray(size_t rows, size_t cols, char fillChar) : rows_(rows), cols_(cols), chars_(rows * cols, fillChar) {}

char CharArray::get(size_t index) const {
    if (index < 1 || index > chars_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    return chars_[index - 1];
}

char CharArray::get(size_t row, size_t col) const {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    return chars_[(row - 1) * cols_ + (col - 1)];
}

void CharArray::set(size_t index, char value) {
    if (index < 1 || index > chars_.size()) {
        throw std::runtime_error("Index out of bounds");
    }
    chars_[index - 1] = value;
}

void CharArray::set(size_t row, size_t col, char value) {
    if (row < 1 || row > rows_ || col < 1 || col > cols_) {
        throw std::runtime_error("Index out of bounds");
    }
    chars_[(row - 1) * cols_ + (col - 1)] = value;
}

std::string CharArray::toString() const {
    return std::string(chars_.begin(), chars_.end());
}

std::string CharArray::getRow(size_t row) const {
    if (row < 1 || row > rows_) {
        throw std::runtime_error("Row index out of bounds");
    }
    size_t start = (row - 1) * cols_;
    return std::string(chars_.begin() + start, chars_.begin() + start + cols_);
}

void CharArray::setRow(size_t row, const std::string& str) {
    if (row < 1 || row > rows_) {
        throw std::runtime_error("Row index out of bounds");
    }
    size_t start = (row - 1) * cols_;
    size_t len = std::min(str.length(), cols_);
    for (size_t i = 0; i < len; ++i) {
        chars_[start + i] = str[i];
    }
}

std::shared_ptr<CharArray> CharArray::fromString(const std::string& str) {
    return std::make_shared<CharArray>(str);
}

std::shared_ptr<CharArray> CharArray::fromStrings(const std::vector<std::string>& strings) {
    if (strings.empty()) {
        return std::make_shared<CharArray>();
    }
    size_t maxLen = 0;
    for (const auto& s : strings) {
        maxLen = std::max(maxLen, s.length());
    }
    auto charArray = std::make_shared<CharArray>(strings.size(), maxLen, ' ');
    for (size_t i = 0; i < strings.size(); ++i) {
        charArray->setRow(i + 1, strings[i]);
    }
    return charArray;
}

std::shared_ptr<StringArray> CharArray::toStringArray() const {
    std::vector<std::string> strings;
    for (size_t i = 1; i <= rows_; ++i) {
        strings.push_back(getRow(i));
    }
    return std::make_shared<StringArray>(strings);
}

std::shared_ptr<CharArray> CharArray::transpose() const {
    auto result = std::make_shared<CharArray>(cols_, rows_);
    for (size_t i = 1; i <= rows_; ++i) {
        for (size_t j = 1; j <= cols_; ++j) {
            result->set(j, i, get(i, j));
        }
    }
    return result;
}

std::shared_ptr<CharArray> CharArray::vertcat(const CharArray& other) const {
    if (cols_ != other.cols_) {
        throw std::runtime_error("Array dimensions must agree for vertical concatenation");
    }
    auto result = std::make_shared<CharArray>(rows_ + other.rows_, cols_);
    for (size_t i = 1; i <= rows_; ++i) {
        result->setRow(i, getRow(i));
    }
    for (size_t i = 1; i <= other.rows_; ++i) {
        result->setRow(rows_ + i, other.getRow(i));
    }
    return result;
}

std::shared_ptr<CharArray> CharArray::horzcat(const CharArray& other) const {
    if (rows_ != other.rows_) {
        throw std::runtime_error("Array dimensions must agree for horizontal concatenation");
    }
    auto result = std::make_shared<CharArray>(rows_, cols_ + other.cols_);
    for (size_t i = 1; i <= rows_; ++i) {
        std::string combined = getRow(i) + other.getRow(i);
        result->setRow(i, combined);
    }
    return result;
}

std::shared_ptr<LogicalArray> CharArray::equals(const CharArray& other) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) {
        throw std::runtime_error("Array dimensions must agree for comparison");
    }
    auto result = std::make_shared<LogicalArray>(rows_, cols_);
    for (size_t i = 1; i <= chars_.size(); ++i) {
        result->set(i, get(i) == other.get(i));
    }
    return result;
}

std::vector<double> CharArray::find(char c) const {
    std::vector<double> indices;
    for (size_t i = 0; i < chars_.size(); ++i) {
        if (chars_[i] == c) {
            indices.push_back(static_cast<double>(i + 1)); // 1-based
        }
    }
    return indices;
}

std::string CharArray::toDisplayString() const {
    std::ostringstream oss;
    if (rows_ == 1) {
        oss << "'" << toString() << "'";
    } else {
        oss << "char array (" << rows_ << "x" << cols_ << "):\n";
        for (size_t i = 1; i <= rows_; ++i) {
            oss << "    '" << getRow(i) << "'\n";
        }
    }
    return oss.str();
}

// FuncHandle implementation
FuncHandle::FuncHandle(const std::string& name) : type_(Type::NAMED_FUNCTION), name_(name) {}

FuncHandle::FuncHandle(const std::vector<std::string>& params, const std::string& expression)
    : type_(Type::ANONYMOUS_FUNCTION), parameters_(params), expression_(expression) {}

std::string FuncHandle::toString() const {
    if (type_ == Type::NAMED_FUNCTION) {
        return "@" + name_;
    } else {
        std::string result = "@(";
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameters_[i];
        }
        result += ") " + expression_;
        return result;
    }
}

bool FuncHandle::equals(const FuncHandle& other) const {
    if (type_ != other.type_) return false;
    if (type_ == Type::NAMED_FUNCTION) {
        return name_ == other.name_;
    } else {
        if (parameters_.size() != other.parameters_.size()) return false;
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (parameters_[i] != other.parameters_[i]) return false;
        }
        return expression_ == other.expression_;
    }
}

// Map implementation
bool Map::isKey(const std::string& key) const {
    return data_.find(key) != data_.end();
}

Value Map::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return it->second;
    }
    return std::monostate{};
}

void Map::set(const std::string& key, const Value& value) {
    data_[key] = value;
}

void Map::remove(const std::string& key) {
    data_.erase(key);
}

std::vector<std::string> Map::keys() const {
    std::vector<std::string> result;
    for (const auto& pair : data_) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<Value> Map::values() const {
    std::vector<Value> result;
    for (const auto& pair : data_) {
        result.push_back(pair.second);
    }
    return result;
}

std::string Map::toString() const {
    std::ostringstream oss;
    oss << "Map with " << size() << " entries:\n";
    for (const auto& pair : data_) {
        oss << "  '" << pair.first << "' => ";
        // Simple type display
        if (std::holds_alternative<double>(pair.second)) {
            oss << std::get<double>(pair.second);
        } else if (std::holds_alternative<std::string>(pair.second)) {
            oss << "'" << std::get<std::string>(pair.second) << "'";
        } else if (std::holds_alternative<bool>(pair.second)) {
            oss << (std::get<bool>(pair.second) ? "true" : "false");
        } else {
            oss << "[value]";
        }
        oss << "\n";
    }
    return oss.str();
}

// FileHandle implementation
FileHandle::FileHandle(const std::string& filename, const std::string& mode) 
    : filename_(filename), mode_(mode) {
    file_ = fopen(filename.c_str(), mode.c_str());
}

FileHandle::~FileHandle() {
    close();
}

FileHandle::FileHandle(FileHandle&& other) noexcept 
    : file_(other.file_), filename_(std::move(other.filename_)), mode_(std::move(other.mode_)) {
    other.file_ = nullptr;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept {
    if (this != &other) {
        close();
        file_ = other.file_;
        filename_ = std::move(other.filename_);
        mode_ = std::move(other.mode_);
        other.file_ = nullptr;
    }
    return *this;
}

void FileHandle::close() {
    if (file_) {
        fclose(file_);
        file_ = nullptr;
    }
}

std::string FileHandle::toString() const {
    std::ostringstream oss;
    oss << "file handle: " << filename_ << " (mode: " << mode_ << ", " 
        << (isOpen() ? "open" : "closed") << ")";
    return oss.str();
}

std::string MultiValue::toString() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < values_.size(); ++i) {
        if (i > 0) oss << ", ";
        if (std::holds_alternative<double>(values_[i])) {
            oss << std::get<double>(values_[i]);
        } else if (std::holds_alternative<std::string>(values_[i])) {
            oss << "'" << std::get<std::string>(values_[i]) << "'";
        } else if (std::holds_alternative<bool>(values_[i])) {
            oss << (std::get<bool>(values_[i]) ? "true" : "false");
        } else {
            oss << "[value]";
        }
    }
    oss << "]";
    return oss.str();
}

} // namespace cnlab
