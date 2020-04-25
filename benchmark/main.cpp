#include <chrono>
#include <csv2/reader.hpp>
#include <iostream>
using namespace csv2;

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cout << "Usage: ./main <csv_file>\n";
    return EXIT_FAILURE;
  }

  using timepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  auto print_exec_time = [](timepoint start, timepoint stop) {
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    std::cout << duration_us.count() << " us | " << duration_ms.count() << " ms | "
              << duration_s.count() << " s\n";
  };

  // Measurement 1: Loading file
  auto start = std::chrono::high_resolution_clock::now();

  Reader<delimiter<','>, quote_character<'"'>, first_row_is_header<false>> csv;
  if (csv.mmap(argv[1])) {
    size_t rows{0}, cells{0};
    for (const auto row : csv) {
      rows += 1;
      for (const auto cell : row) {
        cells += 1;
      }
    }
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Stats:\n";
    std::cout << "Rows: " << rows << "\n";
    std::cout << "Cells: " << cells << "\n";
    std::cout << "Execution Time: ";
    print_exec_time(start, stop);
  } else {
    std::cout << "error: Failed to open " << argv[1] << std::endl;
  }
}