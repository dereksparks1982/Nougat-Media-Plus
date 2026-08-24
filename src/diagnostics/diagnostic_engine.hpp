#pragma once

#include "diagnostic_types.hpp"
#include <string>
#include <vector>

namespace reddmedia {

class DiagnosticEngine {
public:
    DiagnosticReport evaluate(const DiagnosticInput& input) const;
    static const char* severity_name(DiagnosticSeverity severity);
    static std::string report_text(const DiagnosticReport& report,
                                   const DiagnosticInput& input);
    static std::string report_json(const DiagnosticReport& report,
                                   const DiagnosticInput& input);
    static bool write_text_report(const DiagnosticReport& report,
                                  const DiagnosticInput& input,
                                  const std::string& path,
                                  std::string& error);
    static bool write_json_report(const DiagnosticReport& report,
                                  const DiagnosticInput& input,
                                  const std::string& path,
                                  std::string& error);
    static bool write_support_bundle(const DiagnosticReport& report,
                                     const DiagnosticInput& input,
                                     const std::string& archive_path,
                                     std::string& error);
    // Write one explicit history snapshot. Retained for the rejected-v0.0.39
    // main.cpp, which calls this directly after a diagnostic run.
    static bool write_history_snapshot(const DiagnosticReport& report,
                                       const DiagnosticInput& input,
                                       const std::string& path,
                                       std::string& error);
    static bool append_history(const DiagnosticReport& report,
                               const DiagnosticInput& input,
                               std::string& error);
    static std::vector<std::string> read_history(const DiagnosticInput& input,
                                                 std::size_t limit = 12U);
};

} // namespace reddmedia
