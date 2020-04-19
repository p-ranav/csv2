#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <functional>
#include <locale>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace csv2 {

namespace details {

template <bool condition> struct if_else;

template <> struct if_else<true> { using type = std::true_type; };

template <> struct if_else<false> { using type = std::false_type; };

template <bool condition, typename True, typename False> struct if_else_type;

template <typename True, typename False> struct if_else_type<true, True, False> {
  using type = True;
};

template <typename True, typename False> struct if_else_type<false, True, False> {
  using type = False;
};

template <typename... Ops> struct conjuction;

template <> struct conjuction<> : std::true_type {};

template <typename Op, typename... TailOps>
struct conjuction<Op, TailOps...>
    : if_else_type<!Op::value, std::false_type, conjuction<TailOps...>>::type {};

template <typename... Ops> struct disjunction;

template <> struct disjunction<> : std::false_type {};

template <typename Op, typename... TailOps>
struct disjunction<Op, TailOps...>
    : if_else_type<Op::value, std::true_type, disjunction<TailOps...>>::type {};

enum class CsvOption {
  filename = 0,
  delimiter,
  column_names,
  ignore_columns,
  skip_empty_rows,
  quote_character,
  skip_initial_space
};

template <typename T, CsvOption Id> struct Setting {
  template <typename... Args,
            typename = typename std::enable_if<std::is_constructible<T, Args...>::value>::type>
  explicit Setting(Args &&... args) : value(std::forward<Args>(args)...) {}
  Setting(const Setting &) = default;
  Setting(Setting &&) = default;

  static constexpr auto id = Id;
  using type = T;

  T value{};
};

template <typename T> struct is_setting : std::false_type {};

template <CsvOption Id, typename T> struct is_setting<Setting<T, Id>> : std::true_type {};

template <typename... Args>
struct are_settings : if_else<conjuction<is_setting<Args>...>::value>::type {};

template <> struct are_settings<> : std::true_type {};

template <typename Setting, typename Tuple> struct is_setting_from_tuple;

template <typename Setting> struct is_setting_from_tuple<Setting, std::tuple<>> : std::true_type {};

template <typename Setting, typename... TupleTypes>
struct is_setting_from_tuple<Setting, std::tuple<TupleTypes...>>
    : if_else<disjunction<std::is_same<Setting, TupleTypes>...>::value>::type {};

template <typename Tuple, typename... Settings>
struct are_settings_from_tuple
    : if_else<conjuction<is_setting_from_tuple<Settings, Tuple>...>::value>::type {};

template <CsvOption Id> struct always_true { static constexpr auto value = true; };

template <CsvOption Id, typename Default> Default &&get_impl(Default &&def) {
  return std::forward<Default>(def);
}

template <CsvOption Id, typename Default, typename T, typename... Args>
auto get_impl(Default && /*def*/, T &&first, Args &&... /*tail*/) ->
    typename std::enable_if<(std::decay<T>::type::id == Id),
                            decltype(std::forward<T>(first))>::type {
  return std::forward<T>(first);
}

template <CsvOption Id, typename Default, typename T, typename... Args>
auto get_impl(Default &&def, T && /*first*/, Args &&... tail) ->
    typename std::enable_if<(std::decay<T>::type::id != Id),
                            decltype(get_impl<Id>(std::forward<Default>(def),
                                                  std::forward<Args>(tail)...))>::type {
  return get_impl<Id>(std::forward<Default>(def), std::forward<Args>(tail)...);
}

template <CsvOption Id, typename Default, typename... Args,
          typename = typename std::enable_if<are_settings<Args...>::value, void>::type>
auto get(Default &&def, Args &&... args)
    -> decltype(details::get_impl<Id>(std::forward<Default>(def), std::forward<Args>(args)...)) {
  return details::get_impl<Id>(std::forward<Default>(def), std::forward<Args>(args)...);
}

template <CsvOption Id> using CharSetting = Setting<char, Id>;

template <CsvOption Id> using StringSetting = Setting<std::string, Id>;

template <CsvOption Id> using IntegerSetting = Setting<std::size_t, Id>;

template <CsvOption Id> using BooleanSetting = Setting<bool, Id>;

template <CsvOption Id, typename Tuple, std::size_t counter = 0> struct option_idx;

template <CsvOption Id, typename T, typename... Settings, std::size_t counter>
struct option_idx<Id, std::tuple<T, Settings...>, counter>
    : if_else_type<(Id == T::id), std::integral_constant<std::size_t, counter>,
                   option_idx<Id, std::tuple<Settings...>, counter + 1>>::type {};

template <CsvOption Id, std::size_t counter> struct option_idx<Id, std::tuple<>, counter> {
  static_assert(always_true<(CsvOption)Id>::value, "No such option was found");
};

template <CsvOption Id, typename Settings>
auto get_value(Settings &&settings)
    -> decltype((std::get<option_idx<Id, typename std::decay<Settings>::type>::value>(
        std::declval<Settings &&>()))) {
  return std::get<option_idx<Id, typename std::decay<Settings>::type>::value>(
      std::forward<Settings>(settings));
}

} // namespace details

