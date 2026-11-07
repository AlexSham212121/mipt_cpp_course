#include "os_handle.h"

#include <stdexcept>
#include <string>

namespace nano_edr {

OsHandle::OsHandle(const std::string& config_path) {
    os_status s = os_init(config_path.c_str(), &handle_);
    if (s != OS_OK) {
        std::string message = "os_init(";
        message += config_path;
        message += "): ";
        message += os_status_str(s);
        throw std::runtime_error(message);
    }
}

OsHandle::~OsHandle() {
    os_free(handle_);
}

void OsHandle::Subscribe(os_event_cb callback, void* context) {
    os_status s = os_event_subscribe(handle_, callback, context);
    if (s != OS_OK) {
        std::string message = "os_event_subscribe: ";
        message = message + os_status_str(s);
        throw std::runtime_error(message);
    }
}

void OsHandle::Start() {
    os_status s = os_start(handle_);
    if (s != OS_OK) {
        std::string message = "os_start: ";
        message = message + os_status_str(s);
        throw std::runtime_error(message);
    }
}

os_status OsHandle::Wait(uint32_t timeout_ms) {
    os_status s = os_wait(handle_, timeout_ms);
    return s;
}

void OsHandle::Stop() {
    os_stop(handle_);
}

}