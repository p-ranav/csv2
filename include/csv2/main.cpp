#include <csv2/reader.hpp>

int main(int argc, char **argv) {
  csv2::reader csv(std::string{argv[1]}, ' ');
  csv2::row_t row;
  while(csv.read_row(row)) {
    // for (auto& [k, v]: row)
    //     std::cout << k << ":" << v << " ";
    // std::cout << "\n";
  }
  // std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
  // auto header = csv.header();
  // for (auto& h: header)
  //   std::cout << h << " ";
}