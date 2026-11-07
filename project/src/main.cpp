#include <cstdio>
#include <exception>
#include <memory> 
#include <print>
#include <string>
#include <string_view>
#include "agent.h"
#include "event_source.h"
#include "sources.h"

namespace {

struct Options {
    std::string logPath;
    bool quiet = false;
    bool useFile = false;
};
}

bool ParseOptions(int argc, char** argv, Options* out) {
    if (!out || argc < 2) {
        return false;
    }

    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];

        if (arg == "--quiet") {
            options.quiet = true;
        } else if (arg == "--file") {
            if (i + 1 >= argc || !options.logPath.empty()) return false;
            options.useFile = true;
            options.logPath = argv[++i];
        } else if (!arg.empty() && arg.front() != '-' && options.logPath.empty()) {
            options.logPath = arg;
        } else {
            return false;
        }
    }

    if (options.logPath.empty()) {
        return false;
    }

    *out = std::move(options);
    return true;
}

int main(int argc, char** argv) {
    Options options;
    if (!ParseOptions(argc, argv, &options)) {
        std::print(stderr, "использование: nano-edr [--quiet] [--file] <журнал.log>\n");
        return 2;
    }

    try {
        nano_edr::Agent agent(options.quiet);  
        std::unique_ptr<nano_edr::IEventSource> source;
        if (options.useFile) {
           source = std::make_unique<nano_edr::FileSource>(options.logPath);
        } else {
            source = std::make_unique<nano_edr::OsSource>(options.logPath);
        }
        source->Run(&agent);

        agent.PrintSummary();
    }catch (const std::exception& error) {
        std::print(stderr, "{}\n", error.what());
        return 1;
    }

    return 0;
}