#pragma once
#include <string>
namespace nougat::games::ps3 {
struct Profile {
    std::string preset = "Balanced";
    int render_scale = 150;
    int anisotropic = 16;
    std::string frame_limit = "Auto";
    std::string msaa = "Auto";
    bool vsync = false;
    std::string output_scaling = "Bilinear";
    bool gpu_texture_scaling = true;
    bool neural_enabled = false;
    int neural_strength = 50;
    std::string neural_quality = "Quality";
    std::string motion_reconstruction = "Auto";
    bool hud_protection = true;
    bool frame_generation = false;
};
Profile load_profile();
bool save_profile(const Profile& profile);
void apply_preset(Profile& profile, const std::string& preset);
bool neural_bridge_available();
std::string profile_path();
}
