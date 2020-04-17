#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <csv2/task_system.hpp>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <csv2/string_utils.hpp>

namespace csv2 {

using namespace std;

class reader {
    char delimiter_{','};
    task_system t_{1};
    std::vector<std::string> header_;
    size_t current_row_{0};

    template <typename LineHandler>
    void read_file_fast(ifstream &file, LineHandler &&line_handler){
            int64_t buffer_size = 40000;
            file.seekg(0,ios::end);
            ifstream::pos_type p = file.tellg();
    #ifdef WIN32
            int64_t file_size = *(int64_t*)(((char*)&p) +8);
    #else
            int64_t file_size = p;
    #endif
            file.seekg(0,ios::beg);
            buffer_size = min(buffer_size, file_size);
            char* buffer = new char[buffer_size];
            blkcnt_t buffer_length = buffer_size;
            file.read(buffer, buffer_length);

            int string_end = -1;
            int string_start;
            int64_t buffer_position_in_file = 0;
            while (buffer_length > 0) {
                int i = string_end + 1;
                string_start = string_end;
                string_end = -1;
                for (; i < buffer_length && i + buffer_position_in_file < file_size; i++) {
                    if (buffer[i] == '\n') {
                        string_end = i;
                        break;
                    }
                }

                if (string_end == -1) { // scroll buffer
                    if (string_start == -1) {
                        line_handler(buffer + string_start + 1, buffer_length, buffer_position_in_file + string_start + 1);
                        buffer_position_in_file += buffer_length;
                        buffer_length = min(buffer_length, file_size - buffer_position_in_file);
                        delete[]buffer;
                        buffer = new char[buffer_length];
                        file.read(buffer, buffer_length);
                    } else {
                        int moved_length = buffer_length - string_start - 1;
                        memmove(buffer,buffer+string_start+1,moved_length);
                        buffer_position_in_file += string_start + 1;
                        int readSize = min(buffer_length - moved_length, file_size - buffer_position_in_file - moved_length);

                        if (readSize != 0)
                            file.read(buffer + moved_length, readSize);
                        if (moved_length + readSize < buffer_length) {
                            char *temp_buffer = new char[moved_length + readSize];
                            memmove(temp_buffer,buffer,moved_length+readSize);
                            delete[]buffer;
                            buffer = temp_buffer;
                            buffer_length = moved_length + readSize;
                        }
                        string_end = -1;
                    }
                } else {
                    line_handler(buffer+ string_start + 1, string_end - string_start, buffer_position_in_file + string_start + 1);
                }
            }
            line_handler(0, 0, 0);//eof
    }

    enum class CSVState {
        UnquotedField,
        QuotedField,
        QuotedQuote
    };

    std::vector<std::string> tokenize_row(const std::string &row) {
        CSVState state = CSVState::UnquotedField;
        std::vector<std::string> fields {""};
        size_t i = 0; // index of the current field
        for (char c : row) {
            switch (state) {
                case CSVState::UnquotedField:
                    if (c == delimiter_) {
                        fields.push_back(""); 
                        i++;
                    } else if (c == '"') {
                        state = CSVState::QuotedField;
                    } else {
                        fields[i].push_back(c);
                    }
                    break;
                case CSVState::QuotedField:
                    switch (c) {
                        case '"': state = CSVState::QuotedQuote;
                                break;
                        default:  fields[i].push_back(c);
                                break; }
                    break;
                case CSVState::QuotedQuote:
                    switch (c) {
                        if (c == delimiter_) { // , after closing quote
                            fields.push_back(""); 
                            i++;
                            state = CSVState::UnquotedField;
                        } else if (c == '"') { // "" -> "
                            fields[i].push_back('"');
                            state = CSVState::QuotedField;
                        } else {
                            state = CSVState::UnquotedField;
                        }
                    break;
            }
        }
        }
        return fields;
    }

    std::optional<std::unordered_map<std::string_view, std::string>>
    try_read_row(size_t index) {
        const auto it = t_.find_row(index + 1); // index = 0 is the header
        if (it != t_.rows_.end()) {
            const auto row = tokenize_row(it->second);
            std::unordered_map<std::string_view, std::string> result;
            for (size_t i = 0; i < header_.size(); ++i) {
                if (i <= row.size())
                  result.insert({header_[i], row[i]});
                else 
                  result.insert({header_[i], ""});
            }
            {
                // For the last key-value pair, rtrim to remove \n or \r\n
                const size_t i = header_.size() - 1;
                if (i <= row.size())
                  result.insert({header_[i], rtrim_copy(row[i])});
                else 
                  result.insert({header_[i], ""});
            }
            return result;
        }
        return std::nullopt;
    }

public:
    reader(std::string filename, char delimiter = ','): delimiter_(delimiter) {
        t_.start();
        ifstream infile(filename);
        unsigned line_no = 1;
        read_file_fast(infile, [&, this](char*buffer, int length, int64_t position) -> void {
            if (!buffer) return;
            if (!header_.size()) {
              header_ = tokenize_row(std::string(buffer, length));
              rtrim(header_[header_.size() - 1]); // in-place rtrim the last header
              return;
            }
            t_.async_(std::make_pair(line_no++, std::string(buffer, length)));
        });
        t_.stop();
    }

    bool read_row(std::unordered_map<std::string_view, std::string> &result) {
        if (current_row_ == t_.rows_.size())
            return false;
        std::optional<std::unordered_map<std::string_view, std::string>> row;
        do {
          row = try_read_row(current_row_);
        } while(!row.has_value());
        current_row_ += 1;
        result = row.value();
        return true;
    }

    size_t rows() {
        return t_.rows();
    }

    size_t cols() {
        return header_.size();
    }

    auto header() const {
        std::vector<std::string_view> result;
        for (auto& h: header_)
            result.push_back(h);
        return result;
    }
};

}