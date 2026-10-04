
#pragma once
#include <cstring>
#include <csv2/parameters.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>

namespace csv2 {

template <class delimiter = delimiter<','>, typename Stream = std::ofstream> class Writer {
  Stream &stream_; // output stream for the writer
public:
  Writer(Stream &stream) : stream_(stream) {}

  ~Writer() {
    stream_.close();
  }

  template <typename Container> void write_row(Container &&row) {
    const auto &strings = std::forward<Container>(row);
    for (auto it = strings.begin(); it != strings.end() - 1; ++it) {
      write_field(*it);
      stream_ << delimiter::value;
    }
    write_field(strings.back());
    stream_ << "\n";
  }

  template <typename Container> void write_rows(Container &&rows) {
    const auto &container_of_rows = std::forward<Container>(rows);
    for (const auto &row : container_of_rows) {
      write_row(row);
    }
  }

private:
  void write_field(const std::string &field) {
    if (field.find(delimiter::value) == std::string::npos &&
        field.find_first_of("\"\r\n") == std::string::npos) {
      stream_ << field;
      return;
    }

    stream_ << '"';
    for (char character : field) {
      if (character == '"')
        stream_ << '"';
      stream_ << character;
    }
    stream_ << '"';
  }
};

} // namespace csv2
