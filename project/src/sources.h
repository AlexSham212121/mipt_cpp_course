#ifndef NANO_EDR_SOURCES_H
#define NANO_EDR_SOURCES_H

#include <string>
#include <fstream>

#include "event_source.h"
#include "os_handle.h"

namespace nano_edr {

class FileSource : public IEventSource {
public:
    explicit FileSource(const std::string& path);
    void Run(IEventHandler* handler) override;

private:
    std::ifstream log_;
};

class OsSource : public IEventSource {
public:
    explicit OsSource(const std::string& path);
    void Run(IEventHandler* handler) override;
    static void Trampoline(const os_event* ev, void* ctx) noexcept;

private:
    OsHandle handle_;
};

}

#endif