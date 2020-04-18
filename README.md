# csv2
`csv2` is a fast and lazy CSV Parser written in `C++17`

```cpp
#include <csv2/reader.hpp>
using namespace csv2;

int main() {
  // Construct a csv reader
  reader csv{
    option::Filename{std::string("foo.csv")},
    option::Delimiter{','},
    option::TrimCharacters{std::vector<char>{'\n', '\r'}},
    option::TrimPolicy{Trim::trailing}
  };
  
  // At this point, all the data is already loaded.
  std::cout << "Loaded " << csv.rows() << "x" << csv.cols() << " entries\n";
  
  // Iterate over each row like below:
  row next;
  while (csv.read_row(next)) {
    // Do something with row
  }
}
```

## Highlights
* Single header file - Just include `csv2/reader.hpp`
* Blazing fast - Fast file reader coupled with generous use of `std::string_view` to minimize allocations/copies.
* Single-threaded - No additional worker threads.
* Lazy evaluated - Tokenization is not performed until `read_row()` is called.
* Configurable - Set your dialect with `csv2::option`.
* MIT License.
