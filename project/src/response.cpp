#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include <print>

#include "detection.h"
#include "entity_model.h"
#include "event.h"
#include "fields.h"
#include "os_handle.h"
#include "response.h"

namespace nano_edr {

namespace {

bool StartsWithNoCase(const std::string& text, const std::string& prefix) {
    if (text.size() < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        const unsigned char a = static_cast<unsigned char>(text[i]);
        const unsigned char b = static_cast<unsigned char>(prefix[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

bool SystemPath(const std::string& path) {
    return StartsWithNoCase(path, "C:\\Windows\\") || StartsWithNoCase(path, "C:\\Program Files");
}

}

// --- ResponsePlanner -------------------------------------------------------

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(std::move(quarantine_dir)) {}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    const std::vector<std::string> tree = model_->GetProcessTree(detection.pid);

    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t i = tree.size(); i > 0; --i) {
                const std::string& pid = tree[i - 1];
                Action action;
                action.kind = ActionKind::kKillProcess;
                action.pid = pid;
                action.expected_start = model_->FindProcess(pid)->start;
                action.reason = detection.rule;
                if (pid != detection.pid) {
                    action.reason += ", потомок " + detection.pid;
                }
                plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source") {
            const FileRecord* source = model_->FindSpawnSource(detection.pid);
            if (source != nullptr && !SystemPath(source->path)) {
                const std::string to = QuarantinePath(source->path, detection.pid);
                if (!to.empty()) {
                    Action action;
                    action.kind = ActionKind::kQuarantineFile;
                    action.path = source->path;
                    action.to = to;
                    action.reason = detection.rule + ", источник запуска";
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "quarantine_image") {
            const ProcessRecord* process = model_->FindProcess(detection.pid);
            if (process != nullptr && !process->image.empty() && !SystemPath(process->image)) {
                const std::string to = QuarantinePath(process->image, detection.pid);
                if (!to.empty()) {
                    Action action;
                    action.kind = ActionKind::kQuarantineFile;
                    action.path = process->image;
                    action.to = to;
                    action.reason = detection.rule + ", образ процесса";
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                for (const std::string& path : model_->GetFilesCreatedBy(pid, true)) {
                    if (SystemPath(path)) {
                        continue;
                    }
                    Action action;
                    action.kind = ActionKind::kDeleteFile;
                    action.path = path;
                    action.reason = detection.rule + ", принесён " + pid;
                    plan.Add(std::move(action));
                }
            }
        } else if (name == "delete_evidence") {
            for (const auto& event : detection.evidence) {
                const std::string* path = FindField(*event, event->event_type() == EventType::kFileMove ? "to" : "path");
                if (path == nullptr || SystemPath(*path)) {
                    continue;
                }
                Action action;
                action.kind = ActionKind::kDeleteFile;
                action.path = *path;
                action.reason = detection.rule + ", файл из детекта";
                plan.Add(std::move(action));
            }
        }
    }
    return plan;
}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) {
        return "";
    }
    std::size_t start = 0;
    for (std::size_t i = path.size(); i > 0; --i) {
        if (path[i - 1] == '\\') {
            start = i;
            break;
        }
    }
    return quarantine_dir_ + "\\" + pid + "-" + path.substr(start);
}

// --- Responder -------------------------------------------------------------

Responder::Responder(OsHandle* os, bool dry_run) : os_(os), dry_run_(dry_run) {}

void Responder::Execute(const ResponsePlan& plan) {
    for (const Action& action : plan.actions()) {
        ++stats_.planned;

        // цель, по которой уже отработали: на одном процессе срабатывает
        // несколько правил
        bool repeat = false;
        for (const std::string& target : done_) {
            if (target == action.target()) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());

        if (dry_run_ || os_ == nullptr) {
            std::print("[PLAN] {}\n", ToString(action));
            continue;
        }
        if (action.kind == ActionKind::kKillProcess) {
            Kill(action);
        } else if (action.kind == ActionKind::kQuarantineFile) {
            MoveToQuarantine(action);
        } else if (action.kind == ActionKind::kDeleteFile) {
            Delete(action);
        }
    }
}

}  // namespace nano_edr