<p align="center">
  <img height="75" src="img/logo.png" alt="csv2"/>
</p>

```cpp
#include <csv2/reader.hpp>

int main() {
  csv2::Reader<delimiter<','>, quote_character<'"'>, trim_policy::trim_whitespace> reader;
  if (reader.mmap("foo.csv")) {
    for (const auto row: reader) {
      for (const auto cell: row) {
        // Do something with cell value
        // std::string value;
        // cell.read_value(value);
      }
    }
  }
}
```

## Performance Benchmark

This benchmark measures how long it takes on average for `csv2` to memory-map the input CSV file and iterate over every cell in the CSV. See `benchmark/main.cpp` for more details. Assume that the cache has been warmed up.

```bash
cd benchmark
g++ -I../include -O3 -std=c++11 -o main main.cpp
./main <csv_file>
```

### Hardware 

```
MacBook Pro (15-inch, 2019)
Processor: 2.4 GHz 8-Core Intel Core i9
Memory: 32 GB 2400 MHz DDR4
Operating System: macOS Catalina version 10.15.3
```

### Results (as of 23 APR 2020)

| Dataset | File Size | Rows | Cols | Time |
|:---     |       ---:|  ---:|  ---:|  ---:|
| [Denver Crime Data](https://www.kaggle.com/paultimothymooney/denver-crime-data) | 111 MB | 479,100 | 19 | 0.198s |
| [AirBnb Paris Listings](https://www.kaggle.com/juliatb/airbnb-paris) | 196 MB | 141,730 | 96 | 0.345s |
| [2015 Flight Delays and Cancellations](https://www.kaggle.com/usdot/flight-delays) | 574 MB | 5,819,079 | 31 | 0.994s |
| [StackLite: Stack Overflow questions](https://www.kaggle.com/stackoverflow/stacklite) | 870 MB | 17,203,824 | 7 | 1.547s |
| [Used Cars Dataset](https://www.kaggle.com/austinreese/craigslist-carstrucks-data) | 1.4 GB | 539,768 | 25 | 2.381s |
| [Title-Based Semantic Subject Indexing](https://www.kaggle.com/hsrobo/titlebased-semantic-subject-indexing) | 3.7 GB | 12,834,026 | 4 | 6.965s|
| [Bitcoin tweets - 16M tweets](https://www.kaggle.com/alaix14/bitcoin-tweets-20160101-to-20190329) | 4 GB | 47,478,748 | 9 | 7.945s |
| [DDoS Balanced Dataset](https://www.kaggle.com/devendra416/ddos-datasets) | 6.3 GB | 12,794,627 | 85 | 13.578s |
| [Seattle Checkouts by Title](https://www.kaggle.com/city-of-seattle/seattle-checkouts-by-title) | 7.1 GB | 34,892,623 | 11 | 15.350s |
| [SHA-1 password hash dump](https://www.kaggle.com/urvishramaiya/have-i-been-pwnd) | 11 GB | 2,62,974,241 | 2 | 22.069s |
| [DOHUI NOH scaled_data](https://www.kaggle.com/seaa0612/scaled-data) | 16 GB | 496,782 | 3213 | 34.026s |

## API

Here is the public API available to you:

```cpp
template <class delimiter = delimiter<','>, 
          class quote_character = quote_character<'"'>,
          class trim_policy = trim_policy::trim_whitespace>
class Reader {
public:
  
  // Use this if you'd like to mmap the CSV file
  bool mmap(const std::string &filename);

  // Use this if you have the CSV contents in std::string already
  template <typename StringType>
  bool parse(StringType&& contents);

  // Shape
  size_t rows() const;
  size_t cols() const;
  
  // Row iterator
  RowIterator begin() const;
  RowIterator end() const;

  // Accessors
  Row header() const;
};
```

Here's the `Row` class:

```cpp
// Row class
class Row {
public:
  // Get raw contents of the row
  template <typename Container>
  void read_raw_value(Container& value) const;
  
  // Cell iterator
  CellIterator begin() const;
  CellIterator end() const;
};
```

and here's the `Cell` class:

```cpp
// Cell class
class Cell {
public:
  // Get raw contents of the cell
  template <typename Container>
  void read_raw_value(Container& value) const;
  
  // Get converted contents of the cell
  // Handles escaped content, e.g., 
  // """foo""" => ""foo""
  template <typename Container>
  void read_value() const;
};
```

## Compiling Tests

```bash
mkdir build && cd build
cmake -DCSV2_TEST=ON ..
make
cd test
./csv2_test
```

## Contributing
Contributions are welcome, have a look at the [CONTRIBUTING.md](CONTRIBUTING.md) document for more information.

## License
The project is available under the [MIT](https://opensource.org/licenses/MIT) license.
