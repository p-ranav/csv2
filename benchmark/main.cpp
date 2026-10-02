#include <chrono>
#include <csv2/reader.hpp>
#include <iostream>
#include <string>
using namespace csv2;

using timepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

static void print_exec_time(timepoint start, timepoint stop) {
  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

  std::cout << duration_us.count() << " us | " << duration_ms.count() << " ms | "
            << duration_s.count() << " s\n";
}

template <char Delim> static void run(const char *filename) {
  auto start = std::chrono::high_resolution_clock::now();

  Reader<delimiter<Delim>, quote_character<'"'>, first_row_is_header<false>> csv;
  if (csv.mmap(filename)) {
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
    std::cout << "error: Failed to open " << filename << std::endl;
  }
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: ./main <csv_file> [delimiter]\n";
    std::cout << "  delimiter: ',' ';' '|' or 'tab' (default: ',')\n";
    return EXIT_FAILURE;
  }

  char delim = ',';
  if (argc == 3) {
    const std::string arg = argv[2];
    if (arg == "tab" || arg == "\\t") {
      delim = '\t';
    } else if (arg.size() == 1) {
      delim = arg[0];
    } else {
      std::cout << "error: delimiter must be a single character (or 'tab')\n";
      return EXIT_FAILURE;
    }
  }

  // Templates need a compile-time delimiter, so dispatch over the handful of
  // delimiters this benchmark supports.
  switch (delim) {
  case ',':
    run<','>(argv[1]);
    break;
  case ';':
    run<';'>(argv[1]);
    break;
  case '|':
    run<'|'>(argv[1]);
    break;
  case '\t':
    run<'\t'>(argv[1]);
    break;
  default:
    std::cout << "error: unsupported delimiter '" << delim << "'. Supported: ',' ';' '|' tab\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