enum class Trim { none, leading, trailing, leading_and_trailing };

namespace option {
using Filename = details::StringSetting<details::CsvOption::filename>;
using Delimiter = details::CharSetting<details::CsvOption::delimiter>;
using ColumnNames = details::Setting<std::vector<std::string>, details::CsvOption::column_names>;
using IgnoreColumns =
    details::Setting<std::vector<std::string>, details::CsvOption::ignore_columns>;
using SkipEmptyRows = details::BooleanSetting<details::CsvOption::skip_empty_rows>;
using QuoteCharacter = details::CharSetting<details::CsvOption::quote_character>;
using SkipInitialSpace = details::BooleanSetting<details::CsvOption::skip_initial_space>;
} // namespace option

namespace string {

// trim from end (in place)
static inline void rtrim(std::string &s, const std::vector<char> &t) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [&t](int ch) { return std::find(t.begin(), t.end(), ch) == t.end(); })
              .base(),
          s.end());
}

} // namespace string

using Row = std::unordered_map<std::string_view, std::string_view>;

class Reader {
  size_t lines_{0};
  std::string header_string_;
  std::vector<std::string> line_strings_;
  std::vector<std::string> header_tokens_;
  std::vector<std::string_view> row_tokens_;
  std::string empty_{""};
  std::string_view current_row_;
  size_t current_row_index_{0};
  char delimiter_;
  char quote_character_;
  bool skip_initial_space_{false};
  std::vector<std::string> ignore_columns_;
  const std::vector<char> trim_characters_{'\n', '\r'};

