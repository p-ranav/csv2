<p align="center">
  <img height="75" src="img/logo.png" alt="csv2"/>
</p>

`csv2` is a fast, single-threaded CSV Parser written in `C++17`.

## Highlights
* Blazing fast - Fast line reader coupled with generous use of `std::string_view` to minimize allocations/copies.
* Single-threaded - No additional worker threads.
* Lazy evaluated - Tokenization is not performed until `read_row()` is called.
* MIT License.

```cpp
#include <csv2/reader.hpp>
using namespace csv2;

int main() {

  Reader csv{
    option::Filename{std::string("foo.csv")},
    option::Delimiter{','},
    option::SkipInitialSpace{true}
    // ...
  };
  
  Row next;
  while (csv.read_row(next)) {
    // Do something with `row`
    // for (auto& [key, value]: row) {
    //   std::cout << key << ": " << value << "\n";
    // }
  }
}
```

## Options

| Property | Data Type | Description |
|--------------------|-------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `option::Filename` | `std::string` | specifies the file to read. |
| `option::Delimiter` | `char` | specifies the character which should separate fields (aka columns). Default = `','` |
| `option::QuoteCharacter` | `char` | specifies the character to use as the quoting character. Default = `'"'` |
| `option::SkipInitialSpace` | `bool` | specifies how to interpret whitespace immediately following the delimiter; if `false`, it means that whitespace immediately after a delimiter should be treated as part of the following field. Default = `false` |
| `option::TrimCharacters` | `std::vector<char>` | specifies the list of characters to trim from every row in the CSV. Default = `{'\n', '\r'}` |
| `option::TrimPolicy` | `csv2::Trim` | specifies the type of trimming. Default = `Trim::trailing`. **NOTE:** Trimming only applies at the row-level; not for each field in the row. |
| `option::IgnoreColumns` | `std::vector<std::string>` | specifies the list of columns to ignore. Default = ```{}``` - no column ignored |
| `option::ColumnNames` | `std::vector<std::string>` | specifies the list of column names. This is useful when the first row of the CSV isn't a header Default = ```{}``` |
| `option::SkipEmptyRows` | `bool` | specifies how empty rows should be interpreted. If this is set to true, empty rows are skipped. Default = ```false``` |

## Compiling Tests

```bash
mkdir build && cd build
cmake -DCSV2_TEST=ON ..
make
cd test
./test
```
