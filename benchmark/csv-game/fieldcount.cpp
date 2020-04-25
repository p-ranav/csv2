#include <chrono>
#include <csv2/reader.hpp>
#include <iostream>
using namespace csv2;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: ./fieldcount <csv_file>\n";
    return EXIT_FAILURE;
  }

  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  if (csv.mmap(argv[1])) {
    size_t cells{0};
    for (const auto row : csv) {
      for (const auto cell : row) {
        cells += 1;
      }
    }
    std::cout << cells << "\n";
  } else {
    std::cout << "0\n";
  }
}
