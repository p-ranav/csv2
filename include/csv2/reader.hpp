#pragma once
#include <cstring>
#include <csv2/mio.hpp>
#include <istream>
#include <string>

namespace csv2 {

template <char character> struct delimiter {
  constexpr static char value() { return character; }
};

template <char character> struct quote_character {
  constexpr static char value() { return character; }
};

namespace trim_policy {
struct no_trimming {
public:
  static std::pair<size_t, size_t> trim(const char *buffer, size_t start, size_t end) {
    (void)(buffer); // to silence unused parameter warning
    return {start, end};
  }
};

template <char... character_list> struct trim_characters {
private:
  constexpr static bool is_trim_char(char) { return false; }

  template <class... Tail> constexpr static bool is_trim_char(char c, char head, Tail... tail) {
    return c == head || is_trim_char(c, tail...);
  }

public:
  static std::pair<size_t, size_t> trim(const char *buffer, size_t start, size_t end) {
    size_t new_start = start, new_end = end;
    while (new_start != new_end && is_trim_char(buffer[new_start], character_list...))
      ++new_start;
    while (new_start != new_end && is_trim_char(buffer[new_end - 1], character_list...))
      --new_end;
    return {new_start, new_end};
  }
};

using trim_whitespace = trim_characters<' ', '\t'>;
} // namespace trim_policy

template <class delimiter = delimiter<','>, class quote_character = quote_character<'"'>,
          class trim_policy = trim_policy::trim_whitespace>
class Reader {
  mio::mmap_source mmap_; // mmap source
  const char *buffer_;    // pointer to memory-mapped data
  size_t buffer_size_;    // mapped length of buffer

public:
  // Use this if you'd like to mmap the CSV file
  bool mmap(const std::string &filename) {
    mmap_ = mio::mmap_source(filename);
    if (!mmap_.is_open() || !mmap_.is_mapped())
      return false;
    buffer_ = mmap_.data();
    buffer_size_ = mmap_.mapped_length();
    return true;
  }

  // Use this if you have the CSV contents
  // in an std::string already
  template <typename StringType> bool parse(StringType &&contents) {
    buffer_ = std::forward<StringType>(contents).c_str();
    buffer_size_ = contents.size();
    return buffer_size_ > 0;
  }

  class RowIterator;
  class Row;
  class CellIterator;

  class Cell {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of cell content
    size_t end_{0};               // End index of cell content
    bool escaped_{false};         // Does the cell have escaped content?
    friend class Row;
    friend class CellIterator;

  public:
    // Returns the raw_value of the cell without handling escaped
    // content, e.g., cell containing """foo""" will be returned
    // as is
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
    }

    // If cell is escaped, convert and return correct cell contents,
    // e.g., """foo""" => ""foo""
    template <typename Container> void read_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      const auto new_start_end = trim_policy::trim(buffer_, start_, end_);
      for (size_t i = new_start_end.first; i < new_start_end.second; ++i)
        result.push_back(buffer_[i]);
      for (size_t i = 1; i < result.size(); ++i) {
        if (result[i] == quote_character::value() && result[i - 1] == quote_character::value()) {
          result.erase(i - 1, 1);
        }
      }
    }
  };

  class Row {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of row content
    size_t end_{0};               // End index of row content
    friend class RowIterator;

  public:
    // Returns the raw_value of the row
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
    }

    class CellIterator {
      friend class Row;
      const char *buffer_;
      size_t buffer_size_;
      size_t start_;
      size_t current_;
      size_t end_;

    public:
      CellIterator(const char *buffer, size_t buffer_size, size_t start, size_t end)
          : buffer_(buffer), buffer_size_(buffer_size), start_(start), current_(start_), end_(end) {
      }

      CellIterator &operator++() {
        current_ += 1;
        return *this;
      }

      Cell operator*() {
        bool escaped{false};
        class Cell cell;
        cell.buffer_ = buffer_;
        cell.start_ = current_;
        cell.end_ = end_;

        size_t last_quote_location = 0;
        bool quote_opened = false;
        for (auto i = current_; i < end_; i++) {
          if (buffer_[i] == delimiter::value() && !quote_opened) {
            // actual delimiter
            // end of cell
            current_ = i;
            cell.end_ = current_;
            cell.escaped_ = escaped;
            return cell;
          } else {
            if (buffer_[i] == quote_character::value()) {
              if (!quote_opened) {
                // first quote for this cell
                quote_opened = true;
                last_quote_location = i;
              } else {
                // quote previously opened for this cell
                // check last quote location
                if (last_quote_location == i - 1) {
                  // previous character was quote too!
                  escaped = true;
                } else {
                  last_quote_location = i;
                  if (i + 1 < end_ && buffer_[i + 1] == delimiter::value()) {
                    quote_opened = false;
                  }
                }
              }
              current_ = i;
            } else {
              // Not delimiter or quote
              current_ = i;
            }
          }
        }
        cell.end_ = current_ + 1;
        return cell;
      }

      bool operator!=(const CellIterator &rhs) { return current_ != rhs.current_; }
    };

    CellIterator begin() const { return CellIterator(buffer_, end_ - start_, start_, end_); }
    CellIterator end() const { return CellIterator(buffer_, end_ - start_, end_, end_); }
  };

  class RowIterator {
    friend class Reader;
    const char *buffer_;
    size_t buffer_size_;
    size_t start_;
    size_t end_;

  public:
    RowIterator(const char *buffer, size_t buffer_size, size_t start)
        : buffer_(buffer), buffer_size_(buffer_size), start_(start), end_(start_) {}

    RowIterator &operator++() {
      start_ = end_ + 1;
      end_ = start_;
      return *this;
    }

    Row operator*() {
      Row result;
      result.buffer_ = buffer_;
      result.start_ = start_;
      result.end_ = end_;

      if (const char *ptr =
              static_cast<const char *>(memchr(&buffer_[start_], '\n', (buffer_size_ - start_)))) {
        end_ = start_ + (ptr - &buffer_[start_]);
        result.end_ = end_;
        if (end_ + 1 < buffer_size_)
          start_ = end_ + 1;
      } else {
        // last row
        end_ = buffer_size_;
        result.end_ = end_;
      }
      return result;
    }

    bool operator!=(const RowIterator &rhs) { return start_ != rhs.start_; }
  };

  RowIterator begin() const {
    if (buffer_size_ == 0)
      return end();
    return RowIterator(buffer_, buffer_size_, 0);
  }

  RowIterator end() const { return RowIterator(buffer_, buffer_size_, buffer_size_ + 1); }

  Row header() const {
    for (const auto row : *this)
      return row; // just return the first row
    return Row();
  }

  size_t rows() const {
    size_t result{0};
    if (!buffer_ || buffer_size_ == 0)
      return result;
    for (char *p = buffer_; (p = (char *)memchr(p, '\n', (buffer_ + buffer_size_) - p)); ++p)
      ++result;
    return result;
  }

  size_t cols() const {
    size_t result{0};
    for (const auto cell : header())
      result += 1;
    return result;
  }
};
} // namespace csv2
