#pragma once
#include <chrono>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <utility>
#include <string>
#include <cstring>

namespace csv2 {

template <char delimiter, char quote_character> class Reader {
  char *map_;
  struct stat file_info_;
  int fd_;

public:
  Reader() : map_(nullptr), fd_(0) {}
  ~Reader() {
    // Free the mmapped memory
    munmap(map_, file_info_.st_size);
    // Un-mmaping doesn't close the file,
    // so we still need to do that.
    close(fd_);
  }

  bool read(const std::string &filename) {
    fd_ = open(filename.c_str(), O_RDONLY, (mode_t)0600);

    if ((fd_ == -1) || (fstat(fd_, &file_info_) == -1) || (file_info_.st_size == 0)) {
      return false;
    }

    map_ = (char *)mmap(0, file_info_.st_size, PROT_READ, MAP_SHARED, fd_, 0);
    if (map_ == MAP_FAILED) {
      close(fd_);
      return false;
    }
    return true;
  }

  class row_iterator;
  class row;
  class cell_iterator;

  class cell {
    char *buffer_;
    size_t start_;
    size_t end_;
    bool escaped_;
    friend class row;
    friend class cell_iterator;

  public:
    std::string value() {
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

  class row {
    char *buffer_;
    size_t start_;
    size_t end_;
    friend class row_iterator;

  public:
    class cell_iterator {
      friend class row;
      char *buffer_;
      size_t buffer_size_;
      size_t start_;
      size_t current_;
      size_t end_;

    public:
      cell_iterator(char *buffer, size_t buffer_size, size_t start, size_t end)
          : buffer_(buffer), buffer_size_(buffer_size), start_(start), current_(start_), end_(end) {
      }

      cell_iterator &operator++() {
        current_ += 1;
        return *this;
      }

      cell operator*() {
        bool escaped{false};
        class cell cell;
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

      bool operator!=(const cell_iterator &rhs) { return current_ != rhs.current_; }
    };

    cell_iterator begin() { return cell_iterator(buffer_, end_ - start_, start_, end_); }

    cell_iterator end() { return cell_iterator(buffer_, end_ - start_, end_, end_); }
  };

  class row_iterator {
    friend class Reader;
    char *buffer_;
    size_t buffer_size_;
    size_t start_;
    size_t end_;

  public:
    row_iterator(char *buffer, size_t buffer_size, size_t start)
        : buffer_(buffer), buffer_size_(buffer_size), start_(start), end_(start_) {}

    row_iterator &operator++() {
      start_ = end_ + 1;
      end_ = start_;
      return *this;
    }

    row operator*() {
      row result;
      result.buffer_ = buffer_;
      result.start_ = start_;
      result.end_ = end_;

      if (char *ptr = (char *)memchr(&buffer_[start_], '\n', (buffer_size_ - start_))) {
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

    bool operator!=(const row_iterator &rhs) { 
      return start_ != rhs.start_;  
    }
  };

  row_iterator begin() const { 
    if (file_info_.st_size == 0) return end();
    return row_iterator(map_, file_info_.st_size, 0); }

  row_iterator end() const { return row_iterator(map_, file_info_.st_size, file_info_.st_size + 1); }
};
} // namespace csv2