  using Settings = std::tuple<option::Filename, option::Delimiter,
                              option::ColumnNames, option::IgnoreColumns, option::SkipEmptyRows,
                              option::QuoteCharacter, option::SkipInitialSpace>;
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
  void read_file_fast_(std::ifstream &file, LineHandler &&line_handler) {
    int64_t buffer_size = 40000;
    file.seekg(0, std::ios::end);
    std::ifstream::pos_type p = file.tellg();
#ifdef WIN32
    int64_t file_size = *(int64_t *)(((char *)&p) + 8);
#else
    int64_t file_size = p;
#endif
    file.seekg(0, std::ios::beg);
    buffer_size = std::min(buffer_size, file_size);
    char *buffer = new char[buffer_size];
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
          line_handler(buffer + string_start + 1, buffer_length);
          buffer_position_in_file += buffer_length;
          buffer_length = std::min(buffer_length, file_size - buffer_position_in_file);
          delete[] buffer;
          buffer = new char[buffer_length];
          file.read(buffer, buffer_length);
        } else {
          int moved_length = buffer_length - string_start - 1;
          memmove(buffer, buffer + string_start + 1, moved_length);
          buffer_position_in_file += string_start + 1;
          int read_size = std::min(buffer_length - moved_length,
                                   file_size - buffer_position_in_file - moved_length);

          if (read_size != 0)
            file.read(buffer + moved_length, read_size);
          if (moved_length + read_size < buffer_length) {
            char *temp_buffer = new char[moved_length + read_size];
            memmove(temp_buffer, buffer, moved_length + read_size);
            delete[] buffer;
            buffer = temp_buffer;
            buffer_length = moved_length + read_size;
          }
          string_end = -1;
        }
      } else {
        line_handler(buffer + string_start + 1, string_end - string_start);
      }
    }
    line_handler(0, 0); // eof
  }

  enum class CSVState { UnquotedField, QuotedField, QuotedQuote };

  std::vector<std::string_view> tokenize_current_row_() {
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
          field_end = field_start;                            // reset interval
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
          field_end = field_start;                            // reset interval
          i++;
          state = CSVState::UnquotedField;
        } else if (c == quote_character_) { // "" -> "
          if (header_tokens_.empty()) { // this is the header row, mutate header_string_
            header_string_ = header_string_.erase(field_end, 1);
            current_row_ = header_string_;
          } else { // this is not the header row, mutate line_strings_[index]
            line_strings_[current_row_index_] = line_strings_[current_row_index_].erase(field_end, 1);
            current_row_ = header_string_;
          }
          j = j - 1; // update index since 1 quote character has been removed
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

  void read_file_(std::ifstream infile) {
    const auto &skip_empty_rows = get_value<details::CsvOption::skip_empty_rows>();

    read_file_fast_(infile, [&, this](char *buffer, int length) -> void {
      if (!buffer) return;
      auto line = std::string{buffer, static_cast<size_t>(length)};
      string::rtrim(line, trim_characters_);
      if (skip_empty_rows && line.empty())
        return;
      if (header_tokens_.empty()) {
        header_string_ = std::move(line);
        current_row_ = header_string_;
        const auto &&header_tokens = tokenize_current_row_();
        header_tokens_ = std::vector<std::string>(header_tokens.begin(), header_tokens.end());
        return;
      }
      lines_ += 1;
      line_strings_.push_back(std::move(line));
    });
  }

public:
  template <typename... Args,
            typename std::enable_if<details::are_settings_from_tuple<
                                        Settings, typename std::decay<Args>::type...>::value,
                                    void *>::type = nullptr>
  Reader(Args &&... args)
      : settings_(
            details::get<details::CsvOption::filename>(option::Filename{""},
                                                       std::forward<Args>(args)...),
            details::get<details::CsvOption::delimiter>(option::Delimiter{','},
                                                        std::forward<Args>(args)...),
            details::get<details::CsvOption::column_names>(option::ColumnNames{},
                                                           std::forward<Args>(args)...),
            details::get<details::CsvOption::ignore_columns>(option::IgnoreColumns{},
                                                             std::forward<Args>(args)...),
            details::get<details::CsvOption::skip_empty_rows>(option::SkipEmptyRows{false},
                                                              std::forward<Args>(args)...),
            details::get<details::CsvOption::quote_character>(option::QuoteCharacter{'"'},
                                                              std::forward<Args>(args)...),
            details::get<details::CsvOption::skip_initial_space>(option::SkipInitialSpace{false},
                                                                 std::forward<Args>(args)...)) {
    const auto &filename = get_value<details::CsvOption::filename>();
    header_tokens_ = get_value<details::CsvOption::column_names>();
    delimiter_ = get_value<details::CsvOption::delimiter>();
    ignore_columns_ = get_value<details::CsvOption::ignore_columns>();
    quote_character_ = get_value<details::CsvOption::quote_character>();
    skip_initial_space_ = get_value<details::CsvOption::skip_initial_space>();

    // Large I/O buffer to speed up ifstream read
    const uint64_t stream_buffer_size = 1000000;
    char stream_buffer[stream_buffer_size];

    std::ios_base::sync_with_stdio(false);
    std::ifstream infile;
    infile.rdbuf()->pubsetbuf(stream_buffer, stream_buffer_size);
    infile.open(filename);

    if (!infile.is_open())
      throw std::runtime_error("error: Failed to open file " + filename);

    read_file_(std::move(infile));
  }

  bool read_row(Row &result) {
    if (current_row_index_ >= line_strings_.size())
      return false;
    current_row_ = line_strings_[current_row_index_];
    row_tokens_ = tokenize_current_row_();
    result.clear();
    for (size_t i = 0; i < header_tokens_.size(); ++i) {
      if (!ignore_columns_.empty() && std::find(ignore_columns_.begin(), ignore_columns_.end(),
                                                header_tokens_[i]) != ignore_columns_.end())
        continue;
      if (i < row_tokens_.size()) {
        result.insert({header_tokens_[i], row_tokens_[i]});
      } else {
        result.insert({header_tokens_[i], empty_});
      }
    }
    current_row_index_ += 1;
    return true;
  }

  size_t rows() const { return lines_; }

  size_t cols() const {
    return header_tokens_.size() - get_value<details::CsvOption::ignore_columns>().size();
  }

  std::vector<std::string_view> header() const {
    std::vector<std::string_view> result;
    for (auto &h : header_tokens_)
      result.push_back(h);
    return result;
  }

};

} // namespace csv2