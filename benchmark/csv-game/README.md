[`csv-game` Benchmark](https://bitbucket.org/ewanhiggs/csv-game/src/master/)

## Procedure
1. Generate the test file using the script in the test directory.
2. For `fieldcount`, run:

```cpp
g++ -O3 -I../../include -std=c++11 -o fieldcount fieldcount.cpp
hyperfine --min-runs 5 --warmup 3 './fieldcount /tmp/hello.csv'
```

3. For `csv_count` (or csv_summer), run:

```cpp
g++ -O3 -I../../include -std=c++11 -o csv_count csv_count.cpp
hyperfine --min-runs 5 --warmup 3 './csv_count /tmp/count.csv'
```

## The Tests
There are two tests.

`fieldcount`: Count the number of fields in the file. This exercises the CSV processing library by forcing it to parse all the fields. There is a separate run called empty which runs against an empty file and it is used as an attempt to tease out the performance of the actual CSV parsing from the startup for the runtime (importing modules, loading libraries, instantiating structures, etc).

`csv_count`: Take the sum of one of the columns in the file. This exercises the CSV parsing library, string to integer parsing, and basic maths. I saw textql which slurps data into sqlite and runs queries on the resulting database. I thought it's a cool idea, but could it possibly be performant? This test would probably be better named as csv-summer

## Performance Benchmark

### Hardware 

```
MacBook Pro (15-inch, 2019)
Processor: 2.4 GHz 8-Core Intel Core i9
Memory: 32 GB 2400 MHz DDR4
Operating System: macOS Catalina version 10.15.3
```

| Test | Time |
| --- | --- |
| fieldcount | 36.2 ms ± 3.1 ms |
| csv_count | 92.5 ms ± 1.8 ms |