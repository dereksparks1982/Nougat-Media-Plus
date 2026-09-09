#pragma once

#include <string>
#include <vector>

namespace nougat::console {

enum class RuntimeKind { Native, Compatibility, Emulator, DedicatedServer };
enum class ContentSourceKind { Installed, Image, PhysicalMedia, NetworkService };

struct RuntimeCapability {
    std::string id;
    RuntimeKind runtime_kind{RuntimeKind::Native};
    std::vector<std::string> systems;
    std::vector<ContentSourceKind> sources;
    std::string executable;
    bool requires_firmware{false};
    bool supports_embedded_video{false};
    bool supports_direct_physical_media{false};
};

struct GameRuntimeRequest {
    std::string title;
    std::string system;
    ContentSourceKind source_kind{ContentSourceKind::Installed};
    std::string source;
};

struct RuntimeResolution {
    bool supported{false};
    RuntimeCapability runtime{};
    std::string reason;
};

class RuntimeManager {
public:
    void clear();
    void register_runtime(RuntimeCapability capability);
    [[nodiscard]] RuntimeResolution resolve(const GameRuntimeRequest& request) const;
    [[nodiscard]] const std::vector<RuntimeCapability>& runtimes() const noexcept;
    [[nodiscard]] std::vector<std::string> optical_devices() const;
    [[nodiscard]] static std::vector<std::string> optical_device_candidates();

private:
    std::vector<RuntimeCapability> runtimes_;
};

}  // namespace nougat::console
