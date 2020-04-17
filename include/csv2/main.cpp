#include <csv2/reader.hpp>
// #include <csv2/backward.hpp>

int main(int argc, char **argv) {
  csv2::reader csv(std::string{argv[1]}, ',');
  csv2::row_t row;
  while(csv.read_row(row)) {

    // using namespace backward;
    // StackTrace st; st.load_here(32);
    // Printer p;
    // p.object = true;
    // p.color_mode = ColorMode::always;
    // p.address = true;
    // p.print(st, stderr);
    // for (auto& [k, v]: row)
    //     std::cout << k << ":" << v << " ";
    // std::cout << "\n";
  }
  // std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
  // auto header = csv.header();
  // for (auto& h: header)
  //   std::cout << h << " ";
}
