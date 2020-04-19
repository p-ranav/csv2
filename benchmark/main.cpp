#include <csv2/reader.hpp>
#include <iostream>
#include <chrono>
using namespace csv2;

int main(int argc, char **argv) {

  auto start = std::chrono::high_resolution_clock::now();
  
  Reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{';'},
    option::SkipInitialSpace{true},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing}
    // ...
  };
  
  Row next;
  while (csv.read_row(next)) {
    // Iterate over all rows in CSV
  }

  // Get ending timepoint 
  auto stop = std::chrono::high_resolution_clock::now();

  std::cout << "Stats:\n";
  std::cout << "Rows: " << csv.rows() << "\n";
  std::cout << "Cols: " << csv.cols() << "\n";
  
  // Get execution time
  auto duration_us =
    std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  auto duration_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(stop - start); 
  auto duration_s =
    std::chrono::duration_cast<std::chrono::seconds>(stop - start);   
  
  std::cout << "Microseconds: " << duration_us.count() << " us\n";
  std::cout << "Milliseconds: " << duration_ms.count() << " ms\n";
  std::cout << "Seconds: " << duration_s.count() << " s\n";
  std::cout << "Rows per second: " << (csv.rows() / (float)duration_s.count()) << "\n";
}
