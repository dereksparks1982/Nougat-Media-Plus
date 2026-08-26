#include "plugin_registry.hpp"

#include "platform/nougat_paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

namespace nougat::plugins {
namespace {

struct JsonValue {
    enum class Type { Null, Boolean, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolean = false;
    long long number = 0;
    std::string string;
    std::vector<JsonValue> array;
    std::map<std::string, JsonValue> object;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view text) : text_(text) {}

    bool parse(JsonValue& out, std::string& error) {
        skip_ws();
        if (!parse_value(out, error)) return false;
        skip_ws();
        if (pos_ != text_.size()) {
            error = "unexpected trailing data at byte " + std::to_string(pos_);
            return false;
        }
        return true;
    }

private:
    std::string_view text_;
    std::size_t pos_ = 0;

    void skip_ws() {
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_]);
            if (!std::isspace(c)) break;
            ++pos_;
        }
    }

    bool consume(char wanted) {
        if (pos_ >= text_.size() || text_[pos_] != wanted) return false;
        ++pos_;
        return true;
    }

    bool parse_value(JsonValue& out, std::string& error) {
        skip_ws();
        if (pos_ >= text_.size()) {
            error = "unexpected end of JSON";
            return false;
        }
        const char c = text_[pos_];
        if (c == '{') return parse_object(out, error);
        if (c == '[') return parse_array(out, error);
        if (c == '"') {
            out.type = JsonValue::Type::String;
            return parse_string(out.string, error);
        }
        if (c == 't') return parse_literal("true", JsonValue::Type::Boolean, out, error, true);
        if (c == 'f') return parse_literal("false", JsonValue::Type::Boolean, out, error, false);
        if (c == 'n') return parse_literal("null", JsonValue::Type::Null, out, error, false);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number(out, error);
        error = "unexpected JSON token at byte " + std::to_string(pos_);
        return false;
    }

    bool parse_literal(std::string_view literal, JsonValue::Type type,
                       JsonValue& out, std::string& error, bool boolean) {
        if (text_.substr(pos_, literal.size()) != literal) {
            error = "invalid JSON literal at byte " + std::to_string(pos_);
            return false;
        }
        pos_ += literal.size();
        out.type = type;
        out.boolean = boolean;
        return true;
    }

    bool parse_number(JsonValue& out, std::string& error) {
        const std::size_t start = pos_;
        bool negative = false;
        if (consume('-')) negative = true;
        if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            error = "invalid JSON integer at byte " + std::to_string(start);
            return false;
        }
        unsigned long long value = 0;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            const unsigned digit = static_cast<unsigned>(text_[pos_] - '0');
            if (value > (static_cast<unsigned long long>(INT64_MAX) - digit) / 10ULL) {
                error = "JSON integer out of range at byte " + std::to_string(start);
                return false;
            }
            value = value * 10ULL + digit;
            ++pos_;
        }
        if (pos_ < text_.size() && (text_[pos_] == '.' || text_[pos_] == 'e' || text_[pos_] == 'E')) {
            error = "plugin manifest numbers must be integers";
            return false;
        }
        long long signed_value = static_cast<long long>(value);
        if (negative) signed_value = -signed_value;
        out.type = JsonValue::Type::Number;
        out.number = signed_value;
        return true;
    }

    bool parse_hex4(unsigned& value, std::string& error) {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            if (pos_ >= text_.size()) {
                error = "truncated unicode escape";
                return false;
            }
            const char c = text_[pos_++];
            unsigned digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(10 + c - 'a');
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(10 + c - 'A');
            else {
                error = "invalid unicode escape";
                return false;
            }
            value = (value << 4U) | digit;
        }
        return true;
    }

    static void append_utf8(std::string& out, unsigned codepoint) {
        if (codepoint <= 0x7FU) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FFU) {
            out.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
            out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        } else {
            out.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
            out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
        }
    }

    bool parse_string(std::string& out, std::string& error) {
        if (!consume('"')) {
            error = "expected JSON string at byte " + std::to_string(pos_);
            return false;
        }
        out.clear();
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') return true;
            if (c < 0x20U) {
                error = "control character in JSON string";
                return false;
            }
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (pos_ >= text_.size()) {
                error = "truncated JSON escape";
                return false;
            }
            const char escaped = text_[pos_++];
            switch (escaped) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    unsigned codepoint = 0;
                    if (!parse_hex4(codepoint, error)) return false;
                    if (codepoint >= 0xD800U && codepoint <= 0xDFFFU) {
                        error = "surrogate unicode escapes are not supported in plugin manifests";
                        return false;
                    }
                    append_utf8(out, codepoint);
                    break;
                }
                default:
                    error = "invalid JSON escape";
                    return false;
            }
        }
        error = "unterminated JSON string";
        return false;
    }

    bool parse_array(JsonValue& out, std::string& error) {
        if (!consume('[')) return false;
        out.type = JsonValue::Type::Array;
        out.array.clear();
        skip_ws();
        if (consume(']')) return true;
        while (true) {
            JsonValue item;
            if (!parse_value(item, error)) return false;
            out.array.push_back(std::move(item));
            skip_ws();
            if (consume(']')) return true;
            if (!consume(',')) {
                error = "expected ',' or ']' at byte " + std::to_string(pos_);
                return false;
            }
            skip_ws();
        }
    }

    bool parse_object(JsonValue& out, std::string& error) {
        if (!consume('{')) return false;
        out.type = JsonValue::Type::Object;
        out.object.clear();
        skip_ws();
        if (consume('}')) return true;
        while (true) {
            std::string key;
            if (!parse_string(key, error)) return false;
            skip_ws();
            if (!consume(':')) {
                error = "expected ':' after object key";
                return false;
            }
            JsonValue value;
            if (!parse_value(value, error)) return false;
            if (!out.object.emplace(key, std::move(value)).second) {
                error = "duplicate JSON key: " + key;
                return false;
            }
            skip_ws();
            if (consume('}')) return true;
            if (!consume(',')) {
                error = "expected ',' or '}' at byte " + std::to_string(pos_);
                return false;
            }
            skip_ws();
        }
    }
};

