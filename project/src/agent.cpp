#include "agent.h"
#include "agent_rules.h"
#include "dedup_sink.h"
#include <print>
#include <utility>

namespace nano_edr {
    
Agent::Agent(bool quiet, uint64_t dedup_window)
    : dedup_window_(dedup_window),
      quiet_(quiet),
      engine_(BuildAgentRules(&model_)) {
    engine_.SetModel(&model_);
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintCtx(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_window_, &dropped_));
}

void Agent::PrintCtx() {
    if (quiet_ || context_printed_) return;
    context_printed_ = true;
    const std::size_t n = window_.size();
    if (n >= 2) {
        const WindowEntry& e = window_[n - 2];
        std::print("[CTX] -2: {} {} pid={}\n", e.ts.ms, e.type, e.pid);
    }
    if (n >= 1) {
        const WindowEntry& e = window_.back();
        std::print("[CTX] -1: {} {} pid={}\n", e.ts.ms, e.type, e.pid);
    }
}

void Agent::HandleEvent(const Event& event) {
    model_.Observe(event);
    if (!quiet_) {
        ++events_;
        ++events_by_type_[event.type()];
    }
    context_printed_ = false;
    engine_.ProcessEvent(event, nullptr);
    window_.PushBack(WindowEntry{event.ts(), event.type(), event.pid()});
}

void Agent::OnSkipped() {
    ++skipped_;
}

void Agent::OnBroken() {
    ++broken_;
}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) return;
    std::print("--------------------------------------------\n");
    std::print("Всего событий: {}\n", events_);
    for (const auto& [type, count] : events_by_type_) {
        std::print("{}: {}\n", type, count);
    }
    std::print("Пропущено: {}\n", skipped_);
    std::print("Ошибок разбора: {}\n", broken_);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), kWindowCapacity);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), kWindowCapacity);
    std::print("[STAT] в модели процессов {}, файлов {}\n", model_.process_count(), model_.file_count());
    std::print("--- Срабатывания правил ---\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::print("{}: {}\n", rule.id(), rule.hits());
    }
}

}