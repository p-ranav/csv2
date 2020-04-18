#include <string>
#include <algorithm>
#include <cctype>
#include <locale>

// trim from start (in place)
static inline void ltrim(std::string &s, const std::vector<char>& t) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&t](int ch) {
        return std::find(t.begin(), t.end(), ch) == t.end();
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s, const std::vector<char>& t) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [&t](int ch) {
        return std::find(t.begin(), t.end(), ch) == t.end();
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s, const std::vector<char>& t) {
    ltrim(s, t);
    rtrim(s, t);
}