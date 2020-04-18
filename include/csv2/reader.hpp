#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <cstring>
#include <csv2/setting.hpp>
#include <tuple>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <locale>
#include <string>

namespace csv2 {

namespace string {

    // trim from start (in place)
    static inline void ltrim(std::string &s, const std::vector<char>& t) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&t](int ch) {
            return std::find(t.begin(), t.end(), ch) == t.end();
        }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s, const std::vector<char>& t) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [&t](int ch) {
            return std::find(t.begin(), t.end(), ch) == t.end();
        }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string &s, const std::vector<char>& t) {
        ltrim(s, t);
        rtrim(s, t);
    }
}

using Row  = std::unordered_map<std::string_view, std::string_view>;

class Reader {
    size_t lines_{0};
    std::vector<std::string> line_strings_;
    std::vector<std::string> header_tokens_;
    std::vector<std::string_view> row_tokens_;
    std::string empty_{""};
    std::string_view current_row_;
    size_t current_row_index_{0};
    char delimiter_;
    char quote_character_;
    std::function<void(std::string &s, const std::vector<char>& t)> trim_function_;
    bool skip_initial_space_{false};
    std::vector<std::string> ignore_columns_;

    using Settings = std::tuple<option::Filename, 
        option::Delimiter, 
        option::TrimCharacters,
        option::ColumnNames,
        option::IgnoreColumns,
        option::SkipEmptyRows,
        option::QuoteCharacter,
        option::TrimPolicy,
        option::SkipInitialSpace>;
    Settings settings_;

    template <details::CsvOption id>
    auto get_value() -> decltype((details::get_value<id>(std::declval<Settings &>()).value)) {
        return details::get_value<id>(settings_).value;
    }

    template <details::CsvOption id>
    auto get_value() const
        -> decltype((details::get_value<id>(std::declval<const Settings &>()).value)) {
        return details::get_value<id>(settings_).value;
    }

