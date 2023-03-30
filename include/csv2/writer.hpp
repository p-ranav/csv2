
#pragma once
#include <cstring>
#include <csv2/parameters.hpp>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <iostream>

namespace csv2 {

    template <typename, typename T> struct has_close : std::false_type {};

    template <typename C, typename Ret, typename... Args> struct has_close<C, Ret(Args...)> {
    private:
        template <typename T>
        static constexpr auto check(T *) ->
            typename std::is_same<decltype(std::declval<T>().close(std::declval<Args>()...)), Ret>::type;

        template <typename> static constexpr std::false_type check(...);

        typedef decltype(check<C>(0)) type;

    public:
        static constexpr bool value = type::value;
    };

template <class delimiter = delimiter<','>, typename Stream=std::ofstream>
class Writer {
    Stream& stream_;    // output stream for the writer
public:
    Writer(Stream& stream) : stream_(stream) {}

    ~Writer() {
        if constexpr (has_close<Stream, void()>::value) {
            // has `close`
            stream_.close();
        }
    }

    template <typename Container>
    void write_row(Container&& row) {
        const auto& strings = std::forward<Container>(row);
        const auto delimiter_string = std::string(1, delimiter::value);
        std::copy(strings.begin(), strings.end() - 1,
            std::ostream_iterator<std::string>(stream_, delimiter_string.c_str()));
        stream_ << strings.back() << "\n";
    }

    template <typename Container>
    void write_rows(Container&& rows) {
        const auto& container_of_rows = std::forward<Container>(rows);
        for (const auto& row : container_of_rows) {
            write_row(row);
        }
    }
};

}
