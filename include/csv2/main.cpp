#include <csv2/reader.hpp>
using namespace csv2;

int main(int argc, char **argv) {
  reader csv{
    option::Filename{std::string(argv[1])},
    option::Delimiter{' '},
    option::TrimCharacters{'\n', '\r'}
  };
  row_t row;
  while(csv.read_row(row)) {
    // for (auto& [k, v]: row)
    //     std::cout << k << ":" << v << "\n";
    // std::cout << "\n";
  }
  std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
}
