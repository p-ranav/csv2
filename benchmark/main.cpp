#include <csv2/reader.hpp>
#include <iostream>
#include <chrono>
using namespace csv2;

int main(int argc, char **argv) {

  auto start = std::chrono::high_resolution_clock::now();
  
  Reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{','},
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
  
  // Get duration. Substart timepoints to  
  // get durarion. To cast it to proper unit 
  // use duration cast method 
  auto duration =
    std::chrono::duration_cast<std::chrono::microseconds>(stop - start); 
  
  std::cout << "Execution Time: " << duration.count() << " us\n"; 
  
}