    template <typename LineHandler>
    void read_file_fast(std::ifstream &file, LineHandler &&line_handler){
            int64_t buffer_size = 40000;
            file.seekg(0, std::ios::end);
            std::ifstream::pos_type p = file.tellg();
    #ifdef WIN32
            int64_t file_size = *(int64_t*)(((char*)&p) +8);
    #else
            int64_t file_size = p;
    #endif
            file.seekg(0, std::ios::beg);
            buffer_size = std::min(buffer_size, file_size);
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
                        buffer_length = std::min(buffer_length, file_size - buffer_position_in_file);
                        delete[]buffer;
                        buffer = new char[buffer_length];
                        file.read(buffer, buffer_length);
                    } else {
                        int moved_length = buffer_length - string_start - 1;
                        memmove(buffer,buffer+string_start+1,moved_length);
                        buffer_position_in_file += string_start + 1;
                        int readSize = std::min(buffer_length - moved_length, file_size - buffer_position_in_file - moved_length);

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

    std::vector<std::string_view> tokenize_current_row() {
        std::cout << current_row_ << std::endl;
        CSVState state = CSVState::UnquotedField;
        std::vector<std::string_view> fields;
        size_t i = 0; // index of the current field

        int field_start = 0;
        int field_end = 0;

        for (size_t j = 0; j < current_row_.size(); ++j) {
            char c = current_row_[j];
            switch (state) {
                case CSVState::UnquotedField:
                    if (c == delimiter_) {
                        // Check for initial space right after delimiter
                        size_t initial_space_offset{0};
                        if (skip_initial_space_ && j + 1 < current_row_.size() && current_row_[j + 1] == ' ') {
                            initial_space_offset = 1;
                            j++;
                        }

                        fields.push_back(current_row_.substr(field_start, field_end - field_start));
                        field_start = field_end + 1 + initial_space_offset; // start after delimiter
                        field_end = field_start; // reset interval
                        i++;
                    } else if (c == quote_character_) {
                        field_end += 1;
                        state = CSVState::QuotedField;
                    } else {
                        field_end += 1;
                        if (j + 1 == current_row_.size()) { // last entry
                            fields.push_back(current_row_.substr(field_start, field_end - field_start));
                        }
                    }
                    break;
                case CSVState::QuotedField:
                    if (c == quote_character_) {
                        field_end += 1;
                        state = CSVState::QuotedQuote;
                        if (j + 1 == current_row_.size()) { // last entry
                            fields.push_back(current_row_.substr(field_start, field_end - field_start));
                        }
                    } else {
                        field_end += 1;
                    }
                    break;
                case CSVState::QuotedQuote:
                    if (c == delimiter_) { // , after closing quote
                        // Check for initial space right after delimiter
                        size_t initial_space_offset{0};
                        if (skip_initial_space_ && j + 1 < current_row_.size() && current_row_[j + 1] == ' ') {
                            initial_space_offset = 1;
                            j++;
                        }

                        fields.push_back(current_row_.substr(field_start, field_end - field_start));
                        field_start = field_end + 1 + initial_space_offset; // start after delimiter
                        field_end = field_start; // reset interval
                        i++;
                        state = CSVState::UnquotedField;
                    } else if (c == quote_character_) { // "" -> "
                        field_end += 1;
                        state = CSVState::QuotedField;
                    } else {
                        field_end += 1;
                        state = CSVState::UnquotedField;
                    }
                    break;
            }
        }
        return fields;
    }

    bool
    try_read_row(Row &result) {
        if (current_row_index_ < lines_) {
            current_row_ = line_strings_[current_row_index_];
            row_tokens_ = tokenize_current_row();
            result.clear();
            for (size_t i = 0; i < header_tokens_.size(); ++i) {
                if (!ignore_columns_.empty() && std::find(ignore_columns_.begin(), ignore_columns_.end(), header_tokens_[i]) != ignore_columns_.end())
                    continue;
                if (i < row_tokens_.size()) {
                  result.insert({header_tokens_[i], row_tokens_[i]});
                }
                else {
                  result.insert({header_tokens_[i], empty_});
                }
            }
            return true;
        }
        return false;
    }

public:
    template <typename... Args,
            typename std::enable_if<details::are_settings_from_tuple<
                                        Settings, typename std::decay<Args>::type...>::value,
                                    void *>::type = nullptr>
    Reader(Args &&... args)
        : settings_(
            details::get<details::CsvOption::filename>(option::Filename{""}, std::forward<Args>(args)...),
            details::get<details::CsvOption::delimiter>(option::Delimiter{','}, std::forward<Args>(args)...),
            details::get<details::CsvOption::trim_characters>(option::TrimCharacters{std::vector<char>{'\n', '\r'}}, std::forward<Args>(args)...),
            details::get<details::CsvOption::column_names>(option::ColumnNames{}, std::forward<Args>(args)...),
            details::get<details::CsvOption::ignore_columns>(option::IgnoreColumns{}, std::forward<Args>(args)...),
            details::get<details::CsvOption::skip_empty_rows>(option::SkipEmptyRows{false}, std::forward<Args>(args)...),
            details::get<details::CsvOption::quote_character>(option::QuoteCharacter{'"'}, std::forward<Args>(args)...),
            details::get<details::CsvOption::trim_policy>(option::TrimPolicy{Trim::trailing}, std::forward<Args>(args)...),
            details::get<details::CsvOption::skip_initial_space>(option::SkipInitialSpace{false}, std::forward<Args>(args)...)
        ) {
        auto& filename = get_value<details::CsvOption::filename>();
        auto& trim_characters = get_value<details::CsvOption::trim_characters>();
        auto& skip_empty_rows = get_value<details::CsvOption::skip_empty_rows>();
        auto& column_names = get_value<details::CsvOption::column_names>();
        delimiter_ = get_value<details::CsvOption::delimiter>();
        ignore_columns_ = get_value<details::CsvOption::ignore_columns>();
        quote_character_ = get_value<details::CsvOption::quote_character>();
        skip_initial_space_ = get_value<details::CsvOption::skip_initial_space>();
        auto &trim_policy = get_value<details::CsvOption::trim_policy>();
        switch(trim_policy) {
            case Trim::none:
                trim_function_ = {};
                break;
            case Trim::leading:
                trim_function_ = string::ltrim;
                break;
            case Trim::trailing:
                trim_function_ = string::rtrim;
                break;
            case Trim::leading_and_trailing:
                trim_function_ = string::trim;
                break;
        }

        // NOTE: Trimming happens at the row level and not at the field level

        if (column_names.size())
            header_tokens_ = column_names;
            
        std::ifstream infile(filename);
        if (!infile.is_open())
            throw std::runtime_error("error: Failed to open " + filename);

        read_file_fast(infile, [&, this](char*buffer, int length, int64_t position) -> void {
            if (!buffer) return;
            auto line = std::string{buffer, static_cast<size_t>(length)};
            if (trim_function_)
                trim_function_(line, trim_characters);
            if (skip_empty_rows && line.empty())
                return;
            if (!header_tokens_.size()) {
              current_row_ = line;
              const auto&& header_tokens = tokenize_current_row();
              header_tokens_ = std::vector<std::string>(header_tokens.begin(), header_tokens.end());
              return;
            }
            lines_ += 1;
            line_strings_.push_back(std::move(line));
        });
    }

    bool read_row(Row &result) {
        if (current_row_index_ == lines_)
            return false;
        try_read_row(result);
        current_row_index_ += 1;
        return true;
    }

    size_t rows() const {
        return lines_;
    }

    size_t cols() const {
        return header_tokens_.size() - get_value<details::CsvOption::ignore_columns>().size();
    }

    std::vector<std::string_view> header() const {
        std::vector<std::string_view> result;
        for (auto& h: header_tokens_)
            result.push_back(h);
        return result;
    }
};

}