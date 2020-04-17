#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <csv2/task_system.hpp>
#include <algorithm>
#include <cmath>
#include <string_view>

#include <cctype>
#include <locale>

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

namespace csv2 {

using namespace std;

class reader {
    task_system t_{1};
    std::vector<std::string> header_;
public:
    reader(std::string filename) {
        t_.start();
        ifstream infile(filename);
        unsigned line_no = 1;
        read_file_fast(infile, [&, this](char*buffer, int length, int64_t position) -> void {
            if (!buffer) return;
            if (!header_.size()) {
              header_ = read_csv_row(std::string(buffer, length));
              rtrim(header_[header_.size() - 1]); // in-place rtrim the last header
              return;
            }
            t_.async_(std::make_pair(line_no++, std::string(buffer, length)));
        });
        t_.stop();
    }

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

    std::vector<std::string> read_csv_row(const std::string &row) {
        CSVState state = CSVState::UnquotedField;
        std::vector<std::string> fields {""};
        size_t i = 0; // index of the current field
        for (char c : row) {
            switch (state) {
                case CSVState::UnquotedField:
                    switch (c) {
                        case ',': // end of field
                                fields.push_back(""); i++;
                                break;
                        case '"': state = CSVState::QuotedField;
                                break;
                        default:  fields[i].push_back(c);
                                break; }
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
                        case ',': // , after closing quote
                                fields.push_back(""); i++;
                                state = CSVState::UnquotedField;
                                break;
                        case '"': // "" -> "
                                fields[i].push_back('"');
                                state = CSVState::QuotedField;
                                break;
                        default:  // end of quote
                                state = CSVState::UnquotedField;
                                break; }
                    break;
            }
        }
        return fields;
    }

    std::optional<std::unordered_map<std::string_view, std::string>>
    operator[](size_t index) {
        auto it = t_.rows_.find(index);
        if (it != t_.rows_.end()) {
            auto row = read_csv_row(it->second);
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

};

}