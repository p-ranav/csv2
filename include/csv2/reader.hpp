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
  delimiter = 0,
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
using Delimiter = details::CharSetting<details::CsvOption::delimiter>;
using ColumnNames = details::Setting<std::vector<std::string>, details::CsvOption::column_names>;
using IgnoreColumns =
    details::Setting<std::vector<std::string>, details::CsvOption::ignore_columns>;
using SkipEmptyRows = details::BooleanSetting<details::CsvOption::skip_empty_rows>;
using QuoteCharacter = details::CharSetting<details::CsvOption::quote_character>;
using SkipInitialSpace = details::BooleanSetting<details::CsvOption::skip_initial_space>;
} // namespace option

class Cell {
  std::string_view raw_;
  bool escaped_{false};
  bool end_of_row_{false};
  friend class Reader;

public:
  std::string_view raw() {
    return raw_;
  }

  template <typename Container>
  std::string converted(Container& result) {
    result = Container(raw_.begin(), raw_.end());
    for (size_t i = 1; i < result.size(); ++i) {
      if (result[i] == '"' && result[i - 1] == '"') {
        result.erase(i - 1, 1);
      }
    }
    return result;
  }
};

using Row = std::vector<Cell>;

class Reader {
  std::size_t lines_{0};
  std::string header_string_;
  std::vector<std::string> line_strings_;
  Row header_tokens_;
  Row row_tokens_;
  std::string empty_{""};
  std::string_view current_row_;
  std::size_t current_row_index_{0};
  char delimiter_;
  char quote_character_;
  bool skip_initial_space_{false};
  std::vector<std::string> ignore_columns_;

  using Settings =
      std::tuple<option::Delimiter, option::ColumnNames, option::IgnoreColumns,
                 option::SkipEmptyRows, option::QuoteCharacter, option::SkipInitialSpace>;
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
    auto buffer_length = buffer_size;
    file.read(buffer, buffer_length);

    int string_end = -1;
    int string_start{0};
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
    delete[] buffer;
    line_handler(nullptr, 0); // eof
  }

  bool next_column_end_(size_t &end, bool& escaped, const std::size_t& current_row_size) {
    if (end >= current_row_.size())
      return false;
    std::size_t last_quote_location = 0;
    bool quote_opened = false;

    while (end < current_row_size &&
            (current_row_[end] != delimiter_ || 
            (current_row_[end] == delimiter_ && quote_opened))) {
      if (current_row_[end] == quote_character_) {
        quote_opened = true;

        if (end + 1 < current_row_size && current_row_[end + 1] == delimiter_) {
          // end of field, quote is closed, moving on
          end += 1;
          return true;
        }

        // Check if previous character was also a quote
        if (last_quote_location == end - 1)
          escaped = true;
        last_quote_location = end;
      }
      end += 1;
    }
    return true;
  }

  Row tokenize_header() {
    std::size_t start = 0, end = start;
    const size_t current_row_size = current_row_.size();
    bool escaped = false;
    Row result;
    while (next_column_end_(end, escaped, current_row_size)) {
      Cell c;
      c.raw_ = std::string_view(current_row_).substr(start, end - start);
      c.escaped_ = escaped;
      // end is at the delimiter
      start = end + 1;
      end = start;
      if (end >= current_row_size)
        c.end_of_row_ = true;
      result.push_back(std::move(c));
      escaped = false;
    }
    return result;
  }

  void tokenize_current_row_(Row& result) {
    std::size_t start = 0, end = start;
    const size_t current_row_size = current_row_.size();
    bool escaped = false;
    result.clear();
    while (next_column_end_(end, escaped, current_row_size)) {
      Cell c;
      c.raw_ = std::string_view(current_row_).substr(start, end - start);
      c.escaped_ = escaped;
      // end is at the delimiter
      start = end + 1;
      end = start;
      if (end >= current_row_size)
        c.end_of_row_ = true;
      result.push_back(std::move(c));
      escaped = false;
    }
  }

  void read_file_(std::ifstream infile) {
    const auto &skip_empty_rows = get_value<details::CsvOption::skip_empty_rows>();

    read_file_fast_(infile, [&, this](char *buffer, int length) -> void {
      if (!buffer)
        return;
      std::string line = std::string{buffer, static_cast<size_t>(length)};

      if (skip_empty_rows && line.empty())
        return;
      if (header_tokens_.empty()) {
        header_string_ = std::move(line);
        current_row_ = header_string_;
        header_tokens_ = tokenize_header();
        row_tokens_.reserve(header_tokens_.size());
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
      : settings_(details::get<details::CsvOption::delimiter>(option::Delimiter{','},
                                                              std::forward<Args>(args)...),
                  details::get<details::CsvOption::column_names>(option::ColumnNames{},
                                                                 std::forward<Args>(args)...),
                  details::get<details::CsvOption::ignore_columns>(option::IgnoreColumns{},
                                                                   std::forward<Args>(args)...),
                  details::get<details::CsvOption::skip_empty_rows>(option::SkipEmptyRows{false},
                                                                    std::forward<Args>(args)...),
                  details::get<details::CsvOption::quote_character>(option::QuoteCharacter{'"'},
                                                                    std::forward<Args>(args)...),
                  details::get<details::CsvOption::skip_initial_space>(
                      option::SkipInitialSpace{false}, std::forward<Args>(args)...)) {
    delimiter_ = get_value<details::CsvOption::delimiter>();
    ignore_columns_ = get_value<details::CsvOption::ignore_columns>();
    quote_character_ = get_value<details::CsvOption::quote_character>();
    skip_initial_space_ = get_value<details::CsvOption::skip_initial_space>();
  }

  template <typename StringType>
  bool open(StringType&& filename) {
    // Prepare to parse new file
    lines_ = 0;
    header_string_.clear();
    line_strings_.clear();
    current_row_index_ = 0;

    // Large I/O buffer to speed up ifstream read
    const uint64_t stream_buffer_size = 1000000;
    char stream_buffer[stream_buffer_size];

    std::ios_base::sync_with_stdio(false);
    std::ifstream infile;
    infile.rdbuf()->pubsetbuf(stream_buffer, stream_buffer_size);
    infile.open(std::forward<StringType>(filename));
    if (!infile.is_open())
      return false;
    read_file_(std::move(infile)); 
    return true;
  }

  bool read_row(Row& result) {
    if (current_row_index_ >= line_strings_.size())
      return false;
    current_row_ = line_strings_[current_row_index_];
    tokenize_current_row_(result);
    current_row_index_ += 1;
    return true;
  }

  std::size_t rows() const { return lines_; }

  std::size_t cols() const {
    return header_tokens_.size() - get_value<details::CsvOption::ignore_columns>().size();
  }

  Row header() const {
    return header_tokens_;
  }
};

} // namespace csv2