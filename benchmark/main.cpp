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

  // Measurement 1: Loading file
  auto m1_start = std::chrono::high_resolution_clock::now();

  Reader csv{
      option::Filename{std::string(argv[1])}, option::Delimiter{','},
      option::SkipInitialSpace{true}
      // ...
  };

  auto m1_stop = std::chrono::high_resolution_clock::now();
  auto m2_start = m1_stop;

  std::vector<Row> rows;
  Row next;
  while(csv.read_row(next)) {
    rows.push_back(std::move(next));
  }
  auto m2_stop = std::chrono::high_resolution_clock::now();

  std::cout << "Stats:\n";
  std::cout << "Rows: " << csv.rows() << "\n";
  std::cout << "Cols: " << csv.cols() << "\n";
  std::cout << "Measurement 1: ";
  print_exec_time(m1_start, m1_stop);
  std::cout << "Measurement 2: ";
  print_exec_time(m2_start, m2_stop);
  std::cout << "Total Execution Time: ";
  print_exec_time(m1_start, m2_stop);
}
