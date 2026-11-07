#include "conditions.h"
#include "text.h"
#include "fields.h"
#include "text.h"

#include <string>
#include <vector>

namespace nano_edr{

bool FieldEquals::Matches(const Event& event) const {
    const std::string* v = FindField(event, key_);
    if (v == nullptr) {
        return false;
    }
    return ToLowerStr(*v) == ToLowerStr(value_);
}


bool FieldContains::Matches(const Event& event) const {
    const std::string* v = FindField(event, key_);
    if (v == nullptr){
        return false;
    }
    return ToLowerStr(*v).find(ToLowerStr(needle_)) != std::string::npos;
}

bool FieldEndsWith::Matches(const Event& event) const {
    const std::string* v = FindField(event, key_);
    if (v == nullptr){
        return false;
    }
    std::string lower_v = ToLowerStr(*v);
    std::string lower_suffix = ToLowerStr(suffix_);
    if (lower_v.size() < lower_suffix.size()) {
        return false;
    }
    std::string end = lower_v.substr(lower_v.size() - lower_suffix.size());

    return end == lower_suffix;
}

bool EventTypeIs::Matches(const Event& event) const {
    const EventType type = event.event_type();
    for (const EventType t : types_) {
        if (t == type) {
            return true;
        }
    }
    return false;
}

bool FieldInList::Matches(const Event& event) const {
    const std::string* v = FindField(event, key_);
    if (v == nullptr) {
        return false;
    }
    std::string lower_v = ToLowerStr(*v);
    for (const std::string& s : values_) {
        if (lower_v == ToLowerStr(s)) {
            return true;
        }
    }
    return false;
}

void AllOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

void AnyOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AllOf::Matches(const Event& event) const {
    for (const auto& part : parts_){
        if (!part->Matches(event)){
            return false;
        }
    }
    return true;
}

bool AnyOf::Matches(const Event& event) const {
    for (const auto& part : parts_){
        if (part->Matches(event)){
            return true;
        }
    }
    return false;
}

}