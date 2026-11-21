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

void Agent::EnableResponse(OsHandle* os, bool dry_run, const std::string& quarantine_dir) {
    planner_ = std::make_unique<ResponsePlanner>(&model_, quarantine_dir);
    responder_ = std::make_unique<Responder>(os, dry_run);
    engine_.Subscribe([this](const Detection& detection) {
        responder_->Execute(planner_->Plan(detection));
    });
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
    if (responder_ != nullptr && responder_->Recognize(event)) {
        WindowEntry own;
        own.ts = event.ts();
        own.type = event.type();
        own.pid = event.pid();
        window_.PushBack(std::move(own));
        return;
    }
    context_printed_ = false;
    engine_.ProcessEvent(event, nullptr);
    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
    window_.PushBack(std::move(entry));
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
    if (responder_ != nullptr) {
        const ResponseStats& r = responder_->stats();
        std::print("[STAT] действий запланировано {}, повторов отброшено {}\n", r.planned, r.skipped_repeat);
        std::print("[STAT] завершено процессов {}, изолировано файлов {}, удалено файлов {}\n", r.killed, r.quarantined, r.deleted);
        if (r.own_events > 0) {
            std::print("[STAT] своих событий {}\n", r.own_events);
        }
    }
    std::print("--- Срабатывания правил ---\n");
    for (std::size_t i = 0; i < engine_.size(); ++i) {
        const IRule& rule = engine_.rule(i);
        std::print("{}: {}\n", rule.id(), rule.hits());
    }
}

}