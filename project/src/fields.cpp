#include "fields.h"
#include "text.h"
#include <cctype>
#include <stdexcept> 
#include <cstdint>
#include <string>
#include <charconv>

namespace nano_edr {

const std::string* FindField(const Event& event, const std::string& key) {
    if (key == "ts"){
        return &event.raw_ts();
    }
    if (key == "type"){
        return &event.type();
    }
    if (key == "pid"){ 
        return &event.pid();
    }
    for (const auto& field : event.fields()) {
        if (field.key == key) {
            return &field.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const std::string* value = FindField(event, key);
    if (value == nullptr) {
        throw std::invalid_argument("Missing required field: " + key);
    }
    return *value;
}

bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const std::string* value = FindField(event, key);
    if (value == nullptr) {
        return false;
    }

    if (value->empty()) {
        return false;
    }

    auto [end, error] = std::from_chars(value->data(), value->data() + value->size(), *out);

    return error == std::errc{} && end == value->data() + value->size();
}

uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t number = 0;
    if (GetIntField(event, key, &number)) {
        return number;
    }
    return fallback;
}

bool IsProcessStart(const Event& event) {
    return event.type() == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type() == "file_write" || event.type() == "file_create" || event.type() == "file_move";
}

bool IsNetConnect(const Event& event) {
    return event.type() == "net_connect";
}   

std::string NormalizePath(const std::string& path) {
    std::string res = path;
    for (char& c : res) {
        if (c == '/'){
             c = '\\';
        } else{
            c = ToLower(c);
        }
    }

    std::size_t pos = res.find("%temp%");
    while (pos != std::string::npos) {
        res.replace(pos, 6, "\\appdata\\local\\temp\\");
        pos = res.find("%temp%", pos + 1);
    }

    pos = res.find("%tmp%");
    while (pos != std::string::npos) {
        res.replace(pos, 5, "\\appdata\\local\\temp\\");
        pos = res.find("%tmp%", pos + 1);
    }

    std::string out;
    for (char c : res) {
        if (c == '\\' && !out.empty() && out.back() == '\\') {
            continue;
        }
        out += c;
    }
    return out;
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const std::string* cmd_ptr = FindField(event, "cmdline");
    if (!cmd_ptr) {
        return false;
    }
    std::string norm_cmd = NormalizePath(*cmd_ptr);
    std::string norm_needle = NormalizePath(needle);

    return norm_cmd.find(norm_needle) != std::string::npos;
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const std::string* path = FindField(event, "path");
    if (!path) path = FindField(event, "to");
    if (!path) return false;
    return EndsWithIgnoreCase(*path, suffix);
}

}