#include "text.h"

namespace nano_edr {
char ToLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

std::string ToLowerStr(const std::string& s) {
    std::string res = s;
    for (char& c : res) {
        c = ToLower(c);
    }
    return res;
}

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    return ToLowerStr(a) == ToLowerStr(b);
}

bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return ToLowerStr(haystack).find(ToLowerStr(needle)) != std::string::npos;
}

bool EndsWithIgnoreCase(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) {
        return false;
    }
    return ToLowerStr(str.substr(str.size() - suffix.size())) == ToLowerStr(suffix);
}
}