#include "parse.h"

namespace nano_edr {

bool IsBlankOrComment(const std::string& line) {
    for (char c : line) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        }
        return c == '#' || c == ';';
    }
    return true;
}

bool ParseEventParts(const std::string& line, EventParts* out) {
    if (IsBlankOrComment(line)) {
        return false;
    }

    std::size_t pos = 0;
    std::size_t len = line.size();

    while (pos < len) {
        while (pos < len && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\n' || line[pos] == '\r')) {
            ++pos;
        }
        if (pos == len) break;


        std::size_t key_start = pos;
        while (pos < len && line[pos] != '=') {
            if (line[pos] == ' ') return false;
            ++pos;
        }
        if (pos == len) return false;

        std::string key = line.substr(key_start, pos - key_start);
        if (key.empty()) return false;
        ++pos;

        std::string value;
        if (pos < len && line[pos] == '"') {
            ++pos;
            std::size_t value_start = pos;
            while (pos < len && line[pos] != '"') {
                ++pos;
            }
            if (pos == len) return false; 
            value = line.substr(value_start, pos - value_start);
            ++pos;
        } else {
            std::size_t value_start = pos;
            while (pos < len && line[pos] != ' ') {
                ++pos;
            }
            value = line.substr(value_start, pos - value_start);
        }

        if (key == "ts") {
            if (out->ts.empty()) {
                out->ts = value;
            } else {
                out->fields.push_back(Field{key, value});
            }
        } else if (key == "type") {
            if (out->type.empty()) {
                out->type = value;
            } else {
                out->fields.push_back(Field{key, value});
            }
        } else if (key == "pid") {
            if (out->pid.empty()) {
                out->pid = value;
            } else {
                out->fields.push_back(Field{key, value});
            }
        } else {
            out->fields.push_back(Field{key, value});
        }
    }

    return true;
}

}