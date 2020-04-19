#include <chrono>
#include <csv2/reader.hpp>
#include <iostream>
using namespace csv2;

int main(int argc, char **argv) {

  using timepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  auto print_exec_time = [](timepoint start, timepoint stop) {
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    std::cout << duration_us.count() << " us | " << duration_ms.count() << " ms | "
              << duration_s.count() << " s\n";
  };

  auto start = std::chrono::high_resolution_clock::now();

  Reader csv{
      option::Filename{std::string(argv[1])}, option::Delimiter{','},
      option::SkipInitialSpace{true}
      // ...
  };

  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Stats:\n";
  std::cout << "Rows: " << csv.rows() << "\n";
  std::cout << "Cols: " << csv.cols() << "\n";
  print_exec_time(start, stop);
}
