#include "field_traits.h"

#include <charconv>
#include <cstdint>
#include <string>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}

bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    if (text.empty()) {
        return false;
    }

    uint64_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return false;
    }

    *out = value;
    return true;
}

bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    const std::string lower = ToLowerStr(text);

    if (lower == "1" || lower == "true" || lower == "yes") {
        *out = true;
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no") {
        *out = false;
        return true;
    }
    return false;
}

}