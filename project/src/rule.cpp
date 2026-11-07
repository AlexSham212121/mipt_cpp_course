#include "rule.h"

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity) : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

MatchRule::MatchRule(const std::string& id, Severity severity) : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition){
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event& event) {
    for (const auto& cond : conditions_) {
        if (!cond -> Matches(event)) {
            return false;
        }
    }
    CountHit();
    return true;
}

SequenceRule::SequenceRule(const std::string& id, Severity severity, uint64_t window_ms) : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}

void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Check(const Event& event) {
    if (first_ == nullptr || second_ == nullptr) {
        return false;
    }

    std::vector<Pending> fresh;
    for (const Pending& p : pending_) {
        if (event.ts().ms - p.ts_ms <= window_ms_) {
            fresh.push_back(p);
        }
    }
    pending_ = fresh;

    if (second_->Matches(event)) {
        for (std::size_t i = 0; i < pending_.size(); ++i) {
            bool pid_ok = true;
            if (same_pid_) {
                pid_ok = (pending_[i].pid == event.pid());
            }
            if (pid_ok) {
                pending_.erase(pending_.begin() + i);
                CountHit();
                return true;
            }
        }
    }

    if (first_->Matches(event)) {
        Pending p;
        p.ts_ms = event.ts().ms;
        p.pid = event.pid();
        pending_.push_back(p);
    }

    return false;
}
}