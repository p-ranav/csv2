#include <chrono>
#include <csv2/reader.hpp>
#include <iostream>
using namespace csv2;

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: ./fieldcount <column_index> <csv_file>\n";
    return EXIT_FAILURE;
  }

  size_t column_index = std::stoi(argv[1]);
  std::string cell_value{""};

  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  if (csv.mmap(argv[2])) {
    size_t sum{0};
    for (const auto row : csv) {
      size_t col{0};
      for (const auto cell : row) {
        col += 1;
        if (col == column_index) {
          cell.read_raw_value(cell_value);
          sum += std::stoi(cell_value);
          cell_value.clear();
        }
      }
    }
    std::cout << sum << "\n";
  } else {
    std::cout << "0\n";
  }
}
