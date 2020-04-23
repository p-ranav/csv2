#pragma once
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace csv2 {

template <char delimiter = ',', char quote_character = '"'> class Reader {
  int fd_;                // file descriptor
  struct stat file_info_; // file info
  char *map_;             // memory-mapped buffer
  bool file_opened_;      // if true, cleanup map in next .read() call

public:
  Reader() : fd_(-1), file_info_{}, map_(nullptr), file_opened_(false) {}
  ~Reader() {
    // Free the mmapped memory
    munmap(map_, file_info_.st_size);
    // Un-mmaping doesn't close the file,
    // so we still need to do that.
    close(fd_);
  }

  bool read(const std::string &filename) {
    if (file_opened_) {
      munmap(map_, file_info_.st_size);
      close(fd_);
    }
    fd_ = open(filename.c_str(), O_RDONLY, static_cast<mode_t>(0600));

    if ((fd_ == -1) || (fstat(fd_, &file_info_) == -1) || (file_info_.st_size == 0)) {
      return false;
    }

    map_ = static_cast<char *>(mmap(nullptr, file_info_.st_size, PROT_READ, MAP_SHARED, fd_, 0));
    if (map_ == MAP_FAILED) {
      close(fd_);
      return false;
    }
    file_opened_ = true;
    return true;
  }

  class RowIterator;
  class Row;
  class CellIterator;

  class Cell {
    char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};       // Start index of cell content
    size_t end_{0};         // End index of cell content
    bool escaped_{false};   // Does the cell have escaped content?
    friend class Row;
    friend class CellIterator;

  public:
    // Returns the raw_value of the cell without handling escaped
    // content, e.g., cell containing """foo""" will be returned
    // as is
    std::string raw_value() const {
      std::string result;
      if (start_ >= end_)
        return "";
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
      return result;
    }

    // If cell is escaped, convert and return correct cell contents,
    // e.g., """foo""" => ""foo""
    std::string value() const {
      std::string result;
      if (start_ >= end_)
        return "";
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
      for (size_t i = 1; i < result.size(); ++i) {
        if (result[i] == quote_character && result[i - 1] == quote_character) {
          result.erase(i - 1, 1);
        }
      }
      return result;
    }
  };

  class Row {
    char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};       // Start index of row content
    size_t end_{0};         // End index of row content
    friend class RowIterator;

  public:
    // Returns the raw_value of the row
    std::string raw_value() const {
      std::string result;
      if (start_ >= end_)
        return "";
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
      return result;
    }

    class CellIterator {
      friend class Row;
      char *buffer_;
      size_t buffer_size_;
      size_t start_;
      size_t current_;
      size_t end_;

    public:
      CellIterator(char *buffer, size_t buffer_size, size_t start, size_t end)
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
          if (buffer_[i] == delimiter && !quote_opened) {
            // actual delimiter
            // end of cell
            current_ = i;
            cell.end_ = current_;
            cell.escaped_ = escaped;
            return cell;
          } else {
            if (buffer_[i] == quote_character) {
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
                  if (i + 1 < end_ && buffer_[i + 1] == delimiter) {
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
    char *buffer_;
    size_t buffer_size_;
    size_t start_;
    size_t end_;

  public:
    RowIterator(char *buffer, size_t buffer_size, size_t start)
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

      if (char *ptr =
              static_cast<char *>(memchr(&buffer_[start_], '\n', (buffer_size_ - start_)))) {
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
    if (file_info_.st_size == 0)
      return end();
    return RowIterator(map_, file_info_.st_size, 0);
  }

  RowIterator end() const { return RowIterator(map_, file_info_.st_size, file_info_.st_size + 1); }

  Row header() const {
    for (const auto row : *this)
      return row; // just return the first row
    return Row();
  }

  size_t rows() const {
    size_t result{0};
    if (!map_ || file_info_.st_size == 0) return result;
    for(char *p = map_; (p = (char*) memchr(p, '\n', (map_ + file_info_.st_size) - p)); ++p)
      ++result;
    return result;
  }

  size_t cols() const {
    size_t result{0};
    for (const auto cell: header())
      result += 1;
    return result;
  }

};
} // namespace csv2
