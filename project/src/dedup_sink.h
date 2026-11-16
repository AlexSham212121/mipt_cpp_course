#ifndef NANO_EDR_DEDUP_SINK_H
#define NANO_EDR_DEDUP_SINK_H

#include <cstddef>
#include <cstdint>

#include "alert_sink.h"
#include "detection.h"

namespace nano_edr {

class DedupSink {
 public:
    DedupSink(AlertSink inner, uint64_t window_ms, std::size_t* suppressed);

    void operator()(const Detection& detection);

  private:
    struct Record {
        std::string rule_id;
        std::string pid;
        uint64_t t0 = 0;
    };

    AlertSink next_;
    uint64_t window_ = 0;
    std::size_t* dropped_ = nullptr;
    std::vector<Record> seen_;
};

}

#endif