#include "agent_rules.h"
#include "rule.h"
#include "conditions.h"
#include "make_rule.h"
#include "combinators.h"

#include <memory>
#include <string>
#include <vector>

namespace nano_edr {

RuleEngine BuildAgentRules() {
    RuleEngine engine;

    auto r1 = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    r1->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType> {EventType::kProcessStart} ));
    auto image1 = std::make_unique<AnyOf>();
    image1->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    image1->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    r1->AddCondition(std::move(image1));
    auto temp1 = std::make_unique<AnyOf>();
    temp1->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp1->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp1->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp1->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp1->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp1->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    r1->AddCondition(std::move(temp1));
    r1->AddAction("kill_process");
    engine.AddRule(std::move(r1));

    auto r2 = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    r2->AddCondition(std::make_unique<EventTypeIs> (std::vector<EventType> {EventType::kProcessStart} ));
    auto image2 = std::make_unique<AnyOf>();
    image2->Add(std::make_unique<FieldEndsWith>("image", "certutil.exe"));
    image2->Add(std::make_unique<FieldEndsWith>("image", "bitsadmin.exe"));
    r2->AddCondition(std::move(image2));
    auto temp2 = std::make_unique<AnyOf>();
    temp2->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    temp2->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    temp2->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    temp2->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    r2->AddCondition(std::move(temp2));
    r2->AddAction("kill_process");
    engine.AddRule(std::move(r2));

    auto r3 = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    r3->AddCondition(std::make_unique<EventTypeIs> (std::vector<EventType> {EventType::kProcessStart} ));
    auto image3 = std::make_unique<AnyOf>();
    image3->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    image3->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    r3->AddCondition(std::move(image3));
    auto temp3 = std::make_unique<AnyOf>();
    temp3->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    temp3->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    temp3->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    temp3->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    r3->AddCondition(std::move(temp3));
    r3->AddAction("kill_process");
    engine.AddRule(std::move(r3));

    auto r4 = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);

    auto variant_4a = std::make_unique<AllOf>();
    variant_4a->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite}));
    auto paths_4a = std::make_unique<AnyOf>();
    paths_4a->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths_4a->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    variant_4a->Add(std::move(paths_4a));

    auto variant_4b = std::make_unique<AllOf>();
    variant_4b->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileMove}));
    auto paths_4b = std::make_unique<AnyOf>();
    paths_4b->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths_4b->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    variant_4b->Add(std::move(paths_4b));

    auto either4 = std::make_unique<AnyOf>();
    either4->Add(std::move(variant_4a));
    either4->Add(std::move(variant_4b));
    r4->AddCondition(std::move(either4));

    r4->AddAction("quarantine_file");
    engine.AddRule(std::move(r4));

    auto r5 = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);

    auto variant_5a = std::make_unique<AllOf>();
    variant_5a->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite}));
    variant_5a->Add(std::make_unique<FieldEndsWith>("path", ".locked"));

    auto variant_5b = std::make_unique<AllOf>();
    variant_5b->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileMove}));
    variant_5b->Add(std::make_unique<FieldEndsWith>("to", ".locked"));

    auto either_5 = std::make_unique<AnyOf>();
    either_5->Add(std::move(variant_5a));
    either_5->Add(std::move(variant_5b));
    r5->AddCondition(std::move(either_5));

    r5->AddAction("quarantine_file");
    engine.AddRule(std::move(r5));

    auto r6 = std::make_unique<SequenceRule>("script_host_beacon", Severity::kCritical, 10000);
    auto start6 = std::make_unique<AllOf>();

    start6->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));

    auto image6 = std::make_unique<AnyOf>();
    image6->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    image6->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    start6->Add(std::move(image6));

    auto temp6 = std::make_unique<AnyOf>();
    temp6->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp6->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp6->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp6->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp6->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp6->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    start6->Add(std::move(temp6));
    r6->SetFirst(std::move(start6));
    r6->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));
    r6->set_same_pid(false);
    r6->AddAction("kill_process");
    engine.AddRule(std::move(r6));

    auto mass_write = MakeRule<ThresholdRule<128>>("ransomware_mass_write", Severity::kCritical, 20, 10000, "pid");
    mass_write->SetCondition(MakeCondition(AllOfStatic(EventTypeIs({EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}), 
        AnyOfStatic(FieldContains("path", "\\Documents\\"),
                    FieldContains("path", "\\Desktop\\"),
                    FieldContains("to", "\\Documents\\"),
                    FieldContains("to", "\\Desktop\\")))));
    mass_write->AddAction("kill_process");
    mass_write->AddAction("quarantine_file");
    engine.AddRule(std::move(mass_write));

    return engine;
}

}