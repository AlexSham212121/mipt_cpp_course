#include <charconv>
#include <cstdio>
#include <exception>
#include <memory> 
#include <print>
#include <string>
#include <string_view>
#include "agent.h"
#include "event_source.h"
#include "sources.h"
#include "edr_handle.h"
#include "field_traits.h"

namespace {

struct Options {
    std::string logPath;
    bool quiet = false;
    bool useFile = false;
    std::string out;
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
        }else if (arg == "--out" ){
            if (i + 1 >= argc) {
                return false;
            }
            options.out = argv[++i];
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
        std::print(stderr,
                   "использование: nano-edr [--quiet] [--out <файл>] [--file] <журнал.log>\n"
                   "  --quiet        не печатать [CTX] и сводку\n"
                   "  --out <файл>   дополнительно записать строки [DETECT] в файл\n");
        return 2;
    }

    try {
        nano_edr::EdrHandle edr(options.logPath);

        const std::string text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
            dedup_period_ms = 10000;
        }
        nano_edr::Agent agent(options.quiet, dedup_period_ms);

        nano_edr::FanOutSink targets;
        targets.Add(nano_edr::ConsoleSink());
        if (!options.out.empty()) {
            targets.Add(nano_edr::FileSink(options.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));

        std::unique_ptr<nano_edr::IEventSource> source;
        if (options.useFile) {
            source = std::make_unique<nano_edr::FileSource>(options.logPath);
        } else {
            source = std::make_unique<nano_edr::OsSource>(options.logPath);
        }
        source->Run(&agent);

        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();
    }catch (const std::exception& error) {
        std::print(stderr, "{}\n", error.what());
        return 1;
    }

    return 0;
}