const JsonValue* member(const JsonValue& object, const std::string& key) {
    if (object.type != JsonValue::Type::Object) return nullptr;
    const auto it = object.object.find(key);
    return it == object.object.end() ? nullptr : &it->second;
}

bool required_string(const JsonValue& object, const std::string& key,
                     std::string& out, std::string& error) {
    const JsonValue* value = member(object, key);
    if (value == nullptr || value->type != JsonValue::Type::String || value->string.empty()) {
        error = "required string field is missing or empty: " + key;
        return false;
    }
    out = value->string;
    return true;
}

bool required_integer(const JsonValue& object, const std::string& key,
                      int& out, std::string& error) {
    const JsonValue* value = member(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Number ||
        value->number < 0 || value->number > INT32_MAX) {
        error = "required integer field is missing or invalid: " + key;
        return false;
    }
    out = static_cast<int>(value->number);
    return true;
}

bool required_boolean(const JsonValue& object, const std::string& key,
                      bool& out, std::string& error) {
    const JsonValue* value = member(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Boolean) {
        error = "required boolean field is missing or invalid: " + key;
        return false;
    }
    out = value->boolean;
    return true;
}

bool string_array(const JsonValue& object, const std::string& key,
                  std::vector<std::string>& out, std::string& error) {
    out.clear();
    const JsonValue* value = member(object, key);
    if (value == nullptr) return true;
    if (value->type != JsonValue::Type::Array) {
        error = "field must be an array: " + key;
        return false;
    }
    for (const JsonValue& item : value->array) {
        if (item.type != JsonValue::Type::String || item.string.empty()) {
            error = "field must contain only non-empty strings: " + key;
            return false;
        }
        out.push_back(item.string);
    }
    return true;
}

bool safe_relative_path(const fs::path& value) {
    if (value.empty() || value.is_absolute()) return false;
    for (const fs::path& part : value) {
        if (part.empty() || part == "." || part == "..") return false;
    }
    return true;
}

bool read_file(const fs::path& path, std::string& out, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "unable to open " + path.string();
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        error = "unable to read " + path.string();
        return false;
    }
    out = buffer.str();
    if (out.size() > 1024U * 1024U) {
        error = "plugin manifest exceeds 1 MiB";
        return false;
    }
    return true;
}

} // namespace

bool safe_plugin_id(const std::string& id) {
    if (id.empty() || id.size() > 96U) return false;
    for (unsigned char c : id) {
        if (std::islower(c) || std::isdigit(c) || c == '-' || c == '_') continue;
        return false;
    }
    return id != "." && id != "..";
}

