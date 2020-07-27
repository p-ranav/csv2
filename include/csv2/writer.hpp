
#pragma once
#include <cstring>
#include <csv2/parameters.hpp>
#include <fstream>
#include <string>
#include <utility>
#include <iostream>

namespace csv2 {

template <class delimiter = delimiter<','>>
class Writer {
    std::ofstream& stream_;    // output stream for the writer
public:
    template <typename Stream>
    Writer(Stream&& stream) : stream_(std::forward<Stream>(stream)) {}

    ~Writer() {
        stream_.close();
    }

    template <typename Container>
    void write_row(Container&& row) {
        const auto& strings = std::forward<Container>(row);
        const auto delimiter_string = std::string(1, delimiter::value);
        std::copy(strings.begin(), strings.end() - 1,
            std::ostream_iterator<std::string>(stream_, delimiter_string.c_str()));
        stream_ << strings.back() << "\n";
    }

    template <typename Container>
    void write_rows(Container&& rows) {
        const auto& container_of_rows = std::forward<Container>(rows);
        for (const auto& row : container_of_rows) {
            write_row(row);
        }
    }
};

}