#pragma once

#include "diagnostic_types.hpp"

#include <string>

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
};

} // namespace reddmedia
