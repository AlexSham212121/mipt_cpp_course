#include <print>
#include <stdexcept>
#include <string>
#include <utility>

#include "alert_sink.h"
#include "dedup_sink.h"
#include "detection.h"

namespace nano_edr {

// --- ConsoleSink -----------------------------------------------------------

void ConsoleSink::operator()(const Detection& detection) const {
    std::print("{}\n", ToString(detection));
}

// --- FileSink --------------------------------------------------------------

FileSink::FileSink(const std::string& path) : out_(path) {
    if (!out_.is_open()) {
        throw std::runtime_error("не удалось открыть файл: " + path);
    }
}

void FileSink::operator()(const Detection& detection) {
    out_ << ToString(detection) << '\n';
}

// --- CountingSink ----------------------------------------------------------

void CountingSink::operator()(const Detection& detection) const {
    ++counter_->total;
    counter_->seen.push_back(SeenAlert{detection.rule, detection.severity,
                                   detection.ts, detection.pid,
                                   detection.actions});
}

// --- FanOutSink ------------------------------------------------------------

void FanOutSink::Add(AlertSink sink) {
    sinks_.push_back(std::move(sink));
}

void FanOutSink::operator()(const Detection& detection) const {
    for (const AlertSink& sink : sinks_) {
        sink(detection);
    }
}

// --- DedupSink -------------------------------------------------------------

DedupSink::DedupSink(AlertSink next, uint64_t window, std::size_t* dropped)
    : next_(std::move(next)), window_(window), dropped_(dropped) {}

void DedupSink::operator()(const Detection& d) {
    if (window_ == 0) {
        next_(d);
        return;
    }

    const uint64_t now = d.ts.ms;

    for (Record& rec : seen_) {
        if (rec.rule_id != d.rule || rec.pid != d.pid) {
            continue;
        }
        if (now >= rec.t0 && now - rec.t0 <= window_) {
            if (dropped_) {
                ++(*dropped_);
            }
            return;
        }
        rec.t0 = now;
        next_(d);
        return;
    }

    seen_.push_back({d.rule, d.pid, now});
    next_(d);
}

}