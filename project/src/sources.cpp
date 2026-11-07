#include "sources.h"
#include "parse.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <print>
#include <cstdio>

namespace nano_edr {

FileSource::FileSource(const std::string& path) : log_(path) {
    if (!log_.is_open()) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}
 
void FileSource::Run(IEventHandler* handler) {
    std::string line;
    while (std::getline(log_, line)) {
        try {
            if (IsBlankOrComment(line)){
            handler->OnSkipped();
            continue;
            }
            EventParts parts;
            if (!ParseEventParts(line, &parts)) {
                handler->OnBroken();
                continue;
            }
            Event event(parts);
            handler->HandleEvent(event);
        } catch (const std::exception& e) {
            handler->OnBroken();
            std::print(stderr, "Предупреждение: пропущено битое событие ({}).\n", e.what());
            continue;
        }
    }
}

OsSource::OsSource(const std::string& path) : handle_(path) {}

void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    IEventHandler* handler = static_cast<IEventHandler*>(ctx); 
    try {
        EventParts parts;
        parts.ts = std::to_string(ev->ts);
        parts.type = ev->type;
        if (ev->pid != 0) {
            parts.pid = std::to_string(ev->pid);
        }
        for (size_t i = 0; i < ev->field_count; ++i) {
            parts.fields.push_back({ev->fields[i].key, ev->fields[i].value});
        }
        Event event(parts);
        handler->HandleEvent(event);
     } catch (const std::exception& e) {
        handler->OnBroken();
        std::print(stderr, "os.h: пропущено битое событие: {}\n", e.what());
    } catch (...) {
        handler->OnBroken();
        std::print(stderr, "os.h: пропущено событие (неизвестная ошибка)\n");
    }
}

void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(Trampoline, handler);
    handle_.Start();

    while (true) {
        os_status status = handle_.Wait(100);
        
        if (status == OS_TIMEOUT) {
            handler->OnTick();
            continue;
        }
        
        if (status == OS_OK) {
            break;
        }
        
        throw std::runtime_error("ошибка os.h: " + std::string(os_status_str(status)));
    }
}

}