#include "ps3_profile.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
namespace nougat::games::ps3 {
namespace {
std::string home_dir() { if (const char* v=std::getenv("HOME")) return v; return "."; }
bool parse_bool(const std::string& v) { return v=="1"||v=="true"||v=="yes"||v=="on"; }
}
std::string profile_path() {
    if (const char* xdg=std::getenv("XDG_CONFIG_HOME"); xdg&&*xdg)
        return (std::filesystem::path(xdg)/"nougat-media-plus"/"ps3-graphics.conf").string();
    return (std::filesystem::path(home_dir())/".config"/"nougat-media-plus"/"ps3-graphics.conf").string();
}
Profile load_profile() {
    Profile p; std::ifstream in(profile_path()); std::string line;
    while (std::getline(in,line)) {
        const std::size_t pos=line.find('='); if (pos==std::string::npos) continue;
        const std::string k=line.substr(0,pos), v=line.substr(pos+1);
        try {
            if(k=="preset")p.preset=v; else if(k=="render_scale")p.render_scale=std::stoi(v);
            else if(k=="anisotropic")p.anisotropic=std::stoi(v); else if(k=="frame_limit")p.frame_limit=v;
            else if(k=="msaa")p.msaa=v; else if(k=="vsync")p.vsync=parse_bool(v);
            else if(k=="output_scaling")p.output_scaling=v; else if(k=="gpu_texture_scaling")p.gpu_texture_scaling=parse_bool(v);
            else if(k=="neural_enabled")p.neural_enabled=parse_bool(v); else if(k=="neural_strength")p.neural_strength=std::stoi(v);
            else if(k=="neural_quality")p.neural_quality=v; else if(k=="motion_reconstruction")p.motion_reconstruction=v;
            else if(k=="hud_protection")p.hud_protection=parse_bool(v); else if(k=="frame_generation")p.frame_generation=parse_bool(v);
        } catch(...) {}
    }
    p.render_scale=std::clamp(p.render_scale,25,800); p.anisotropic=std::clamp(p.anisotropic,0,16); p.neural_strength=std::clamp(p.neural_strength,0,100); return p;
}
bool neural_bridge_available() {
    if (const char* e=std::getenv("NOUGAT_PS3_NEURAL_LAYER")) if (*e && std::filesystem::exists(e)) return true;
    const auto r=std::filesystem::path(home_dir())/".local"/"share"/"nougat-media-plus"/"ps3-neural";
    return std::filesystem::exists(r/"libnougat_ps3_neural.so") || std::filesystem::exists(r/"VkLayer_nougat_ps3_neural.json");
}
bool save_profile(const Profile& input) {
    Profile p=input; p.render_scale=std::clamp(p.render_scale,25,800); p.anisotropic=std::clamp(p.anisotropic,0,16); p.neural_strength=std::clamp(p.neural_strength,0,100);
    if(!neural_bridge_available()){p.neural_enabled=false;p.frame_generation=false;}
    const std::filesystem::path path(profile_path()); std::error_code ec; std::filesystem::create_directories(path.parent_path(),ec); std::ofstream out(path,std::ios::trunc); if(!out)return false;
    out<<"# Nougat Media Plus v0.0.66 PS3 graphics profile\n"<<"preset="<<p.preset<<'\n'<<"render_scale="<<p.render_scale<<'\n'<<"anisotropic="<<p.anisotropic<<'\n'<<"frame_limit="<<p.frame_limit<<'\n'<<"msaa="<<p.msaa<<'\n'<<"vsync="<<(p.vsync?1:0)<<'\n'<<"output_scaling="<<p.output_scaling<<'\n'<<"gpu_texture_scaling="<<(p.gpu_texture_scaling?1:0)<<'\n'<<"neural_enabled="<<(p.neural_enabled?1:0)<<'\n'<<"neural_strength="<<p.neural_strength<<'\n'<<"neural_quality="<<p.neural_quality<<'\n'<<"motion_reconstruction="<<p.motion_reconstruction<<'\n'<<"hud_protection="<<(p.hud_protection?1:0)<<'\n'<<"frame_generation="<<(p.frame_generation?1:0)<<'\n'; return static_cast<bool>(out);
}
void apply_preset(Profile& p,const std::string& n){p.preset=n;
    if(n=="Original"){p.render_scale=100;p.anisotropic=0;p.frame_limit="Auto";p.msaa="Auto";p.vsync=false;p.output_scaling="Bilinear";p.gpu_texture_scaling=false;p.neural_enabled=false;p.neural_strength=0;p.frame_generation=false;}
    else if(n=="Performance"){p.render_scale=100;p.anisotropic=4;p.frame_limit="Auto";p.msaa="Auto";p.vsync=false;p.output_scaling="FidelityFX Super Resolution";p.gpu_texture_scaling=true;p.neural_strength=25;}
    else if(n=="Balanced"){p.render_scale=150;p.anisotropic=16;p.frame_limit="Auto";p.msaa="Auto";p.vsync=false;p.output_scaling="Bilinear";p.gpu_texture_scaling=true;p.neural_strength=50;}
    else if(n=="Quality"){p.render_scale=200;p.anisotropic=16;p.frame_limit="Auto";p.msaa="Auto";p.vsync=true;p.output_scaling="Bilinear";p.gpu_texture_scaling=true;p.neural_strength=70;}
    else if(n=="Ultra"){p.render_scale=300;p.anisotropic=16;p.frame_limit="Auto";p.msaa="Auto";p.vsync=true;p.output_scaling="Bilinear";p.gpu_texture_scaling=true;p.neural_strength=85;}
}
}
