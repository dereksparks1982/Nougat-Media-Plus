#include "console/runtime_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <set>
#include <utility>

namespace nougat::console {

namespace {

bool has_system(const RuntimeCapability& capability, const std::string& system) {
    return std::find(capability.systems.begin(), capability.systems.end(), system) != capability.systems.end();
}

bool has_source(const RuntimeCapability& capability, ContentSourceKind source) {
    return std::find(capability.sources.begin(), capability.sources.end(), source) != capability.sources.end();
}

}  // namespace

void RuntimeManager::clear() {
    runtimes_.clear();
}

void RuntimeManager::register_runtime(RuntimeCapability capability) {
    const auto duplicate = std::find_if(runtimes_.begin(), runtimes_.end(), [&](const RuntimeCapability& existing) {
        return existing.id == capability.id;
    });
    if (duplicate != runtimes_.end()) {
        *duplicate = std::move(capability);
        return;
    }
    runtimes_.push_back(std::move(capability));
}

RuntimeResolution RuntimeManager::resolve(const GameRuntimeRequest& request) const {
    for (const auto& runtime : runtimes_) {
        if (!has_system(runtime, request.system) || !has_source(runtime, request.source_kind)) {
            continue;
        }
        if (request.source_kind == ContentSourceKind::PhysicalMedia && !runtime.supports_direct_physical_media) {
            continue;
        }
        if (runtime.executable.empty()) {
            continue;
        }
        return RuntimeResolution{true, runtime, "resolved by capability"};
    }

    return RuntimeResolution{false, RuntimeCapability{}, "no registered runtime satisfies system/source capability"};
}

const std::vector<RuntimeCapability>& RuntimeManager::runtimes() const noexcept {
    return runtimes_;
}

std::vector<std::string> RuntimeManager::optical_device_candidates() {
    std::vector<std::string> candidates;
    candidates.reserve(18);
    for (int index = 0; index < 16; ++index) {
        candidates.push_back("/dev/sr" + std::to_string(index));
    }
    candidates.emplace_back("/dev/cdrom");
    candidates.emplace_back("/dev/dvd");
    return candidates;
}

std::vector<std::string> RuntimeManager::optical_devices() const {
    std::set<std::string> unique;
    std::vector<std::string> devices;
    for (const auto& candidate : optical_device_candidates()) {
        std::error_code error;
        if (!std::filesystem::exists(candidate, error) || error) {
            continue;
        }
        std::filesystem::path path(candidate);
        const auto canonical = std::filesystem::weakly_canonical(path, error);
        const std::string value = error ? path.string() : canonical.string();
        if (unique.insert(value).second) {
            devices.push_back(value);
        }
    }
    return devices;
}

}  // namespace nougat::console
