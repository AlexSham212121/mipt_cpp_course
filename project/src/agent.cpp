#include "agent.h"
#include "agent_rules.h"
#include <print>

namespace nano_edr {

Agent::Agent(bool quiet)
    : quiet_(quiet), engine_(BuildAgentRules()) {
}

void Agent::HandleEvent(const Event& event) {
    if (!quiet_) {
        ++events_;
        ++events_by_type_[event.type()];
    }

    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);

    if (!fired.empty() && !quiet_) {
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

    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule->severity()),
                   rule->id(),
                   event.raw_ts(),
                   event.pid());
    }

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
    std::print("--- Срабатывания правил ---\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::print("{}: {}\n", rule.id(), rule.hits());
    }
}

}