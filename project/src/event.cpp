#include "event.h"

#include <charconv>
#include <sstream>
#include <stdexcept>


namespace nano_edr {

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") return EventType::kProcessStart;
    if (type == "process_end")   return EventType::kProcessEnd;
    if (type == "file_create")   return EventType::kFileCreate;
    if (type == "file_write")    return EventType::kFileWrite;
    if (type == "file_move")     return EventType::kFileMove;
    if (type == "net_connect")   return EventType::kNetConnect;
    if (type == "file_delete") return EventType::kFileDelete;   
    return EventType::kOther;
}

Event::Event(const EventParts& parts) {
    if (parts.ts.empty()) {
        throw std::invalid_argument("пустое время события");
    }
    if (parts.type.empty()) {
        throw std::invalid_argument("пустой тип события");
    }

    uint64_t value = 0;
    auto [ptr, ec] = std::from_chars(parts.ts.data(), parts.ts.data() + parts.ts.size(), value);
    if (ec != std::errc{} || ptr != parts.ts.data() + parts.ts.size()) {
        throw std::invalid_argument("неверное время события: " + parts.ts);
    }
    ts_ = Timestamp{value};
    raw_ts_ = parts.ts;
    event_type_ = EventTypeFromString(parts.type);
    type_ = parts.type;
    pid_ = parts.pid;
    fields_ = parts.fields;
}

std::string ToString(const Event& event) {
    std::string result = "";
    result += "ts=" + event.raw_ts();
    result += " type=" + event.type();
    result += " pid=" + event.pid();
    for (std::size_t i = 0; i < event.fields().size(); i++) {
        result += " " + event.fields()[i].key + "=" + event.fields()[i].value;
    }
    return result;
}
    
std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}
}