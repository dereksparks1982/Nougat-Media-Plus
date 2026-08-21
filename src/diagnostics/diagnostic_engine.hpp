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
};

} // namespace reddmedia
