#pragma once
#include <functional>
#include <cstdint>
#include <string>
#include <vector>
namespace reddmedia {
struct TunerDevice { std::string id; std::string name; std::string frontend_path; std::string video_path; std::string backend; std::string status; bool readable=false; bool hauppauge=false; };
struct LiveTvChannel { std::string id; std::string name; std::string service; std::string frequency; int program_number=0; int physical_channel=0; std::uint16_t source_id=0; };
struct TunerRuntimeStatus { bool frontend_accessible=false; bool demux_accessible=false; bool dvr_accessible=false; bool net_accessible=false; bool signal_lock=false; int signal_percent=-1; int quality_percent=-1; std::string delivery_systems; };
struct ChannelScanProgress { int physical_channel=0; unsigned frequency_hz=0; int completed=0; int total=0; bool locked=false; int signal_percent=-1; int quality_percent=-1; int channels_found=0; std::string message; };
using ChannelScanCallback=std::function<bool(const ChannelScanProgress&)>;
}
