#include "rules.h"
#include <iostream>

namespace nano_edr {

const char* SeverityName(Severity severity) {
    if (severity == Severity::kLow) {
        return "low";
    }
    if (severity == Severity::kMedium) {
        return "medium";
    }
    if (severity == Severity::kHigh) {
        return "high";
    }
    if (severity == Severity::kCritical) {
        return "critical";
    }
    
    return "?";
}

}