bool load_plugin_manifest(const fs::path& plugin_root,
                          const std::string& expected_id,
                          PluginManifest& out,
                          std::string& error) {
    out = PluginManifest{};
    error.clear();

    if (!safe_plugin_id(expected_id)) {
        error = "unsafe plugin directory id: " + expected_id;
        return false;
    }

    std::error_code ec;
    if (!fs::is_directory(plugin_root, ec) || ec) {
        error = "plugin root is not a directory: " + plugin_root.string();
        return false;
    }

    const fs::path manifest_path = plugin_root / "plugin.json";
    if (!fs::is_regular_file(manifest_path, ec) || ec) {
        error = "plugin.json is missing";
        return false;
    }

    std::string text;
    if (!read_file(manifest_path, text, error)) return false;

    JsonValue root;
    JsonParser parser(text);
    if (!parser.parse(root, error)) return false;
    if (root.type != JsonValue::Type::Object) {
        error = "plugin manifest root must be a JSON object";
        return false;
    }

    std::string format;
    if (!required_string(root, "format", format, error)) return false;
    if (format != "NOUGAT_PLUGIN") {
        error = "unsupported plugin format: " + format;
        return false;
    }
    if (!required_integer(root, "format_version", out.format_version, error)) return false;
    if (out.format_version != kPluginFormatVersion) {
        error = "unsupported plugin format version: " + std::to_string(out.format_version);
        return false;
    }
    if (!required_integer(root, "nougat_plugin_api", out.api_version, error)) return false;
    if (out.api_version != kPluginApiVersion) {
        error = "unsupported Nougat plugin API: " + std::to_string(out.api_version);
        return false;
    }
    if (!required_string(root, "id", out.id, error)) return false;
    if (!safe_plugin_id(out.id) || out.id != expected_id) {
        error = "manifest id does not safely match plugin directory: " + out.id;
        return false;
    }
    if (!required_string(root, "display_name", out.display_name, error)) return false;
    if (!required_string(root, "version", out.version, error)) return false;
    if (!required_string(root, "description", out.description, error)) return false;
    if (!required_string(root, "top_level_tab", out.top_level_tab, error)) return false;
    if (!required_boolean(root, "required_for_application_start", out.required_for_application_start, error)) return false;
    if (!required_boolean(root, "recommended_by_default", out.recommended_by_default, error)) return false;
    if (out.required_for_application_start) {
        error = "optional plugins may not be required for player-core startup";
        return false;
    }

    const JsonValue* runtime = member(root, "runtime");
    if (runtime == nullptr || runtime->type != JsonValue::Type::Object) {
        error = "required object field is missing or invalid: runtime";
        return false;
    }
    std::string entrypoint_text;
    if (!required_string(*runtime, "kind", out.runtime_kind, error)) return false;
    if (out.runtime_kind != "x11-process") {
        error = "unsupported plugin runtime kind: " + out.runtime_kind;
        return false;
    }
    if (!required_string(*runtime, "entrypoint", entrypoint_text, error)) return false;
    const fs::path relative_entrypoint(entrypoint_text);
    if (!safe_relative_path(relative_entrypoint)) {
        error = "unsafe plugin entrypoint path";
        return false;
    }

    out.root = plugin_root;
    out.manifest_path = manifest_path;
    out.entrypoint = plugin_root / relative_entrypoint;
    if (!fs::is_regular_file(out.entrypoint, ec) || ec) {
        error = "plugin entrypoint is missing: " + out.entrypoint.string();
        return false;
    }
    if (::access(out.entrypoint.c_str(), X_OK) != 0) {
        error = "plugin entrypoint is not executable: " + out.entrypoint.string();
        return false;
    }

    if (!string_array(root, "dependencies", out.dependencies, error)) return false;
    if (!string_array(root, "features", out.features, error)) return false;
    for (const std::string& dependency : out.dependencies) {
        if (!safe_plugin_id(dependency)) {
            error = "unsafe plugin dependency id: " + dependency;
            return false;
        }
    }
    return true;
}

PluginScanResult scan_installed_plugins() {
    PluginScanResult result;
    const fs::path root = nougat::paths::layout().plugins;
    std::error_code ec;
    if (!fs::exists(root, ec)) return result;
    if (ec || !fs::is_directory(root, ec) || ec) {
        result.issues.push_back({root, "managed plugin root is not a readable directory"});
        return result;
    }

    fs::directory_iterator iterator(root, fs::directory_options::skip_permission_denied, ec);
    const fs::directory_iterator end;
    for (; !ec && iterator != end; iterator.increment(ec)) {
        const fs::directory_entry& entry = *iterator;
        std::error_code entry_ec;
        if (!entry.is_directory(entry_ec) || entry_ec) continue;
        const std::string expected_id = entry.path().filename().string();
        PluginManifest manifest;
        std::string error;
        if (!load_plugin_manifest(entry.path(), expected_id, manifest, error)) {
            result.issues.push_back({entry.path(), error});
            continue;
        }
        result.plugins.push_back(std::move(manifest));
    }
    if (ec) result.issues.push_back({root, "plugin directory scan failed: " + ec.message()});

    std::sort(result.plugins.begin(), result.plugins.end(),
              [](const PluginManifest& a, const PluginManifest& b) { return a.id < b.id; });
    std::sort(result.issues.begin(), result.issues.end(),
              [](const PluginIssue& a, const PluginIssue& b) { return a.path.string() < b.path.string(); });
    return result;
}

} // namespace nougat::plugins
