#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <memory>
#include <map>
#include <random>
#include <mutex>
#include <set>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include <cerrno>
#include <cctype>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include "p2p_engine.hpp"
#include "p2p_stream_server.hpp"
#include "ytdlp_stream_server.hpp"
#include "nougat_media_suite_icon_data.hpp"
#include "nougat_quilt_texture_data.hpp"
#include "nougat_ui_sheet_texture_data.hpp"
#include "media_server/jellyfin_api_client.hpp"
#include "media_server/library_poster.hpp"
#include "media_server/library_metadata_cache.hpp"
#include "media_server/media_server_manager.hpp"
#include "live_tv/tuner_backend.hpp"
#include "diagnostics/diagnostic_engine.hpp"
#include "recommendations/recommendation_engine.hpp"
#include "recommendations/watch_provider_preferences.hpp"
#include "nougat/nougat_bridge.hpp"

struct libvlc_instance_t;
struct libvlc_media_t;
struct libvlc_media_player_t;

typedef libvlc_instance_t* (*fn_libvlc_new)(int, const char* const*);
typedef void (*fn_libvlc_release)(libvlc_instance_t*);
typedef libvlc_media_t* (*fn_libvlc_media_new_path)(libvlc_instance_t*, const char*);
typedef libvlc_media_t* (*fn_libvlc_media_new_location)(libvlc_instance_t*, const char*);
typedef void (*fn_libvlc_media_release)(libvlc_media_t*);
typedef void (*fn_libvlc_media_add_option)(libvlc_media_t*, const char*);
typedef libvlc_media_player_t* (*fn_libvlc_media_player_new_from_media)(libvlc_media_t*);
typedef void (*fn_libvlc_media_player_release)(libvlc_media_player_t*);
typedef int (*fn_libvlc_media_player_play)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_pause)(libvlc_media_player_t*, int);
typedef void (*fn_libvlc_media_player_stop)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_xwindow)(libvlc_media_player_t*, unsigned int);
typedef long long (*fn_libvlc_media_player_get_time)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_time)(libvlc_media_player_t*, long long);
typedef long long (*fn_libvlc_media_player_get_length)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_get_volume)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_set_volume)(libvlc_media_player_t*, int);
typedef int (*fn_libvlc_media_player_get_state)(libvlc_media_player_t*);
typedef const char* (*fn_libvlc_get_version)(void);

struct libvlc_track_description_t {
    int i_id;
    char* psz_name;
    libvlc_track_description_t* p_next;
};
struct libvlc_chapter_description_t {
    long long i_time_offset;
    long long i_duration;
    char* psz_name;
};

typedef libvlc_track_description_t* (*fn_libvlc_audio_get_track_description)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_get_track)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_set_track)(libvlc_media_player_t*, int);
typedef libvlc_track_description_t* (*fn_libvlc_video_get_spu_description)(libvlc_media_player_t*);
typedef int (*fn_libvlc_video_get_spu)(libvlc_media_player_t*);
typedef int (*fn_libvlc_video_set_spu)(libvlc_media_player_t*, int);
typedef int (*fn_libvlc_video_set_subtitle_file)(libvlc_media_player_t*, const char*);
typedef long long (*fn_libvlc_video_get_spu_delay)(libvlc_media_player_t*);
typedef int (*fn_libvlc_video_set_spu_delay)(libvlc_media_player_t*, long long);
typedef void (*fn_libvlc_track_description_list_release)(libvlc_track_description_t*);
typedef int (*fn_libvlc_media_player_get_chapter_count)(libvlc_media_player_t*);
typedef int (*fn_libvlc_media_player_get_chapter)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_chapter)(libvlc_media_player_t*, int);
typedef int (*fn_libvlc_media_player_get_title)(libvlc_media_player_t*);
typedef int (*fn_libvlc_media_player_get_full_chapter_descriptions)(libvlc_media_player_t*, int, libvlc_chapter_description_t***);
typedef void (*fn_libvlc_chapter_descriptions_release)(libvlc_chapter_description_t**, unsigned);

struct VlcApi {
    void* handle = nullptr;
    fn_libvlc_new new_ = nullptr;
    fn_libvlc_release release = nullptr;
    fn_libvlc_media_new_path media_new_path = nullptr;
    fn_libvlc_media_new_location media_new_location = nullptr;
    fn_libvlc_media_release media_release = nullptr;
    fn_libvlc_media_add_option media_add_option = nullptr;
    fn_libvlc_media_player_new_from_media player_new_from_media = nullptr;
    fn_libvlc_media_player_release player_release = nullptr;
    fn_libvlc_media_player_play play = nullptr;
    fn_libvlc_media_player_set_pause set_pause = nullptr;
    fn_libvlc_media_player_stop stop = nullptr;
    fn_libvlc_media_player_set_xwindow set_xwindow = nullptr;
    fn_libvlc_media_player_get_time get_time = nullptr;
    fn_libvlc_media_player_set_time set_time = nullptr;
    fn_libvlc_media_player_get_length get_length = nullptr;
    fn_libvlc_audio_get_volume get_volume = nullptr;
    fn_libvlc_audio_set_volume set_volume = nullptr;
    fn_libvlc_media_player_get_state get_state = nullptr;
    fn_libvlc_get_version get_version = nullptr;
    fn_libvlc_audio_get_track_description audio_get_track_description = nullptr;
    fn_libvlc_audio_get_track audio_get_track = nullptr;
    fn_libvlc_audio_set_track audio_set_track = nullptr;
    fn_libvlc_video_get_spu_description video_get_spu_description = nullptr;
    fn_libvlc_video_get_spu video_get_spu = nullptr;
    fn_libvlc_video_set_spu video_set_spu = nullptr;
    fn_libvlc_video_set_subtitle_file video_set_subtitle_file = nullptr;
    fn_libvlc_video_get_spu_delay video_get_spu_delay = nullptr;
    fn_libvlc_video_set_spu_delay video_set_spu_delay = nullptr;
    fn_libvlc_track_description_list_release track_description_list_release = nullptr;
    fn_libvlc_media_player_get_chapter_count get_chapter_count = nullptr;
    fn_libvlc_media_player_get_chapter get_chapter = nullptr;
    fn_libvlc_media_player_set_chapter set_chapter = nullptr;
    fn_libvlc_media_player_get_title get_title = nullptr;
    fn_libvlc_media_player_get_full_chapter_descriptions get_full_chapter_descriptions = nullptr;
    fn_libvlc_chapter_descriptions_release chapter_descriptions_release = nullptr;

    bool load(std::string& err) {
        const char* names[] = {"libvlc.so.5", "libvlc.so", nullptr};
        for (int i=0; names[i]; ++i) {
            handle = dlopen(names[i], RTLD_NOW);
            if (handle) break;
        }
        if (!handle) { err = "VLC/libVLC was not found. Install VLC, then reopen Nougat Media Suite."; return false; }
#define LOAD_SYM(field, name) do { field = (decltype(field))dlsym(handle, name); if (!field) { err = std::string("Missing libVLC symbol: ") + name; return false; } } while(0)
        LOAD_SYM(new_, "libvlc_new");
        LOAD_SYM(release, "libvlc_release");
        LOAD_SYM(media_new_path, "libvlc_media_new_path");
        LOAD_SYM(media_new_location, "libvlc_media_new_location");
        LOAD_SYM(media_release, "libvlc_media_release");
        LOAD_SYM(media_add_option, "libvlc_media_add_option");
        LOAD_SYM(player_new_from_media, "libvlc_media_player_new_from_media");
        LOAD_SYM(player_release, "libvlc_media_player_release");
        LOAD_SYM(play, "libvlc_media_player_play");
        LOAD_SYM(set_pause, "libvlc_media_player_set_pause");
        LOAD_SYM(stop, "libvlc_media_player_stop");
        LOAD_SYM(set_xwindow, "libvlc_media_player_set_xwindow");
        LOAD_SYM(get_time, "libvlc_media_player_get_time");
        LOAD_SYM(set_time, "libvlc_media_player_set_time");
        LOAD_SYM(get_length, "libvlc_media_player_get_length");
        LOAD_SYM(get_volume, "libvlc_audio_get_volume");
        LOAD_SYM(set_volume, "libvlc_audio_set_volume");
#undef LOAD_SYM
#define LOAD_OPTIONAL(field, name) do { field = (decltype(field))dlsym(handle, name); } while(0)
        LOAD_OPTIONAL(get_state, "libvlc_media_player_get_state");
        LOAD_OPTIONAL(get_version, "libvlc_get_version");
        LOAD_OPTIONAL(audio_get_track_description, "libvlc_audio_get_track_description");
        LOAD_OPTIONAL(audio_get_track, "libvlc_audio_get_track");
        LOAD_OPTIONAL(audio_set_track, "libvlc_audio_set_track");
        LOAD_OPTIONAL(video_get_spu_description, "libvlc_video_get_spu_description");
        LOAD_OPTIONAL(video_get_spu, "libvlc_video_get_spu");
        LOAD_OPTIONAL(video_set_spu, "libvlc_video_set_spu");
        LOAD_OPTIONAL(video_set_subtitle_file, "libvlc_video_set_subtitle_file");
        LOAD_OPTIONAL(video_get_spu_delay, "libvlc_video_get_spu_delay");
        LOAD_OPTIONAL(video_set_spu_delay, "libvlc_video_set_spu_delay");
        LOAD_OPTIONAL(track_description_list_release, "libvlc_track_description_list_release");
        LOAD_OPTIONAL(get_chapter_count, "libvlc_media_player_get_chapter_count");
        LOAD_OPTIONAL(get_chapter, "libvlc_media_player_get_chapter");
        LOAD_OPTIONAL(set_chapter, "libvlc_media_player_set_chapter");
        LOAD_OPTIONAL(get_title, "libvlc_media_player_get_title");
        LOAD_OPTIONAL(get_full_chapter_descriptions, "libvlc_media_player_get_full_chapter_descriptions");
        LOAD_OPTIONAL(chapter_descriptions_release, "libvlc_chapter_descriptions_release");
#undef LOAD_OPTIONAL
        return true;
    }
};

struct Rect { int x=0,y=0,w=0,h=0; bool contains(int px,int py) const { return px>=x && py>=y && px<x+w && py<y+h; } };
struct LibraryGridMetrics {
    int columns = 1;
    int rows = 1;
    int tileWidth = 140;
    int tileHeight = 190;
    int posterHeight = 140;
    int gap = 8;
    int visibleItems = 1;
};

static std::string home_dir() {
    const char* h = getenv("HOME"); return h ? std::string(h) : std::string(".");
}
static bool exists_file(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode); }
static std::string config_dir() { return home_dir() + "/.config/reddmedia"; }
static std::string session_file() { return config_dir() + "/session.json"; }
static void ensure_config_dir() {
    std::string base = home_dir() + "/.config";
    mkdir(base.c_str(), 0755);
    mkdir(config_dir().c_str(), 0755);
}
static std::string basename_only(const std::string& path) {
    size_t p = path.find_last_of('/'); return p == std::string::npos ? path : path.substr(p+1);
}
static std::string dirname_only(const std::string& path) {
    size_t p = path.find_last_of('/'); return p == std::string::npos ? std::string(".") : path.substr(0,p);
}
static std::string stem_only(const std::string& path) {
    std::string b = basename_only(path);
    size_t p = b.find_last_of('.');
    return p == std::string::npos ? b : b.substr(0,p);
}
static std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}
static bool ends_with_lower(const std::string& s, const std::string& ending) {
    std::string a = lower_copy(s);
    return a.size() >= ending.size() && a.substr(a.size()-ending.size()) == ending;
}
static bool is_playable_video_path(const std::string& path) {
    static const char* extensions[] = {
        ".mkv", ".mp4", ".m4v", ".avi", ".mov", ".webm", ".mpg", ".mpeg",
        ".ts", ".m2ts", ".wmv", ".flv"
    };
    for (const char* extension : extensions) if (ends_with_lower(path, extension)) return true;
    return false;
}

static bool parse_episode_code(const std::string& path, int& season, int& episode) {
    const std::string text = lower_copy(stem_only(path));
    season = 0;
    episode = 0;
    for (std::size_t i = 0; i + 3 < text.size(); ++i) {
        if (text[i] != 's' || !std::isdigit(static_cast<unsigned char>(text[i + 1]))) continue;
        std::size_t pos = i + 1;
        int parsed_season = 0;
        int season_digits = 0;
        while (pos < text.size() && season_digits < 3 && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            parsed_season = parsed_season * 10 + (text[pos] - '0');
            ++pos; ++season_digits;
        }
        if (season_digits == 0 || pos >= text.size() || text[pos] != 'e') continue;
        ++pos;
        int parsed_episode = 0;
        int episode_digits = 0;
        while (pos < text.size() && episode_digits < 3 && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            parsed_episode = parsed_episode * 10 + (text[pos] - '0');
            ++pos; ++episode_digits;
        }
        if (episode_digits > 0 && parsed_episode > 0) {
            season = parsed_season;
            episode = parsed_episode;
            return true;
        }
    }
    // Common alternate form: 1x13, 02x04, etc.
    for (std::size_t i = 0; i + 2 < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) continue;
        std::size_t start = i;
        std::size_t pos = i;
        int parsed_season = 0;
        int season_digits = 0;
        while (pos < text.size() && season_digits < 3 && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            parsed_season = parsed_season * 10 + (text[pos] - '0');
            ++pos; ++season_digits;
        }
        if (season_digits == 0 || pos >= text.size() || text[pos] != 'x') { i = start; continue; }
        ++pos;
        int parsed_episode = 0;
        int episode_digits = 0;
        while (pos < text.size() && episode_digits < 3 && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            parsed_episode = parsed_episode * 10 + (text[pos] - '0');
            ++pos; ++episode_digits;
        }
        if (episode_digits > 0 && parsed_episode > 0) {
            season = parsed_season;
            episode = parsed_episode;
            return true;
        }
    }
    return false;
}

static bool natural_filename_less(const std::string& left_path, const std::string& right_path) {
    const std::string left = lower_copy(basename_only(left_path));
    const std::string right = lower_copy(basename_only(right_path));
    std::size_t a = 0, b = 0;
    while (a < left.size() && b < right.size()) {
        const bool ad = std::isdigit(static_cast<unsigned char>(left[a]));
        const bool bd = std::isdigit(static_cast<unsigned char>(right[b]));
        if (ad && bd) {
            std::size_t ae = a, be = b;
            while (ae < left.size() && std::isdigit(static_cast<unsigned char>(left[ae]))) ++ae;
            while (be < right.size() && std::isdigit(static_cast<unsigned char>(right[be]))) ++be;
            std::string an = left.substr(a, ae - a);
            std::string bn = right.substr(b, be - b);
            std::size_t anz = an.find_first_not_of('0');
            std::size_t bnz = bn.find_first_not_of('0');
            const std::string ac = anz == std::string::npos ? "0" : an.substr(anz);
            const std::string bc = bnz == std::string::npos ? "0" : bn.substr(bnz);
            if (ac.size() != bc.size()) return ac.size() < bc.size();
            if (ac != bc) return ac < bc;
            if (an.size() != bn.size()) return an.size() < bn.size();
            a = ae; b = be; continue;
        }
        if (left[a] != right[b]) return left[a] < right[b];
        ++a; ++b;
    }
    return left.size() < right.size();
}
static std::string json_escape(const std::string& s) {
    std::ostringstream o;
    for(char c: s) {
        if (c == '\\' || c == '"') o << '\\' << c;
        else if (c == '\n') o << "\\n";
        else o << c;
    }
    return o.str();
}
static std::string json_value_string(const std::string& text, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t k = text.find(pat); if (k == std::string::npos) return "";
    size_t colon = text.find(':', k); if (colon == std::string::npos) return "";
    size_t q = text.find('"', colon+1); if (q == std::string::npos) return "";
    std::string out; bool esc=false;
    for (size_t i=q+1; i<text.size(); ++i) {
        char c = text[i];
        if (esc) { out.push_back(c); esc=false; continue; }
        if (c == '\\') { esc=true; continue; }
        if (c == '"') break;
        out.push_back(c);
    }
    return out;
}
static long long json_value_number(const std::string& text, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t k = text.find(pat); if (k == std::string::npos) return 0;
    size_t colon = text.find(':', k); if (colon == std::string::npos) return 0;
    size_t start = text.find_first_of("-0123456789", colon+1); if (start == std::string::npos) return 0;
    size_t end = text.find_first_not_of("0123456789-", start);
    return atoll(text.substr(start, end-start).c_str());
}

static std::string run_command_capture(const std::string& cmd) {
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return "";
    char buf[4096]; std::string out;
    while (fgets(buf, sizeof(buf), f)) out += buf;
    pclose(f);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}
static std::string shell_quote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\"'\"'";
        else out.push_back(c);
    }
    out += "'";
    return out;
}
static bool run_binary_command(const std::string& command, std::string& output) {
    output.clear();
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    char buffer[16384];
    for (;;) {
        const std::size_t amount = std::fread(buffer, 1, sizeof(buffer), pipe);
        if (amount > 0U) output.append(buffer, amount);
        if (amount < sizeof(buffer)) break;
    }
    const int status = pclose(pipe);
    return status == 0 && !output.empty();
}

static bool extract_video_frame_bmp(const std::string& path,
                                    long long position_ms,
                                    int width,
                                    int height,
                                    std::string& bytes) {
    if (path.empty() || !exists_file(path)) return false;
    width = std::max(96, std::min(640, width));
    height = std::max(54, std::min(360, height));
    const double seconds = std::max<long long>(0, position_ms) / 1000.0;
    std::ostringstream command;
    command << "timeout 6 ffmpeg -nostdin -v error -ss " << std::fixed << std::setprecision(3)
            << seconds << " -i " << shell_quote(path)
            << " -frames:v 1 -vf "
            << shell_quote("scale=" + std::to_string(width) + ":" + std::to_string(height) +
                           ":force_original_aspect_ratio=increase,crop=" +
                           std::to_string(width) + ":" + std::to_string(height))
            << " -f image2pipe -vcodec bmp pipe:1 2>/dev/null";
    return run_binary_command(command.str(), bytes);
}

static std::string resolved_executable_path() {
    char buf[PATH_MAX];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (n <= 0) return "Unknown";
    buf[n] = 0;
    return std::string(buf);
}
static std::string choose_file_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --title='Open Media' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askopenfilename(title='Open Media'); print(p if p else '')\" 2>/dev/null";
    p = run_command_capture(py);
    return p;
}
static std::string choose_subtitle_file_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --title='Load Subtitle File' --file-filter='Subtitle files | *.srt *.SRT' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askopenfilename(title='Load Subtitle File', filetypes=[('Subtitle files','*.srt'),('All files','*')]); print(p if p else '')\" 2>/dev/null";
    return run_command_capture(py);
}
static std::string choose_folder_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --directory --title='Open Subtitle Folder' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askdirectory(title='Open Subtitle Folder'); print(p if p else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string choose_media_library_folder_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --directory --title='Add Media Folder' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askdirectory(title='Add Media Folder'); print(p if p else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string choose_tmdb_credential_dialog() {
    std::string credential = run_command_capture(
        "command -v zenity >/dev/null 2>&1 && "
        "zenity --entry --hide-text --title='Nougat Media Suite External Recommendations' "
        "--text='Enter a TMDb API key or read access token' 2>/dev/null");
    if (!credential.empty()) return credential;
    const std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import simpledialog; "
        "root=tk.Tk(); root.withdraw(); "
        "v=simpledialog.askstring('Nougat Media Suite External Recommendations',"
        "'Enter a TMDb API key or read access token',show='*'); print(v if v else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string choose_security_auth_key_dialog() {
    std::string credential = run_command_capture(
        "command -v zenity >/dev/null 2>&1 && "
        "zenity --entry --hide-text --title='Nougat Security Analysis' "
        "--text='Enter your free abuse.ch Community Auth-Key (leave blank to clear)' 2>/dev/null");
    if (!credential.empty()) return credential;
    const std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import simpledialog; "
        "root=tk.Tk(); root.withdraw(); "
        "v=simpledialog.askstring('Nougat Security Analysis',"
        "'Enter your free abuse.ch Community Auth-Key (leave blank to clear)',show='*'); print(v if v else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string choose_torrent_file_dialog() {
    std::string p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --title='Open P2P File' --file-filter='P2P files | *.torrent' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askopenfilename(title='Open P2P File', filetypes=[('P2P files','*.torrent'),('All files','*')]); print(p if p else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string choose_p2p_folder_dialog() {
    std::string p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --directory --title='P2P Download Folder' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askdirectory(title='P2P Download Folder'); print(p if p else '')\" 2>/dev/null";
    return run_command_capture(py);
}

static std::string exe_dir() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = 0;
        std::string p(buf);
        return dirname_only(p);
    }
    return ".";
}
static std::string read_clipboard_text() {
    const char* cmds[] = {
        "timeout 1 bash -lc 'command -v xclip >/dev/null 2>&1 && xclip -selection clipboard -o 2>/dev/null'",
        "timeout 1 bash -lc 'command -v xsel >/dev/null 2>&1 && xsel --clipboard --output 2>/dev/null'",
        "timeout 1 bash -lc 'command -v wl-paste >/dev/null 2>&1 && wl-paste -n 2>/dev/null'",
        nullptr
    };
    for (int i=0; cmds[i]; ++i) {
        std::string out = run_command_capture(cmds[i]);
        if (!out.empty()) return out;
    }
    return "";
}

static long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static std::string format_time(long long ms) {
    if (ms < 0) ms = 0;
    long long sec = ms / 1000;
    long long h = sec / 3600; long long m = (sec % 3600) / 60; long long s = sec % 60;
    char b[64];
    if (h > 0) snprintf(b, sizeof(b), "%lld:%02lld:%02lld", h, m, s);
    else snprintf(b, sizeof(b), "%lld:%02lld", m, s);
    return b;
}

enum class MenuAction {
    NoAction, OpenFile, ExitApp, TogglePlay, ToggleFullscreen, Rewind10, Forward10,
    SubtitleToggle, SubtitleLoadFile, SubtitleLoadFolder, SubtitleDelayPlus, SubtitleDelayMinus, SubtitleDelayReset, SubtitleTrack,
    AudioTrack, PrevChapter, NextChapter, ChapterJump, YtDlpClearLog, UrlCut, UrlCopy, UrlPaste,
    P2pUrlCut, P2pUrlCopy, P2pUrlPaste,
    NougatCopySelection, NougatSelectAll
};
enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, Nougat, Stream, Studio, P2P, Debug };
enum class NougatPanel { Search, Crawler, P2P, VirusScan };
enum class StreamPlatform { YouTube, Vimeo, Rumble, RuTube, VK, OK };
enum class LibraryDisplayMode { Grid, List };
enum class NougatInputFocus { NoFocus, Search, CrawlSeed, Peer };
enum class YtDlpJob { Idle, Download };
struct NavigationSnapshot {
    ViewMode view = ViewMode::VideoPlayer;
    bool library_type_chosen = false;
    reddmedia::LibraryMediaType library_media_type = reddmedia::LibraryMediaType::Movies;
    std::vector<reddmedia::LibraryNode> library_parents;
    int library_selected = -1;
    int library_scroll = 0;
    bool discover_service_settings = false;
    reddmedia::RecommendationMode discover_mode = reddmedia::RecommendationMode::Usual;
    reddmedia::RecommendationSource discover_source = reddmedia::RecommendationSource::Local;
    reddmedia::RecommendationMediaType discover_media_type = reddmedia::RecommendationMediaType::Movie;
    bool discover_target_selected = false;
    NougatPanel nougat_panel = NougatPanel::Search;
    bool nougat_network_advanced = false;
    StreamPlatform stream_platform = StreamPlatform::YouTube;
};
struct MenuItem {
    std::string label;
    MenuAction action = MenuAction::NoAction;
    int value = 0;
};
struct TrackChoice { int id = -1; std::string name; };
struct LibraryUiState {
    std::mutex mutex;
    std::vector<reddmedia::MediaFolder> folders;
    std::vector<reddmedia::LibraryNode> nodes;
    std::string status = "Choose Movies or TV.";
    bool busy = false;
    bool updated = false;
    double progress = 0.0;
    bool progress_determinate = false;
    std::string progress_label;
};
struct DiscoverUiState {
    std::mutex mutex;
    reddmedia::RecommendationResult result;
    std::string status = "Choose one recommendation option.";
    bool hasResult = false;
    bool busy = false;
    bool updated = false;
    double progress = 0.0;
    reddmedia::LibraryPoster poster;
    bool hasPoster = false;
    reddmedia::WatchAvailability availability;
    bool hasAvailability = false;
    std::string availabilityStatus;
    std::vector<reddmedia::WatchProvider> providerCatalog;
    bool providerCatalogLoaded = false;
};
struct PosterUiState {
    std::mutex mutex;
    std::map<std::string, reddmedia::LibraryPoster> cache;
    std::set<std::string> failed;
    bool busy = false;
    bool updated = false;
    double progress = 0.0;
    bool progress_determinate = false;
    std::string progress_label;
};
struct ServerUiState {
    std::mutex mutex;
    std::string status = "Server controls ready.";
    reddmedia::MediaServerState state = reddmedia::MediaServerState::Stopped;
    bool owned = false;
    bool busy = false;
    bool updated = false;
    double progress = 0.0;
    bool progress_determinate = false;
    std::string progress_label;
};
struct DebugUiState {
    std::mutex mutex;
    reddmedia::DiagnosticInput input;
    reddmedia::DiagnosticReport report;
    std::string status = "Run Checks to inspect Nougat Media Suite.";
    bool busy = false;
    bool updated = false;
    bool hasReport = false;
    double progress = 0.0;
};
struct SecurityUiState {
    std::mutex mutex;
    bool busy = false;
    bool updated = false;
    bool folder = false;
    std::string target;
    std::string status = "Ready. Choose a file or folder to scan.";
    std::string verdict = "NOT SCANNED";
    std::string report = "No scan has been run yet.";
};
struct LiveTvScanUiState {
    std::mutex mutex;
    bool busy = false;
    bool updated = false;
    bool cancel = false;
    bool finished = false;
    bool success = false;
    int physical_channel = 0;
    unsigned frequency_hz = 0;
    int completed = 0;
    int total = 35;
    bool locked = false;
    int signal_percent = -1;
    int quality_percent = -1;
    int channels_found = 0;
    std::string status = "Ready to scan ATSC channels.";
    std::vector<reddmedia::LiveTvChannel> channels;
};
struct LiveTvGuideUiState {
    std::mutex mutex;
    bool busy = false;
    bool updated = false;
    bool cancel = false;
    bool finished = false;
    bool success = false;
    int completed = 0;
    int total = 0;
    int programs_found = 0;
    std::string status = "Broadcast guide cache ready.";
    std::vector<reddmedia::LiveTvChannel> channels;
    std::vector<reddmedia::LiveTvProgram> programs;
};
struct LiveTvHitbox {
    Rect rect;
    int channel_index = -1;
};

enum class LiveTvTunerUse { Idle, Scanning, GuideRefreshing, Watching };

struct NougatUiState {
    std::mutex mutex;
    reddmedia::NougatSearchResponse search;
    std::vector<std::string> crawl_log;
    std::vector<std::string> peers;
    std::string status = "Ready. Crawl a site or add a peer, then search.";
    std::string node_id;
    bool search_busy = false;
    bool crawl_busy = false;
    bool updated = false;
};
struct NougatResultHitboxes {
    Rect card;
    Rect open;
    Rect open_tor;
    Rect copy_url;
    int index = -1;
};
struct ResumeRecord {
    std::string path;
    std::string item_id;
    std::string title;
    std::string series_name;
    std::string episode_title;
    std::string tmdb_id;
    std::string series_id;
    std::string series_tmdb_id;
    std::string primary_image_tag;
    std::string backdrop_image_tag;
    int kind = static_cast<int>(reddmedia::LibraryNodeKind::Movie);
    int production_year = 0;
    int season_number = 0;
    int episode_number = 0;
    long long position_ms = 0;
    long long duration_ms = 0;
    long long last_watched = 0;
    bool completed = false;
};

class PlaybackResumeStore {
public:
    PlaybackResumeStore() {
        const char* home = std::getenv("HOME");
        path_ = std::string(home ? home : ".") + "/.config/reddmedia/playback_resume.tsv";
        load();
    }

    bool find(const std::string& path, ResumeRecord& record) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = records_.find(path);
        if (found == records_.end()) return false;
        record = found->second;
        return true;
    }

    std::vector<ResumeRecord> unfinished() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ResumeRecord> result;
        for (const auto& entry : records_) {
            const ResumeRecord& record = entry.second;
            if (record.completed || record.path.empty() || !exists_file(record.path)) continue;
            if (record.position_ms < 30000) continue;
            if (record.duration_ms > 0 && record.position_ms >= std::max<long long>(0, record.duration_ms - 30000)) continue;
            result.push_back(record);
        }
        std::sort(result.begin(), result.end(), [](const ResumeRecord& a, const ResumeRecord& b) {
            return a.last_watched > b.last_watched;
        });
        return result;
    }

    void update(const ResumeRecord& record) {
        if (record.path.empty()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            records_[record.path] = record;
        }
        save();
    }

    void mark_completed(const std::string& path) {
        if (path.empty()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto found = records_.find(path);
            if (found == records_.end()) return;
            found->second.completed = true;
            found->second.position_ms = found->second.duration_ms;
            found->second.last_watched = static_cast<long long>(std::time(nullptr));
        }
        save();
    }

    void clear_position(const std::string& path) {
        if (path.empty()) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto found = records_.find(path);
            if (found == records_.end()) return;
            found->second.position_ms = 0;
            found->second.completed = false;
            found->second.last_watched = static_cast<long long>(std::time(nullptr));
        }
        save();
    }

private:
    void load() {
        std::ifstream input(path_);
        if (!input) return;
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) continue;
            std::istringstream in(line);
            ResumeRecord record;
            int completed = 0;
            if (!(in >> std::quoted(record.path) >> std::quoted(record.item_id) >> std::quoted(record.title)
                    >> std::quoted(record.series_name) >> std::quoted(record.episode_title)
                    >> std::quoted(record.tmdb_id) >> std::quoted(record.series_id) >> std::quoted(record.series_tmdb_id)
                    >> std::quoted(record.primary_image_tag) >> std::quoted(record.backdrop_image_tag) >> record.kind >> record.production_year
                    >> record.season_number >> record.episode_number >> record.position_ms
                    >> record.duration_ms >> record.last_watched >> completed)) continue;
            record.completed = completed != 0;
            if (!record.path.empty()) records_[record.path] = std::move(record);
        }
    }

    void save() const {
        ensure_config_dir();
        const std::string temporary = path_ + ".tmp";
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& entry : records_) {
                const ResumeRecord& record = entry.second;
                output << std::quoted(record.path) << '\t' << std::quoted(record.item_id) << '\t'
                       << std::quoted(record.title) << '\t' << std::quoted(record.series_name) << '\t'
                       << std::quoted(record.episode_title) << '\t' << std::quoted(record.tmdb_id) << '\t'
                       << std::quoted(record.series_id) << '\t' << std::quoted(record.series_tmdb_id) << '\t'
                       << std::quoted(record.primary_image_tag) << '\t' << std::quoted(record.backdrop_image_tag) << '\t'
                       << record.kind << '\t' << record.production_year << '\t' << record.season_number << '\t'
                       << record.episode_number << '\t' << record.position_ms << '\t' << record.duration_ms << '\t'
                       << record.last_watched << '\t' << (record.completed ? 1 : 0) << '\n';
            }
        }
        output.close();
        chmod(temporary.c_str(), 0600);
        rename(temporary.c_str(), path_.c_str());
        chmod(path_.c_str(), 0600);
    }

    std::string path_;
    mutable std::mutex mutex_;
    std::map<std::string, ResumeRecord> records_;
};

struct HomeSection {
    std::string title;
    std::vector<reddmedia::LibraryNode> items;
};

struct HomeUiState {
    std::mutex mutex;
    std::vector<ResumeRecord> continue_watching;
    std::vector<HomeSection> sections;
    std::map<std::string, reddmedia::LibraryPoster> artwork;
    std::map<std::string, reddmedia::LibraryNode> continue_artwork_nodes;
    std::set<std::string> artwork_failed;
    std::string status = "Loading your local Home feed...";
    bool busy = false;
    bool updated = false;
    bool loaded = false;
    double progress = 0.0;
};

struct HomeCardHitbox {
    Rect card;
    reddmedia::LibraryNode node;
    bool continue_watching = false;
    long long resume_ms = 0;
};

struct FramePreviewState {
    std::mutex mutex;
    reddmedia::LibraryPoster frame;
    std::map<long long, reddmedia::LibraryPoster> cache;
    std::string path;
    long long target_ms = 0;
    long long next_request_ms = 0;
    int generation = 0;
    bool busy = false;
    bool has_frame = false;
    bool updated = false;
};
class App {
public:
    Display* d=nullptr; int screen=0; Window win=0, video=0, seekPreviewWindow=0; GC gc=0; XFontStruct* fontInfo=nullptr; XFontStruct* sectionFontInfo=nullptr; XFontStruct* metadataFontInfo=nullptr;
    Pixmap quiltTiles[10] = {};
    Pixmap streamQuiltTiles[6] = {};
    int W=1000,H=650;
    int videoW=980, videoH=530;
    Rect openBtn, rewindBtn, previousBtn, playBtn, nextBtn, forwardBtn, stopBtn, fsBtn, seekRect, volRect, volumeHousingRect, resumeBtn, loadBtn;
    Rect videoResumeBtn, videoLoadBtn, videoRestartBtn, videoCancelBtn, videoBackLibraryBtn;
    Rect videoUpNextPlayBtn, videoUpNextSeriesBtn, videoUpNextReplayBtn;
    Rect homeTab, videoPlayerTab, libraryTab, discoverTab, liveTvTab, nougatTab, ytdlpTab, studioTab, debugTab;
    Rect libraryMoviesBtn, libraryTvBtn, libraryGridBtn, libraryListViewBtn, libraryAddFolderBtn, libraryUnlinkFolderBtn;
    Rect libraryRefreshBtn, libraryBackBtn, librarySearchRect, librarySearchBtn, libraryListBox;
    Rect libraryVerticalScrollTrack, libraryVerticalScrollThumb;
    Rect serverStartBtn, serverStopBtn, serverRefreshBtn;
    Rect discoverUsualTab, discoverRandomTab;
    Rect discoverLocalMovieBtn, discoverLocalTvBtn, discoverLiveTvBtn, discoverExternalMovieBtn, discoverExternalTvBtn;
    Rect discoverTmdbTestBtn, discoverTmdbReplaceBtn, discoverTmdbClearBtn;
    Rect discoverResultBox, discoverOpenBtn, discoverWatchBtn, discoverMyServicesBtn;
    Rect discoverServicesBackBtn;
    Rect debugRunBtn, debugRetryBtn, debugMetadataBtn, debugTmdbBtn;
    Rect debugServerBtn, debugLogsBtn, debugCopyBtn, debugExportTextBtn, debugExportJsonBtn, debugBundleBtn, debugListBox;
    Rect streamYoutubeTab, streamVimeoTab, streamRumbleTab, streamRutubeTab, streamVkTab, streamOkTab;
    Rect ytdlpUrlRect, ytdlpOutputRect, ytdlpDownloadBtn, ytdlpDirectWatchBtn, ytdlpWebpageBtn, ytdlpClearBtn, ytdlpFolderBtn;
    Rect p2pMagnetRect, p2pOutputRect, p2pLoadMagnetBtn, p2pOpenTorrentBtn, p2pPlayBtn, p2pStopResumeBtn, p2pRemoveBtn;
    Rect p2pSpeedBtn, p2pSeedRulesBtn, p2pQueueUpBtn, p2pQueueDownBtn, p2pReannounceBtn, p2pRecheckBtn, p2pPriorityBtn;
    Rect nougatSearchPanelTab, nougatCrawlerPanelTab, nougatP2PPanelTab, nougatVirusScanPanelTab, nougatNetworkAdvancedBtn;
    Rect securityScanFileBtn, securityScanFolderBtn, securityScanAgainBtn, securityCommunityKeyBtn, securityHistoryBtn, securityResultsBox;
    Rect nougatSearchRect, nougatSearchBtn, nougatRawBtn, nougatPeersToggleBtn, nougatResultsBox;
    Rect nougatCrawlSeedRect, nougatCrawlMinusBtn, nougatCrawlPlusBtn, nougatSameDomainBtn, nougatStartCrawlBtn, nougatCrawlLogBox;
    Rect nougatPeerEntryRect, nougatAddPeerBtn, nougatRemovePeerBtn, nougatNodeBtn, nougatPeerListBox;
    Rect liveTvDetectBtn, liveTvRefreshBtn, liveTvScanBtn, liveTvWatchBtn, liveTvChannelsBtn, liveTvGuideBtn, liveTvGuideRefreshBtn, liveTvNowBtn, liveTvRecordBtn, liveTvListBox;
    VlcApi api; std::string vlcErr;
    libvlc_instance_t* inst=nullptr; libvlc_media_player_t* mp=nullptr;
    bool running=true, paused=false, fullscreen=false, hasMedia=false, needResumePrompt=false;
    bool shuttingDown=false;
    bool playbackCacheValid=false;
    long long cachedPlaybackTimeMs=0;
    long long cachedPlaybackLengthMs=0;
    bool chapterScanComplete=false;
    std::string currentPath, sessionPath;
    bool currentMediaIsP2P=false;
    bool currentMediaIsNetwork=false;
    bool currentMediaIsLiveTv=false;
    std::string liveTvPlayingLabel;
    long long sessionTime=0;
    Cursor blankCursor=0, normalCursor=0;
    bool pointerInVideo=false, pointerHidden=false;
    int pointerWindowX=-10000, pointerWindowY=-10000;
    long long lastPointerFullRedrawMs=0;
    bool pointerFullRedrawPending=false;
    time_t lastMouse=0;
    Time lastClickTime=0;
    int lastClickX=0, lastClickY=0;
    bool pendingVideoSingleClick=false;
    long long pendingVideoSingleClickDeadlineMs=0;
    Window contextMenu=0;
    bool contextMenuOpen=false;
    std::vector<MenuItem> contextMenuItems;
    int contextMenuW=220;
    int contextMenuH=32;
    bool subtitlesOn=false;
    std::string subtitlePath;
    std::string subtitleFolder;
    long long subtitleDelayUs=0;
    std::vector<long long> chapterMarksMs;
    std::vector<std::string> chapterNames;
    bool chapterMarksAreReal=false;
    bool pendingSeek=false;
    long long pendingSeekMs=0;
    time_t pendingSeekDeadline=0;
    ViewMode currentView = ViewMode::Home;
    bool urlFocused=false;
    bool urlSelectAll=false;
    std::string ytdlpUrl;
    std::string ownedClipboardText;
    Atom clipboardAtom=None;
    Atom utf8Atom=None;
    Atom targetsAtom=None;
    Atom textAtom=None;
    std::string ytdlpOutputFolder = home_dir() + "/Downloads";
    std::string ytdlpStatus = "Ready.";
    std::string ytdlpLog = "No download output yet.";
    pid_t ytdlpPid = -1;
    int ytdlpPipe = -1;
    YtDlpJob ytdlpJob = YtDlpJob::Idle;
    std::string ytdlpProcessOutput;
    YtDlpStreamServer ytdlpStream;
    bool currentMediaIsYtDlpStream = false;
    long long ytdlpStreamBaseMs = 0;
    long long ytdlpTotalDurationMs = 0;
    bool ytdlpSeekBuffering = false;
    long long ytdlpSeekTargetMs = 0;
    long long ytdlpSeekStartedAtMs = 0;
    int controlsScrollX = 0;
    int topNavScrollX = 0;
    int topNavViewportW = 0;
    int topNavClipX = 222;
    int topNavClipRight = 846;
    int libraryButtonsScrollX = 0;
    int discoverButtonsScrollX = 0;
    int ytdlpButtonsScrollX = 0;
    int streamSourceScrollX = 0;
    int nougatPanelButtonsScrollX = 0;
    int liveTvButtonsScrollX = 0;
    int p2pButtonsScrollX = 0;
    int debugButtonsScrollX = 0;
    int volumePercent = 100;
    bool volumeDragging = false;
    // Pixel-derived from the owner-approved VOLUME component in the canonical
    // Nougat sheet. Frame 100 is the actual sheet housing, pixel-for-pixel;
    // the remaining frames are generated from those same sheet pixels.
    static constexpr int kSheetVolumeW = 335;
    static constexpr int kSheetVolumeH = 47;
    static constexpr int kSheetVolumeFrames = 201;
    static constexpr int kSheetSeekW = 378;
    static constexpr int kSheetSeekH = 20;
    static constexpr int kSheetSeekSpriteH = 33;
    static constexpr int kSheetSeekFrames = 101;
    static constexpr int kServerStatusDiameter = 20;
    std::vector<unsigned char> sheetVolumeRgb;
    bool sheetVolumeLoaded = false;
    std::vector<unsigned char> sheetSeekRgba;
    bool sheetSeekLoaded = false;
    StreamPlatform streamPlatform = StreamPlatform::YouTube;
    LibraryDisplayMode libraryMovieView = LibraryDisplayMode::Grid;
    LibraryDisplayMode libraryTvView = LibraryDisplayMode::Grid;
    std::string librarySearchQuery;
    bool librarySearchFocused = false;
    bool librarySearchSelectAll = false;
    bool tvAutoplayArmed = false;
    bool playbackEndHandled = false;
    std::vector<reddmedia::LibraryNode> tvAutoplayQueue;
    int tvAutoplayIndex = -1;
    int tvAutoplayRetryIndex = -1;
    int tvAutoplayRetryAttempts = 0;
    long long tvAutoplayRetryAtMs = 0;
    long long lastLocalPlaybackPositionMs = 0;
    long long lastLocalPlaybackLengthMs = 0;
    bool upNextVisible = false;
    bool upNextHasEpisode = false;
    int upNextTargetIndex = -1;
    long long upNextDeadlineMs = 0;
    int upNextLastDisplayedSeconds = -1;
    std::string upNextMessage;
    reddmedia::LibraryNode upNextEpisode;
    std::vector<NavigationSnapshot> navigationBackStack;
    std::vector<NavigationSnapshot> navigationForwardStack;
    bool navigationRestoring = false;
    reddmedia::LibraryNode activeLibraryItem;
    bool activeLibraryItemValid = false;
    PlaybackResumeStore resumeStore;
    bool resumePromptVisible = false;
    ViewMode resumePromptOrigin = ViewMode::Home;
    ResumeRecord pendingResumeRecord;
    reddmedia::LibraryNode pendingResumeNode;
    bool stoppedPlaybackVisible = false;
    long long stoppedPlaybackPositionMs = 0;
    long long lastResumePersistMs = 0;
    long long lastFullscreenOverlayMotionMs = 0;
    std::shared_ptr<HomeUiState> homeState = std::make_shared<HomeUiState>();
    std::thread homeWorker;
    std::atomic<bool> homeNeedsRefresh{false};
    int homePageScroll = 0;
    int homeContentHeight = 0;
    int homeContinueScrollX = 0;
    int homeContinueMaxScrollX = 0;
    Rect homeContinueArea;
    Rect homeVerticalScrollTrack, homeVerticalScrollThumb;
    Rect homeContinueScrollTrack, homeContinueScrollThumb;
    bool homeVerticalScrollDragging = false;
    bool homeContinueScrollDragging = false;
    bool libraryVerticalScrollDragging = false;
    int homeVerticalScrollDragOffset = 0;
    int homeContinueScrollDragOffset = 0;
    int libraryVerticalScrollDragOffset = 0;
    std::vector<HomeCardHitbox> homeCardHitboxes;
    std::string homeHoveredPath;
    long long homeHoverStartedMs = 0;
    long long homePreviewCursorMs = 0;
    long long homePreviewNextFrameMs = 0;
    std::shared_ptr<FramePreviewState> homePreviewState = std::make_shared<FramePreviewState>();
    bool seekPreviewHover = false;
    long long seekPreviewHoverStartedMs = 0;
    long long seekPreviewTargetMs = 0;
    std::shared_ptr<FramePreviewState> seekPreviewState = std::make_shared<FramePreviewState>();
    void* xextHandle = nullptr;
    using XShapeCombineMaskFn = void (*)(Display*, Window, int, int, int, Pixmap, int);
    XShapeCombineMaskFn xShapeCombineMask = nullptr;
    P2PEngine p2p;
    int p2pSpeedPreset = 0;
    int p2pSeedPreset = 0;
    int p2pPriorityPreset = 4;
    P2PStreamServer p2pStream{p2p};
    reddmedia::MediaServerManager mediaServer;
    reddmedia::NougatTunerBackend tunerBackend;
    std::shared_ptr<LiveTvScanUiState> liveTvScanState = std::make_shared<LiveTvScanUiState>();
    std::thread liveTvScanWorker;
    std::shared_ptr<LiveTvGuideUiState> liveTvGuideState = std::make_shared<LiveTvGuideUiState>();
    std::thread liveTvGuideWorker;
    std::vector<reddmedia::TunerDevice> liveTvTuners;
    std::vector<reddmedia::LiveTvChannel> liveTvChannels;
    std::vector<reddmedia::LiveTvProgram> liveTvPrograms;
    std::vector<LiveTvHitbox> liveTvChannelHitboxes;
    int liveTvSelectedTuner = -1;
    int liveTvSelectedChannel = -1;
    bool liveTvGuideMode = false;
    int liveTvGuideChannelScroll = 0;
    int liveTvGuideTimeOffsetSlots = 0;
    Time liveTvLastClickTime = 0;
    int liveTvLastClickChannel = -1;
    std::string liveTvStatus = "Live TV ready. Detect your tuner to begin.";
    LiveTvTunerUse liveTvTunerUse = LiveTvTunerUse::Idle;
    int liveTvPlayingChannel = -1;
    std::shared_ptr<reddmedia::JellyfinApiClient> libraryClient =
        std::make_shared<reddmedia::JellyfinApiClient>();
    std::shared_ptr<reddmedia::LibraryMetadataCache> libraryMetadataCache =
        std::make_shared<reddmedia::LibraryMetadataCache>();
    std::shared_ptr<LibraryUiState> libraryState = std::make_shared<LibraryUiState>();
    std::thread libraryWorker;
    std::vector<Rect> libraryRows;
    std::vector<int> libraryRowNodeIndices;
    int librarySelected = -1;
    int libraryScroll = 0;
    bool libraryTypeChosen = false;
    reddmedia::LibraryMediaType libraryMediaType = reddmedia::LibraryMediaType::Movies;
    std::vector<reddmedia::LibraryNode> libraryParents;
    std::shared_ptr<PosterUiState> posterState = std::make_shared<PosterUiState>();
    std::thread posterWorker;
    bool posterQueued = false;
    std::shared_ptr<reddmedia::RecommendationEngine> recommendationEngine =
        std::make_shared<reddmedia::RecommendationEngine>(
            exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf");
    reddmedia::WatchProviderPreferences watchPreferences;
    std::shared_ptr<DiscoverUiState> discoverState = std::make_shared<DiscoverUiState>();
    std::thread discoverWorker;
    std::shared_ptr<ServerUiState> serverState = std::make_shared<ServerUiState>();
    std::thread serverWorker;
    reddmedia::DiagnosticEngine diagnosticEngine;
    std::shared_ptr<DebugUiState> debugState = std::make_shared<DebugUiState>();
    std::thread debugWorker;
    reddmedia::RecommendationMode discoverMode = reddmedia::RecommendationMode::Usual;
    reddmedia::RecommendationSource discoverSource = reddmedia::RecommendationSource::Local;
    reddmedia::RecommendationMediaType discoverMediaType = reddmedia::RecommendationMediaType::Movie;
    bool discoverTargetSelected = false;
    bool discoverServiceSettings = false;
    int discoverDetailsScroll = 0;
    int discoverServicesScroll = 0;
    int debugScroll = 0;
    std::vector<std::pair<Rect, int>> discoverProviderRows;
    std::vector<Rect> debugIssueRows;
    bool p2pMagnetFocused=false;
    bool p2pMagnetSelectAll=false;
    std::string p2pMagnet;
    std::string p2pOutputFolder = home_dir() + "/Downloads";
    std::string p2pUiStatus = "Ready.";
    std::vector<Rect> p2pFileRows;
    long long lastP2PRedrawMs=0;
    long long lastLoadingRedrawMs=0;
    reddmedia::NougatBridge nougat{exe_dir() + "/components/nougat/nougat_engine.py"};
    std::shared_ptr<NougatUiState> nougatState = std::make_shared<NougatUiState>();
    std::thread nougatSearchWorker;
    std::thread nougatCrawlWorker;
    std::shared_ptr<SecurityUiState> securityState = std::make_shared<SecurityUiState>();
    std::thread securityWorker;
    int securityScroll = 0;
    std::string lastP2PAutoScanTransfer;
    std::vector<std::string> pendingP2PAutoScanPaths;
    NougatPanel nougatPanel = NougatPanel::Search;
    bool nougatNetworkAdvanced = false;
    NougatInputFocus nougatInputFocus = NougatInputFocus::NoFocus;
    bool nougatInputSelectAll = false;
    std::string nougatSearchQuery;
    std::string nougatCrawlSeed = "https://example.com/";
    std::string nougatPeerEntry;
    bool nougatRaw = false;
    bool nougatSearchPeers = true;
    bool nougatSameDomain = true;
    int nougatMaxPages = 25;
    int nougatSearchOffset = 0;
    int nougatResultScroll = 0;
    int nougatPeerSelected = -1;
    int nougatPeerScroll = 0;
    int nougatCrawlScroll = 0;
    bool nougatOutputFocused = false;
    bool nougatOutputSelecting = false;
    int nougatOutputSelectionStart = -1;
    int nougatOutputSelectionEnd = -1;
    std::vector<NougatResultHitboxes> nougatResultHitboxes;

    unsigned long col(unsigned short r, unsigned short g, unsigned short b) {
        XColor color; color.red=r; color.green=g; color.blue=b; color.flags=DoRed|DoGreen|DoBlue;
        XAllocColor(d, DefaultColormap(d, screen), &color); return color.pixel;
    }
    static std::string x11_safe_text(const std::string& input) {
        // XDrawString consumes the active legacy X11 font encoding rather than UTF-8.
        // Translate the metadata bullet to a single-byte middle dot so the UI never
        // exposes the UTF-8 bytes as mojibake (the reported "a/cents" glyphs).
        std::string output;
        output.reserve(input.size());
        for (std::size_t i = 0; i < input.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(input[i]);
            if (c == 0xE2U && i + 2U < input.size() &&
                static_cast<unsigned char>(input[i + 1U]) == 0x80U &&
                static_cast<unsigned char>(input[i + 2U]) == 0xA2U) {
                output.push_back(static_cast<char>(0xB7U));
                i += 2U;
                continue;
            }
            output.push_back(input[i]);
        }
        return output;
    }
    void text_with_font(Drawable target, int x, int y, const std::string& raw,
                        unsigned long c, XFontStruct* chosen) {
        const std::string s = x11_safe_text(raw);
        XSetForeground(d, gc, c);
        if (chosen) XSetFont(d, gc, chosen->fid);
        XDrawString(d, target, gc, x, y, s.c_str(), static_cast<int>(s.size()));
        if (fontInfo && chosen != fontInfo) XSetFont(d, gc, fontInfo->fid);
    }
    void text(Drawable target, int x, int y, const std::string& s, unsigned long c) {
        text_with_font(target, x, y, s, c, fontInfo);
    }
    void section_text(Drawable target, int x, int y, const std::string& s, unsigned long c) {
        text_with_font(target, x, y, s, c, sectionFontInfo ? sectionFontInfo : fontInfo);
    }
    void metadata_text(Drawable target, int x, int y, const std::string& s, unsigned long c) {
        text_with_font(target, x, y, s, c, metadataFontInfo ? metadataFontInfo : fontInfo);
    }
    int text_width_for_font(const std::string& raw, XFontStruct* chosen) {
        const std::string s = x11_safe_text(raw);
        if (s.empty()) return 0;
        if (chosen) return XTextWidth(chosen, s.c_str(), static_cast<int>(s.size()));
        return static_cast<int>(s.size()) * 8;
    }
    int text_width(const std::string& s) {
        return text_width_for_font(s, fontInfo);
    }
    std::string tail_to_width(const std::string& s, int maxPixels) {
        if (maxPixels <= 0) return "";
        if (text_width(s) <= maxPixels) return s;
        std::string out = s;
        while (!out.empty() && text_width(out) > maxPixels) out.erase(out.begin());
        return out;
    }
    std::string head_to_width(const std::string& value, int maxPixels) {
        if (maxPixels <= 0) return "";
        if (text_width(value) <= maxPixels) return value;
        const std::string ellipsis = "...";
        std::string output = value;
        while (!output.empty() && text_width(output + ellipsis) > maxPixels) output.pop_back();
        return output + ellipsis;
    }
    std::vector<std::string> wrap_text(const std::string& value, int maxPixels) {
        std::vector<std::string> lines;
        if (maxPixels <= 0) return lines;
        std::istringstream paragraphs(value);
        std::string paragraph;
        while (std::getline(paragraphs, paragraph)) {
            std::istringstream words(paragraph);
            std::string word;
            std::string current;
            while (words >> word) {
                if (text_width(word) > maxPixels) {
                    if (!current.empty()) { lines.push_back(current); current.clear(); }
                    while (!word.empty()) {
                        std::string part;
                        while (!word.empty() && text_width(part + word.front()) <= maxPixels) {
                            part.push_back(word.front());
                            word.erase(word.begin());
                        }
                        if (part.empty()) { part.push_back(word.front()); word.erase(word.begin()); }
                        lines.push_back(part);
                    }
                    continue;
                }
                const std::string candidate = current.empty() ? word : current + " " + word;
                if (!current.empty() && text_width(candidate) > maxPixels) {
                    lines.push_back(current);
                    current = word;
                } else current = candidate;
            }
            if (!current.empty()) lines.push_back(current);
            if (paragraph.empty()) lines.emplace_back();
        }
        return lines;
    }
    std::string local_time_text(long long timestamp) {
        if (timestamp <= 0) return "Never";
        const std::time_t value = static_cast<std::time_t>(timestamp);
        std::tm local {};
        localtime_r(&value, &local);
        std::ostringstream output;
        output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        return output.str();
    }
    void fill(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XFillRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void fill_circle(Drawable target, int x, int y, int diameter, unsigned long c) {
        XSetForeground(d, gc, c);
        const double radius = static_cast<double>(diameter) / 2.0;
        const double center_x = static_cast<double>(x) + radius;
        const double center_y = static_cast<double>(y) + radius;
        for (int offset_y = 0; offset_y < diameter; ++offset_y) {
            const double relative_y = static_cast<double>(y + offset_y) + 0.5 - center_y;
            const int half_width = static_cast<int>(
                std::sqrt(std::max(0.0, radius * radius - relative_y * relative_y)));
            XDrawLine(d, target, gc,
                      static_cast<int>(center_x) - half_width, y + offset_y,
                      static_cast<int>(center_x) + half_width, y + offset_y);
        }
    }
    void outline(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XDrawRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void line(Drawable target, int x1, int y1, int x2, int y2, unsigned long c) { XSetForeground(d,gc,c); XDrawLine(d,target,gc,x1,y1,x2,y2); }

    unsigned long rgb8(unsigned char r, unsigned char g, unsigned char b) {
        return col(static_cast<unsigned short>(r) * 257U,
                   static_cast<unsigned short>(g) * 257U,
                   static_cast<unsigned short>(b) * 257U);
    }

    void fill_round(Drawable target, const Rect& raw, int radius, unsigned long c) {
        if (raw.w <= 0 || raw.h <= 0) return;
        Rect r = raw;
        radius = std::max(0, std::min(radius, std::min(r.w, r.h) / 2));
        XSetForeground(d, gc, c);
        if (radius <= 0) {
            XFillRectangle(d, target, gc, r.x, r.y, r.w, r.h);
            return;
        }
        XFillRectangle(d, target, gc, r.x + radius, r.y, std::max(0, r.w - 2 * radius), r.h);
        XFillRectangle(d, target, gc, r.x, r.y + radius, r.w, std::max(0, r.h - 2 * radius));
        const int diameter = radius * 2;
        XFillArc(d, target, gc, r.x, r.y, diameter, diameter, 90 * 64, 90 * 64);
        XFillArc(d, target, gc, r.x + r.w - diameter, r.y, diameter, diameter, 0, 90 * 64);
        XFillArc(d, target, gc, r.x, r.y + r.h - diameter, diameter, diameter, 180 * 64, 90 * 64);
        XFillArc(d, target, gc, r.x + r.w - diameter, r.y + r.h - diameter, diameter, diameter, 270 * 64, 90 * 64);
    }

    void outline_round(Drawable target, const Rect& raw, int radius, unsigned long c) {
        if (raw.w <= 1 || raw.h <= 1) return;
        Rect r = raw;
        radius = std::max(0, std::min(radius, std::min(r.w, r.h) / 2));
        XSetForeground(d, gc, c);
        if (radius <= 0) {
            XDrawRectangle(d, target, gc, r.x, r.y, r.w - 1, r.h - 1);
            return;
        }
        const int diameter = radius * 2;
        XDrawLine(d, target, gc, r.x + radius, r.y, r.x + r.w - radius - 1, r.y);
        XDrawLine(d, target, gc, r.x + radius, r.y + r.h - 1, r.x + r.w - radius - 1, r.y + r.h - 1);
        XDrawLine(d, target, gc, r.x, r.y + radius, r.x, r.y + r.h - radius - 1);
        XDrawLine(d, target, gc, r.x + r.w - 1, r.y + radius, r.x + r.w - 1, r.y + r.h - radius - 1);
        XDrawArc(d, target, gc, r.x, r.y, diameter, diameter, 90 * 64, 90 * 64);
        XDrawArc(d, target, gc, r.x + r.w - diameter - 1, r.y, diameter, diameter, 0, 90 * 64);
        XDrawArc(d, target, gc, r.x, r.y + r.h - diameter - 1, diameter, diameter, 180 * 64, 90 * 64);
        XDrawArc(d, target, gc, r.x + r.w - diameter - 1, r.y + r.h - diameter - 1, diameter, diameter, 270 * 64, 90 * 64);
    }


    void outline_round_dashed(Drawable target, const Rect& r, int radius, unsigned long c, int dash=2) {
        const int dashSize = std::max(1, dash);
        char pattern[2] = {static_cast<char>(dashSize), static_cast<char>(dashSize)};
        XSetLineAttributes(d, gc, 1, LineOnOffDash, CapButt, JoinRound);
        XSetDashes(d, gc, 0, pattern, 2);
        outline_round(target, r, radius, c);
        XSetLineAttributes(d, gc, 1, LineSolid, CapButt, JoinMiter);
    }


    static int quilt_view_index(ViewMode view) {
        switch (view) {
            case ViewMode::Home: return 0;
            case ViewMode::VideoPlayer: return 1;
            case ViewMode::Library: return 2;
            case ViewMode::Discover: return 3;
            case ViewMode::LiveTV: return 8;
            case ViewMode::Nougat: return 4;
            case ViewMode::Stream: return 5;
            case ViewMode::Studio: return 9;
            case ViewMode::P2P: return 6;
            case ViewMode::Debug: return 7;
        }
        return 4;
    }

    static int stream_platform_index(StreamPlatform platform) {
        switch (platform) {
            case StreamPlatform::YouTube: return 0;
            case StreamPlatform::Vimeo: return 1;
            case StreamPlatform::Rumble: return 2;
            case StreamPlatform::RuTube: return 3;
            case StreamPlatform::VK: return 4;
            case StreamPlatform::OK: return 5;
        }
        return 0;
    }

    void quilt_tint_for(ViewMode view, unsigned char& r, unsigned char& g,
                        unsigned char& b, unsigned& blendPercent,
                        StreamPlatform provider=StreamPlatform::YouTube) const {
        // Exact quilt material comes from the owner-approved concept sheet.
        // The active area dyes that material. Stream is provider-reactive, so
        // the selected service owns the whole Stream quilt tint.
        if (view == ViewMode::Stream) {
            switch (provider) {
                case StreamPlatform::YouTube: r=205; g=76;  b=67;  blendPercent=22; return;
                case StreamPlatform::Vimeo:   r=23;  g=213; b=255; blendPercent=22; return;
                case StreamPlatform::Rumble:  r=128; g=154; b=79;  blendPercent=22; return;
                case StreamPlatform::RuTube:  r=168; g=107; b=178; blendPercent=20; return;
                case StreamPlatform::VK:      r=91;  g=142; b=174; blendPercent=20; return;
                case StreamPlatform::OK:      r=211; g=135; b=48;  blendPercent=22; return;
            }
        }
        switch (view) {
            // v0.0.28: the page background carries the tab identity.  Keep the
            // exact quilt material, but dye it strongly enough that each page
            // reads as its own candy-wrapper color family rather than cream.
            case ViewMode::Home:        r=91;  g=58;  b=134; blendPercent=58; break; // blackberry/grape
            case ViewMode::VideoPlayer: r=91;  g=52;  b=31;  blendPercent=62; break; // cocoa/chocolate
            case ViewMode::Library:     r=77;  g=120; b=61;  blendPercent=56; break; // forest/candy green
            case ViewMode::Discover:    r=158; g=51;  b=68;  blendPercent=56; break; // cherry/wine red
            case ViewMode::LiveTV:      r=37;  g=109; b=126; blendPercent=52; break; // broadcast teal
            case ViewMode::Nougat:      r=241; g=227; b=194; blendPercent=8;  break; // Search stays cream
            case ViewMode::Stream:      r=205; g=76;  b=67;  blendPercent=22; break;
            case ViewMode::Studio:      r=221; g=176; b=70;  blendPercent=48; break; // Gold Studio: true yellow/gold quilt
            case ViewMode::P2P:         r=105; g=160; b=192; blendPercent=17; break;
            case ViewMode::Debug:       r=41;  g=40;  b=48;  blendPercent=70; break; // licorice/charcoal
        }
    }

    Pixmap create_quilt_tile(ViewMode view, StreamPlatform provider=StreamPlatform::YouTube) {
        constexpr int sourceSize = nougat_quilt_texture::kSourceSize;
        constexpr int tileSize = sourceSize * 2;
        const int depth = DefaultDepth(d, screen);
        constexpr int bytesPerPixel = 4;
        char* imageData = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(tileSize) * static_cast<std::size_t>(tileSize),
            static_cast<std::size_t>(bytesPerPixel)));
        if (!imageData) return 0;

        unsigned char tintR=255, tintG=255, tintB=255;
        unsigned blend=0;
        quilt_tint_for(view, tintR, tintG, tintB, blend, provider);
        const unsigned keep = 100U - blend;

        for (int y=0; y<tileSize; ++y) {
            const int sy = y < sourceSize ? y : (tileSize - 1 - y);
            for (int x=0; x<tileSize; ++x) {
                const int sx = x < sourceSize ? x : (tileSize - 1 - x);
                const std::uint32_t argb = nougat_quilt_texture::kSource[sy * sourceSize + sx];
                const unsigned char sr = static_cast<unsigned char>((argb >> 16) & 0xffU);
                const unsigned char sg = static_cast<unsigned char>((argb >> 8) & 0xffU);
                const unsigned char sb = static_cast<unsigned char>(argb & 0xffU);
                const unsigned char rr = static_cast<unsigned char>((static_cast<unsigned>(sr) * keep + static_cast<unsigned>(tintR) * blend) / 100U);
                const unsigned char gg = static_cast<unsigned char>((static_cast<unsigned>(sg) * keep + static_cast<unsigned>(tintG) * blend) / 100U);
                const unsigned char bb = static_cast<unsigned char>((static_cast<unsigned>(sb) * keep + static_cast<unsigned>(tintB) * blend) / 100U);
                const unsigned long pixel = visual_pixel(rr, gg, bb);
                std::memcpy(imageData +
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(tileSize) +
                     static_cast<std::size_t>(x)) * static_cast<std::size_t>(bytesPerPixel),
                    &pixel, static_cast<std::size_t>(bytesPerPixel));
            }
        }

        XImage* image = XCreateImage(d, DefaultVisual(d, screen), depth, ZPixmap, 0,
                                     imageData, tileSize, tileSize, 32, 0);
        if (!image) {
            std::free(imageData);
            return 0;
        }
        Pixmap tile = XCreatePixmap(d, win, tileSize, tileSize, static_cast<unsigned>(depth));
        if (!tile) {
            XDestroyImage(image);
            return 0;
        }
        XPutImage(d, tile, gc, image, 0, 0, 0, 0, tileSize, tileSize);
        XDestroyImage(image);
        return tile;
    }

    void init_quilt_tiles() {
        const ViewMode views[10] = {
            ViewMode::Home, ViewMode::VideoPlayer, ViewMode::Library, ViewMode::Discover,
            ViewMode::Nougat, ViewMode::Stream, ViewMode::P2P, ViewMode::Debug, ViewMode::LiveTV,
            ViewMode::Studio
        };
        for (int i=0; i<10; ++i) quiltTiles[i] = create_quilt_tile(views[i]);
        const StreamPlatform providers[6] = {
            StreamPlatform::YouTube, StreamPlatform::Vimeo, StreamPlatform::Rumble,
            StreamPlatform::RuTube, StreamPlatform::VK, StreamPlatform::OK
        };
        for (int i=0; i<6; ++i) {
            streamQuiltTiles[i] = create_quilt_tile(ViewMode::Stream, providers[i]);
        }
    }

    void free_quilt_tiles() {
        if (!d) return;
        for (Pixmap& tile : quiltTiles) {
            if (tile) XFreePixmap(d, tile);
            tile = 0;
        }
        for (Pixmap& tile : streamQuiltTiles) {
            if (tile) XFreePixmap(d, tile);
            tile = 0;
        }
    }

    void draw_quilted_background(Drawable target, const Rect& area, ViewMode view) {
        if (area.w <= 0 || area.h <= 0) return;
        const Pixmap tile = view == ViewMode::Stream
            ? streamQuiltTiles[stream_platform_index(streamPlatform)]
            : quiltTiles[quilt_view_index(view)];
        if (!tile) {
            // Safe fallback only if X11 could not allocate the exact concept tile.
            unsigned long fallback = palette_for(view).background;
            if (view == ViewMode::Stream) fallback = stream_palette_for(streamPlatform).background;
            fill(target, area, fallback);
            return;
        }
        XSetFillStyle(d, gc, FillTiled);
        XSetTile(d, gc, tile);
        XSetTSOrigin(d, gc, 0, 0);
        XFillRectangle(d, target, gc, area.x, area.y,
                       static_cast<unsigned>(area.w), static_cast<unsigned>(area.h));
        XSetFillStyle(d, gc, FillSolid);
    }

    void draw_concept_field(Drawable target, const Rect& r, unsigned long fillColor,
                            unsigned long borderColor, bool focused=false) {
        // Literal component grammar from the approved sheet: soft drop shadow,
        // dark outer rim, bright raised inner bevel, then an inset seam.
        Rect shadow{r.x, r.y + 3, r.w, r.h};
        fill_round(target, shadow, 8, rgb8(183, 149, 109));
        fill_round(target, r, 8, fillColor);
        const unsigned long rim = focused ? rgb8(179, 108, 42) : borderColor;
        outline_round(target, r, 8, rim);
        Rect bevel{r.x + 2, r.y + 2, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        outline_round(target, bevel, 6, rgb8(255, 244, 224));
        Rect seam{r.x + 4, r.y + 4, std::max(1, r.w - 8), std::max(1, r.h - 8)};
        const bool searchCream = borderColor == rgb8(166, 112, 56);
        const unsigned long seamColor = searchCream
            ? (focused ? rgb8(116, 66, 34) : rgb8(132, 78, 40))
            : (focused ? rgb8(220, 163, 91) : rgb8(225, 205, 177));
        outline_round_dashed(target, seam, 4, seamColor, 2);
        line(target, r.x + 9, r.y + 2, r.x + r.w - 10, r.y + 2, rgb8(255, 249, 236));
        line(target, r.x + 9, r.y + r.h - 2, r.x + r.w - 10, r.y + r.h - 2, rgb8(177, 139, 101));
    }

    void draw_speaker_icon(Drawable target, int x, int y, bool loud, unsigned long c) {
        XSetForeground(d, gc, c);
        XFillRectangle(d, target, gc, x, y + 5, 4, 6);
        XPoint points[3] = {{static_cast<short>(x + 4), static_cast<short>(y + 4)},
                            {static_cast<short>(x + 10), static_cast<short>(y)},
                            {static_cast<short>(x + 10), static_cast<short>(y + 16)}};
        XFillPolygon(d, target, gc, points, 3, Convex, CoordModeOrigin);
        if (loud) {
            XDrawArc(d, target, gc, x + 8, y + 3, 10, 10, -60 * 64, 120 * 64);
            XDrawArc(d, target, gc, x + 7, y, 16, 16, -55 * 64, 110 * 64);
        }
    }
    void button(const Rect& r, const std::string& label) {
        button_on(win, r, label);
    }

    struct ViewPalette {
        unsigned long background;
        unsigned long panel;
        unsigned long field;
        unsigned long border;
        unsigned long text;
        unsigned long muted;
        unsigned long selection;
        unsigned long button;
        unsigned long buttonDark;
        unsigned long buttonLight;
        unsigned long buttonText;
        unsigned long accent;
    };

    ViewPalette stream_palette_for(StreamPlatform platform) {
        // The page stays in the approved pale dusty-blue quilt. Service selection
        // still owns the Stream accents and button treatment.
        const unsigned long background = rgb8(232, 236, 235);
        const unsigned long panel = rgb8(246, 240, 229);
        const unsigned long field = rgb8(252, 247, 239);
        const unsigned long textColor = rgb8(68, 42, 28);
        const unsigned long muted = rgb8(125, 105, 91);
        const unsigned long cream = rgb8(250, 239, 219);
        switch (platform) {
            case StreamPlatform::YouTube:
                return {background, panel, field, rgb8(166,93,79), textColor, muted,
                        rgb8(190,75,67), rgb8(196,82,72), rgb8(120,55,49),
                        rgb8(225,132,119), cream, rgb8(190,75,67)};
            case StreamPlatform::Vimeo:
                return {background, panel, field, rgb8(23,213,255), rgb8(20,26,32), muted,
                        rgb8(23,213,255), rgb8(86,224,255), rgb8(20,26,32),
                        rgb8(145,237,255), rgb8(250,252,253), rgb8(23,213,255)};
            case StreamPlatform::Rumble:
                return {background, panel, field, rgb8(112,126,70), textColor, muted,
                        rgb8(134,151,84), rgb8(144,153,89), rgb8(82,91,48),
                        rgb8(188,196,127), rgb8(48,45,27), rgb8(116,132,65)};
            case StreamPlatform::RuTube:
                return {background, panel, field, rgb8(139,91,144), textColor, muted,
                        rgb8(166,112,171), rgb8(181,143,177), rgb8(111,73,114),
                        rgb8(211,177,210), rgb8(61,42,61), rgb8(149,99,154)};
            case StreamPlatform::VK:
                return {background, panel, field, rgb8(92,122,141), textColor, muted,
                        rgb8(112,146,165), rgb8(133,154,168), rgb8(77,100,117),
                        rgb8(176,194,203), rgb8(44,55,61), rgb8(91,133,157)};
            case StreamPlatform::OK:
                return {background, panel, field, rgb8(172,113,55), textColor, muted,
                        rgb8(201,130,50), rgb8(205,145,73), rgb8(126,76,30),
                        rgb8(232,183,112), rgb8(69,38,18), rgb8(194,120,43)};
        }
        return {background, panel, field, rgb8(161,111,61), textColor, muted,
                rgb8(191,130,61), rgb8(206,161,102), rgb8(116,63,23),
                rgb8(232,194,137), rgb8(66,34,17), rgb8(184,111,43)};
    }

    ViewPalette palette_for(ViewMode view) {
        const unsigned long cream = rgb8(244, 232, 205);
        const unsigned long darkText = rgb8(54, 36, 28);
        if (view == ViewMode::Home) return {
            rgb8(91,58,134), rgb8(112,74,151), rgb8(244,232,205),
            rgb8(58,35,88), cream, rgb8(220,202,230),
            rgb8(128,91,168), rgb8(108,72,148), rgb8(57,34,86),
            rgb8(145,105,181), cream, rgb8(199,126,58)};
        if (view == ViewMode::VideoPlayer) return {
            rgb8(65,37,24), rgb8(91,52,31), rgb8(244,232,205),
            rgb8(49,28,19), cream, rgb8(214,190,166),
            rgb8(144,82,39), rgb8(111,62,32), rgb8(54,30,19),
            rgb8(199,126,58), cream, rgb8(199,126,58)};
        if (view == ViewMode::Library) return {
            rgb8(61,94,49), rgb8(77,120,61), rgb8(244,232,205),
            rgb8(42,67,35), cream, rgb8(215,228,204),
            rgb8(107,142,83), rgb8(83,127,66), rgb8(44,70,37),
            rgb8(132,160,101), cream, rgb8(198,151,58)};
        if (view == ViewMode::Discover) return {
            rgb8(126,38,53), rgb8(158,51,68), rgb8(244,232,205),
            rgb8(91,28,39), cream, rgb8(235,205,211),
            rgb8(184,69,86), rgb8(160,52,69), rgb8(91,28,39),
            rgb8(203,92,108), cream, rgb8(211,144,55)};
        if (view == ViewMode::LiveTV) return {
            rgb8(35,91,105), rgb8(44,112,128), rgb8(244,232,205),
            rgb8(24,68,80), cream, rgb8(207,229,232),
            rgb8(57,135,151), rgb8(43,111,127), rgb8(23,66,77),
            rgb8(91,159,172), cream, rgb8(211,144,55)};
        if (view == ViewMode::Stream) return stream_palette_for(streamPlatform);
        if (view == ViewMode::Studio) return {
            rgb8(194,148,47), rgb8(222,181,85), rgb8(250,239,213),
            rgb8(105,59,24), darkText, rgb8(119,83,45),
            rgb8(228,188,91), rgb8(215,166,66), rgb8(111,62,25),
            rgb8(248,215,143), darkText, rgb8(184,120,35)};
        if (view == ViewMode::P2P) return {
            rgb8(225,233,240), rgb8(213,226,237), rgb8(248,250,252),
            rgb8(85,122,150), rgb8(43,58,70), rgb8(101,122,138),
            rgb8(91,133,157), rgb8(112,146,165), rgb8(77,100,117),
            rgb8(176,194,203), rgb8(44,55,61), rgb8(91,133,157)};
        if (view == ViewMode::Debug) return {
            rgb8(41,40,48), rgb8(57,55,64), rgb8(78,75,83),
            rgb8(22,21,26), rgb8(244,232,205), rgb8(194,187,177),
            rgb8(86,82,91), rgb8(70,67,76), rgb8(27,26,32),
            rgb8(102,97,108), rgb8(244,232,205), rgb8(199,126,58)};
        // Search is the one native page whose main background remains Nougat cream.
        return {
            rgb8(241,227,194), rgb8(236,216,179), rgb8(252,247,239),
            rgb8(166,112,56), darkText, rgb8(124,95,71),
            rgb8(202,158,62), rgb8(219,190,147), rgb8(144,91,37),
            rgb8(238,210,168), rgb8(72,39,20), rgb8(191,122,46)};
    }


    static unsigned long component_to_visual_mask(unsigned char value, unsigned long mask) {
        if (mask == 0) return 0;
        int shift = 0;
        unsigned long scaledMask = mask;
        while ((scaledMask & 1UL) == 0UL) { scaledMask >>= 1U; ++shift; }
        const unsigned long scaled = (static_cast<unsigned long>(value) * scaledMask + 127UL) / 255UL;
        return (scaled << shift) & mask;
    }

    unsigned long visual_pixel(unsigned char r, unsigned char g, unsigned char b) const {
        Visual* visual = DefaultVisual(d, screen);
        return component_to_visual_mask(r, visual->red_mask) |
               component_to_visual_mask(g, visual->green_mask) |
               component_to_visual_mask(b, visual->blue_mask);
    }

    struct SheetRgb { unsigned char r=0, g=0, b=0; };

    SheetRgb sheet_rgb_from_pixel(unsigned long pixel) const {
        XColor c{};
        c.pixel = pixel;
        XQueryColor(d, DefaultColormap(d, screen), &c);
        return {static_cast<unsigned char>(c.red / 257U),
                static_cast<unsigned char>(c.green / 257U),
                static_cast<unsigned char>(c.blue / 257U)};
    }

    static unsigned char sheet_lerp_channel(unsigned char a, unsigned char b, int numerator, int denominator) {
        numerator = std::max(0, std::min(denominator, numerator));
        return static_cast<unsigned char>((static_cast<int>(a) * (denominator - numerator) +
                                           static_cast<int>(b) * numerator + denominator / 2) / denominator);
    }

    void draw_sheet_reference_texture(Drawable target, const Rect& area,
                                      unsigned long darkPixel, unsigned long facePixel,
                                      unsigned long lightPixel) {
        if (area.w <= 0 || area.h <= 0) return;
        const SheetRgb dark = sheet_rgb_from_pixel(darkPixel);
        const SheetRgb face = sheet_rgb_from_pixel(facePixel);
        const SheetRgb light = sheet_rgb_from_pixel(lightPixel);
        const int depth = DefaultDepth(d, screen);
        const int bits_per_pixel = (depth <= 16) ? 16 : 32;
        const int bytes_per_pixel = bits_per_pixel / 8;
        const std::size_t bytes = static_cast<std::size_t>(area.w) * static_cast<std::size_t>(area.h) * static_cast<std::size_t>(bytes_per_pixel);
        char* image_data = static_cast<char*>(std::calloc(bytes, 1));
        if (!image_data) return;
        for (int y = 0; y < area.h; ++y) {
            const int sy = std::min(nougat_ui_sheet_texture::kButtonTextureHeight - 1,
                                    y * nougat_ui_sheet_texture::kButtonTextureHeight / std::max(1, area.h));
            for (int x = 0; x < area.w; ++x) {
                const int sx = std::min(nougat_ui_sheet_texture::kButtonTextureWidth - 1,
                                        x * nougat_ui_sheet_texture::kButtonTextureWidth / std::max(1, area.w));
                const int luma = nougat_ui_sheet_texture::kButtonTexture[sy * nougat_ui_sheet_texture::kButtonTextureWidth + sx];
                SheetRgb out{};
                if (luma <= 128) {
                    out.r = sheet_lerp_channel(dark.r, face.r, luma, 128);
                    out.g = sheet_lerp_channel(dark.g, face.g, luma, 128);
                    out.b = sheet_lerp_channel(dark.b, face.b, luma, 128);
                } else {
                    const int t = luma - 128;
                    out.r = sheet_lerp_channel(face.r, light.r, t, 127);
                    out.g = sheet_lerp_channel(face.g, light.g, t, 127);
                    out.b = sheet_lerp_channel(face.b, light.b, t, 127);
                }
                const unsigned long pixel = visual_pixel(out.r, out.g, out.b);
                std::memcpy(image_data +
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(area.w) + static_cast<std::size_t>(x)) * static_cast<std::size_t>(bytes_per_pixel),
                    &pixel, static_cast<std::size_t>(bytes_per_pixel));
            }
        }
        XImage* image = XCreateImage(d, DefaultVisual(d, screen), depth, ZPixmap, 0,
                                     image_data, area.w, area.h, 32, 0);
        if (!image) { std::free(image_data); return; }
        XPutImage(d, target, gc, image, 0, 0, area.x, area.y, area.w, area.h);
        XDestroyImage(image);
    }

    enum class SheetControlState { Normal, Hover, Pressed, Disabled };

    unsigned long sheet_button_face(const ViewPalette& palette, SheetControlState state) {
        switch (state) {
            case SheetControlState::Hover: return palette.buttonLight;
            case SheetControlState::Pressed: return palette.buttonDark;
            case SheetControlState::Disabled: return palette.panel;
            case SheetControlState::Normal: break;
        }
        return palette.button;
    }

    unsigned long sheet_button_ink(const ViewPalette& palette, SheetControlState state) {
        if (state == SheetControlState::Disabled) return palette.muted;
        if (state == SheetControlState::Pressed) return rgb8(250, 239, 219);
        return palette.buttonText;
    }

    void draw_sheet_button_surface(Drawable target, const Rect& raw, const ViewPalette& palette,
                                   SheetControlState state) {
        if (raw.w <= 3 || raw.h <= 5) return;
        const Rect visual{raw.x + 2, raw.y + 1, std::max(1, raw.w - 4), std::max(1, raw.h - 4)};
        const int radius = std::max(5, std::min(10, visual.h / 2));
        Rect shadow1{visual.x, visual.y + 3, visual.w, visual.h};
        fill_round(target, shadow1, radius, palette.buttonDark);
        Rect shadow2{visual.x + 1, visual.y + 2, std::max(1, visual.w - 2), visual.h};
        fill_round(target, shadow2, std::max(4, radius - 1), palette.border);

        const unsigned long face = sheet_button_face(palette, state);
        fill_round(target, visual, radius, face);
        outline_round(target, visual, radius, palette.buttonDark);
        // The surface lighting texture is sampled directly from the owner-approved
        // UI sheet, with its hue replaced by the already-accepted page palette.
        // This keeps the sheet's actual highlight/depth pattern instead of a flat approximation.
        Rect textureArea{visual.x + 5, visual.y + 4, std::max(1, visual.w - 10), std::max(1, visual.h - 9)};
        const unsigned long textureLight = state == SheetControlState::Pressed ? palette.button : palette.buttonLight;
        draw_sheet_reference_texture(target, textureArea, palette.buttonDark, face, textureLight);

        Rect bevel{visual.x + 1, visual.y + 1, std::max(1, visual.w - 2), std::max(1, visual.h - 2)};
        outline_round(target, bevel, std::max(4, radius - 1),
                      state == SheetControlState::Pressed ? palette.button : palette.buttonLight);
        Rect seam{visual.x + 3, visual.y + 3, std::max(1, visual.w - 6), std::max(1, visual.h - 6)};
        const bool searchCream = palette.background == rgb8(241, 227, 194);
        const unsigned long seamInk = searchCream
            ? (state == SheetControlState::Pressed ? rgb8(89, 48, 25) : rgb8(126, 72, 35))
            : (state == SheetControlState::Pressed ? palette.button : palette.buttonLight);
        outline_round_dashed(target, seam, std::max(3, radius - 3), seamInk, 2);

        // Raised leather/nougat highlight and lower bevel from the sheet.
        line(target, visual.x + radius, visual.y + 2,
             visual.x + visual.w - radius - 1, visual.y + 2,
             state == SheetControlState::Pressed ? palette.button : rgb8(255, 238, 205));
        line(target, visual.x + radius, visual.y + visual.h - 2,
             visual.x + visual.w - radius - 1, visual.y + visual.h - 2, palette.buttonDark);
    }

    void draw_top_nav_tab_surface(Drawable target, const Rect& raw, const ViewPalette& palette,
                                  bool active, bool hover) {
        if (raw.w <= 6 || raw.h <= 8) return;
        const SheetControlState state = hover ? SheetControlState::Hover : SheetControlState::Normal;
        const unsigned long face = sheet_button_face(palette, state);
        const Rect body{raw.x + 1, raw.y + 1, std::max(1, raw.w - 2), std::max(1, raw.h - 3)};
        const int radius = 5;

        // Actual sheet tab construction: shallow square-rounded body, dark
        // lower/right depth, bright inner bevel, and stitched/inset inner line.
        Rect shadow{body.x + 1, body.y + 3, body.w, body.h};
        fill_round(target, shadow, radius, palette.buttonDark);
        fill_round(target, body, radius, face);
        outline_round(target, body, radius, palette.buttonDark);

        Rect textureArea{body.x + 4, body.y + 4, std::max(1, body.w - 8), std::max(1, body.h - 9)};
        draw_sheet_reference_texture(target, textureArea, palette.buttonDark, face, palette.buttonLight);

        Rect bevel{body.x + 2, body.y + 2, std::max(1, body.w - 4), std::max(1, body.h - 4)};
        outline_round(target, bevel, 3, hover ? palette.button : palette.buttonLight);
        Rect seam{body.x + 4, body.y + 4, std::max(1, body.w - 8), std::max(1, body.h - 8)};
        const bool searchCream = palette.background == rgb8(241, 227, 194);
        const unsigned long seamInk = searchCream ? rgb8(126, 72, 35) : palette.buttonLight;
        outline_round_dashed(target, seam, 2, seamInk, 2);
        line(target, body.x + 7, body.y + 2, body.x + body.w - 8, body.y + 2, rgb8(255, 241, 214));
        line(target, body.x + 5, body.y + body.h - 2, body.x + body.w - 6, body.y + body.h - 2, palette.buttonDark);

        // The active pointer is painted as a final overlay after the page body.
        // That keeps the enlarged pointer visible below the taller global tabs
        // instead of letting the page background/loading strip clip it away.
        (void)active;
    }

    void draw_sheet_tab_surface(Drawable target, const Rect& raw, const ViewPalette& palette,
                                bool active, bool hover) {
        draw_sheet_button_surface(target, raw, palette,
                                  hover ? SheetControlState::Hover : SheetControlState::Normal);
        if (!active) return;
        const Rect visual{raw.x + 2, raw.y + 1, std::max(1, raw.w - 4), std::max(1, raw.h - 4)};
        const int cx = visual.x + visual.w / 2;
        const int baseY = visual.y + visual.h - 1;
        const int tipY = raw.y + raw.h + 6;
        XPoint outer[5] = {
            {static_cast<short>(cx - 12), static_cast<short>(baseY - 2)},
            {static_cast<short>(cx - 7), static_cast<short>(baseY + 1)},
            {static_cast<short>(cx), static_cast<short>(tipY)},
            {static_cast<short>(cx + 7), static_cast<short>(baseY + 1)},
            {static_cast<short>(cx + 12), static_cast<short>(baseY - 2)}
        };
        XSetForeground(d, gc, palette.buttonDark);
        XFillPolygon(d, target, gc, outer, 5, Convex, CoordModeOrigin);
        XPoint inner[5] = {
            {static_cast<short>(cx - 9), static_cast<short>(baseY - 2)},
            {static_cast<short>(cx - 5), static_cast<short>(baseY)},
            {static_cast<short>(cx), static_cast<short>(tipY - 2)},
            {static_cast<short>(cx + 5), static_cast<short>(baseY)},
            {static_cast<short>(cx + 9), static_cast<short>(baseY - 2)}
        };
        XSetForeground(d, gc, hover ? palette.buttonLight : palette.button);
        XFillPolygon(d, target, gc, inner, 5, Convex, CoordModeOrigin);
        line(target, cx - 6, baseY, cx, tipY - 2, palette.buttonLight);
        line(target, cx, tipY - 2, cx + 6, baseY, palette.buttonLight);
    }

    void draw_sheet_panel_surface(Drawable target, const Rect& r, const ViewPalette& palette) {
        if (r.w <= 2 || r.h <= 2) return;
        Rect shadow{r.x, r.y + 4, r.w, r.h};
        fill_round(target, shadow, 11, palette.buttonDark);
        fill_round(target, r, 11, palette.panel);
        outline_round(target, r, 11, palette.border);
        Rect bevel{r.x + 2, r.y + 2, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        outline_round(target, bevel, 9, palette.buttonLight);
        Rect seam{r.x + 5, r.y + 5, std::max(1, r.w - 10), std::max(1, r.h - 10)};
        const unsigned long panelSeam = palette.background == rgb8(241, 227, 194)
            ? rgb8(132, 78, 40) : palette.buttonLight;
        outline_round_dashed(target, seam, 7, panelSeam, 3);
        line(target, r.x + 12, r.y + 2, r.x + r.w - 13, r.y + 2, rgb8(255, 242, 217));
    }

    void draw_sheet_track(Drawable target, const Rect& track, unsigned long border,
                          unsigned long base, unsigned long highlight) {
        Rect shadow{track.x, track.y + 2, track.w, track.h};
        fill_round(target, shadow, std::max(4, track.h / 2), border);
        fill_round(target, track, std::max(4, track.h / 2), base);
        outline_round(target, track, std::max(4, track.h / 2), border);
        Rect texture{track.x + 5, track.y + 4, std::max(1, track.w - 10), std::max(1, track.h - 8)};
        draw_sheet_reference_texture(target, texture, border, base, highlight);
        Rect inset{track.x + 2, track.y + 2, std::max(1, track.w - 4), std::max(1, track.h - 4)};
        outline_round(target, inset, std::max(2, track.h / 2 - 2), highlight);
    }

    void draw_sheet_track_segment(Drawable target, const Rect& track, int start, int width,
                                  unsigned long dark, unsigned long face, unsigned long highlight) {
        start = std::max(0, std::min(track.w, start));
        width = std::max(0, std::min(track.w - start, width));
        if (width <= 0) return;
        Rect fillRect{track.x + start + 1, track.y + 2, std::max(1, width - 1), std::max(3, track.h - 4)};
        fill_round(target, fillRect, std::max(2, fillRect.h / 2), face);
        if (fillRect.w > 10) {
            Rect texture{fillRect.x + 4, fillRect.y + 2, std::max(1, fillRect.w - 8), std::max(1, fillRect.h - 4)};
            draw_sheet_reference_texture(target, texture, dark, face, highlight);
        }
        outline_round(target, fillRect, std::max(2, fillRect.h / 2), dark);
        if (fillRect.w > 8) line(target, fillRect.x + 4, fillRect.y + 1,
                                 fillRect.x + fillRect.w - 5, fillRect.y + 1, highlight);
    }

    void draw_sheet_track_fill(Drawable target, const Rect& track, int width,
                               unsigned long dark, unsigned long face, unsigned long highlight) {
        draw_sheet_track_segment(target, track, 0, width, dark, face, highlight);
    }

    void draw_sheet_knob(Drawable target, int centerX, int centerY, int diameter,
                         unsigned long dark, unsigned long face, unsigned long highlight) {
        diameter = std::max(12, diameter);
        const int x = centerX - diameter / 2;
        const int y = centerY - diameter / 2;
        fill_circle(target, x + 1, y + 3, diameter, dark);
        fill_circle(target, x, y, diameter, face);
        XSetForeground(d, gc, dark);
        XDrawArc(d, target, gc, x, y, diameter, diameter, 0, 360 * 64);
        XSetForeground(d, gc, highlight);
        XDrawArc(d, target, gc, x + 2, y + 2, diameter - 4, diameter - 4, 35 * 64, 150 * 64);
        XSetForeground(d, gc, rgb8(173, 127, 74));
        XDrawArc(d, target, gc, x + 3, y + 3, diameter - 6, diameter - 6, 210 * 64, 115 * 64);
    }

    void draw_sheet_status_circle(Drawable target, int x, int y, int diameter,
                                  unsigned long stateColor) {
        // v0.0.37: use the sheet's circular SHAPE VARIANT as a stateful button.
        // The entire face changes state color; there is no tiny LED nested in a
        // tan ring. The stitched perimeter remains visible in every state.
        diameter = std::max(16, diameter);
        const unsigned long dark = rgb8(112, 70, 35);
        const unsigned long shadow = rgb8(166, 125, 82);
        const unsigned long light = rgb8(255, 239, 207);
        const unsigned long stitch = rgb8(244, 223, 187);
        fill_circle(target, x + 1, y + 3, diameter, shadow);
        fill_circle(target, x, y, diameter, stateColor);
        XSetForeground(d, gc, dark);
        XDrawArc(d, target, gc, x, y, diameter, diameter, 0, 360 * 64);
        if (diameter > 8) {
            XSetForeground(d, gc, light);
            XDrawArc(d, target, gc, x + 2, y + 2, diameter - 4, diameter - 4, 35 * 64, 150 * 64);
            XSetLineAttributes(d, gc, 1, LineOnOffDash, CapButt, JoinMiter);
            const char dash[] = {2, 2};
            XSetDashes(d, gc, 0, dash, 2);
            XSetForeground(d, gc, stitch);
            XDrawArc(d, target, gc, x + 3, y + 3, diameter - 6, diameter - 6, 0, 360 * 64);
            XSetLineAttributes(d, gc, 1, LineSolid, CapButt, JoinMiter);
        }
    }

    void draw_sheet_checkbox(Drawable target, const Rect& box, bool checked, const ViewPalette& palette) {
        draw_sheet_button_surface(target, box, palette,
                                  checked ? SheetControlState::Pressed : SheetControlState::Normal);
        if (!checked) return;
        const int x0 = box.x + std::max(6, box.w / 4);
        const int y0 = box.y + box.h / 2;
        const int x1 = box.x + box.w / 2 - 1;
        const int y1 = box.y + box.h - std::max(6, box.h / 4);
        const int x2 = box.x + box.w - std::max(5, box.w / 5);
        const int y2 = box.y + std::max(5, box.h / 4);
        const unsigned long ink = rgb8(250, 239, 219);
        line(target, x0, y0, x1, y1, ink);
        line(target, x0, y0 + 1, x1, y1 + 1, ink);
        line(target, x1, y1, x2, y2, ink);
        line(target, x1, y1 + 1, x2, y2 + 1, ink);
    }


    void draw_primary_panel(Drawable target, const Rect& r, const ViewPalette& palette) {
        draw_sheet_panel_surface(target, r, palette);
    }

    void draw_disabled_player_button(Drawable target, const Rect& r, const std::string& label) {
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        draw_sheet_button_surface(target, r, palette, SheetControlState::Disabled);
        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        text(target, label_x, visual.y + visual.h / 2 + 5, label, palette.muted);
    }

    void button_on(Drawable target, const Rect& r, const std::string& label) {
        const ViewPalette palette = currentView == ViewMode::Stream
            ? stream_palette_for(streamPlatform) : palette_for(currentView);
        const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);
        const SheetControlState state = hover ? SheetControlState::Hover : SheetControlState::Normal;
        draw_sheet_button_surface(target, r, palette, state);
        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int label_y = visual.y + visual.h / 2 + 5;
        text(target, label_x, label_y, head_to_width(label, visual.w - 10), sheet_button_ink(palette, state));
    }

    void draw_suite_badge(Drawable target, int x0, int y0, unsigned char bgR, unsigned char bgG, unsigned char bgB) {
        for (int y=0; y<nougat_media_suite_icon::kTopBar14Size; ++y) {
            for (int x=0; x<nougat_media_suite_icon::kTopBar14Size; ++x) {
                const std::uint32_t argb = nougat_media_suite_icon::kTopBar14[y * nougat_media_suite_icon::kTopBar14Size + x];
                const unsigned char a = static_cast<unsigned char>((argb >> 24) & 0xffU);
                if (a == 0) continue;
                const unsigned char sr = static_cast<unsigned char>((argb >> 16) & 0xffU);
                const unsigned char sg = static_cast<unsigned char>((argb >> 8) & 0xffU);
                const unsigned char sb = static_cast<unsigned char>(argb & 0xffU);
                const unsigned char r = static_cast<unsigned char>((static_cast<unsigned>(sr) * a + static_cast<unsigned>(bgR) * (255U-a)) / 255U);
                const unsigned char g = static_cast<unsigned char>((static_cast<unsigned>(sg) * a + static_cast<unsigned>(bgG) * (255U-a)) / 255U);
                const unsigned char b = static_cast<unsigned char>((static_cast<unsigned>(sb) * a + static_cast<unsigned>(bgB) * (255U-a)) / 255U);
                XSetForeground(d, gc, visual_pixel(r,g,b));
                XDrawPoint(d, target, gc, x0+x, y0+y);
            }
        }
    }

    void append_net_wm_icon(std::vector<unsigned long>& data, int size, const std::uint32_t* pixels) {
        data.push_back(static_cast<unsigned long>(size));
        data.push_back(static_cast<unsigned long>(size));
        const int count = size * size;
        for (int i=0; i<count; ++i) data.push_back(static_cast<unsigned long>(pixels[i]));
    }

    void set_net_wm_icon() {
        std::vector<unsigned long> data;
        data.reserve(2 + 16*16 + 2 + 32*32 + 2 + 64*64);
        append_net_wm_icon(data, nougat_media_suite_icon::kIcon16Size, nougat_media_suite_icon::kIcon16);
        append_net_wm_icon(data, nougat_media_suite_icon::kIcon32Size, nougat_media_suite_icon::kIcon32);
        append_net_wm_icon(data, nougat_media_suite_icon::kIcon64Size, nougat_media_suite_icon::kIcon64);
        Atom netWmIcon = XInternAtom(d, "_NET_WM_ICON", False);
        Atom cardinal = XInternAtom(d, "CARDINAL", False);
        XChangeProperty(d, win, netWmIcon, cardinal, 32, PropModeReplace,
                        reinterpret_cast<unsigned char*>(data.data()), static_cast<int>(data.size()));
    }

    void set_window_identity() {
        XClassHint classHint;
        classHint.res_name = const_cast<char*>("nougat-media-suite");
        classHint.res_class = const_cast<char*>("NougatMediaSuite");
        XSetClassHint(d, win, &classHint);

        // GNOME Shell can associate a raw X11 window with the installed
        // NougatMediaSuite.desktop entry through this stable application ID.
        // This prevents the running application from falling back to the
        // generic gear icon when launched outside the desktop file.
        Atom gtkApplicationId = XInternAtom(d, "_GTK_APPLICATION_ID", False);
        Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
        const char* appId = "com.elderredsoftworks.NougatMediaSuite";
        XChangeProperty(d, win, gtkApplicationId, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(appId),
                        static_cast<int>(std::strlen(appId)));

        // Ubuntu/GNOME primarily associates a running X11 window with a
        // desktop entry by application ID / WM_CLASS.  Use a canonical
        // reverse-DNS desktop ID so a raw executable launch still resolves
        // to the installed Nougat launcher instead of the generic gear.
        Atom bamfDesktopFile = XInternAtom(d, "_BAMF_DESKTOP_FILE", False);
        const char* home = std::getenv("HOME");
        std::string desktopPath = home ? std::string(home) +
            "/.local/share/applications/com.elderredsoftworks.NougatMediaSuite.desktop" :
            "com.elderredsoftworks.NougatMediaSuite.desktop";
        XChangeProperty(d, win, bamfDesktopFile, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(desktopPath.c_str()),
                        static_cast<int>(desktopPath.size()));
        set_net_wm_icon();
    }

    void set_window_title() {
        const char* title = "Nougat Media Suite";
        XStoreName(d, win, title);
        Atom netWmName = XInternAtom(d, "_NET_WM_NAME", False);
        Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
        XChangeProperty(d, win, netWmName, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(title),
                        static_cast<int>(std::strlen(title)));
    }

    bool init() {
        d = XOpenDisplay(nullptr); if (!d) return false;
        screen = DefaultScreen(d);
        unsigned long bg = col(0xdede,0xdede,0xdede);
        win = XCreateSimpleWindow(d, RootWindow(d,screen), 100, 80, W, H, 1, BlackPixel(d,screen), bg);
        set_window_title();
        set_window_identity();
        clipboardAtom = XInternAtom(d, "CLIPBOARD", False);
        utf8Atom = XInternAtom(d, "UTF8_STRING", False);
        targetsAtom = XInternAtom(d, "TARGETS", False);
        textAtom = XInternAtom(d, "TEXT", False);
        XSelectInput(d, win, ExposureMask|StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|PointerMotionMask);
        Atom wmDelete = XInternAtom(d, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(d, win, &wmDelete, 1);
        gc = XCreateGC(d, win, 0, nullptr);
        fontInfo = XLoadQueryFont(d, "fixed");
        sectionFontInfo = XLoadQueryFont(d, "9x15bold");
        if (!sectionFontInfo) sectionFontInfo = XLoadQueryFont(d, "9x15");
        metadataFontInfo = XLoadQueryFont(d, "7x14");
        if (fontInfo) XSetFont(d, gc, fontInfo->fid);
        init_quilt_tiles();
        sheetVolumeLoaded = load_sheet_volume_frames();
        sheetSeekLoaded = load_sheet_seek_frames();
        layout();
        video = XCreateSimpleWindow(d, win, 10, 42, W-20, H-120, 0, BlackPixel(d,screen), BlackPixel(d,screen));
        XSelectInput(d, video, ExposureMask|ButtonPressMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|KeyPressMask);
        seekPreviewWindow = XCreateSimpleWindow(d, win, 0, 0, 260, 176, 0, rgb8(90,55,35), rgb8(35,25,22));
        XSelectInput(d, seekPreviewWindow, ExposureMask);
        xextHandle = dlopen("libXext.so.6", RTLD_NOW | RTLD_LOCAL);
        if (xextHandle) xShapeCombineMask = reinterpret_cast<XShapeCombineMaskFn>(dlsym(xextHandle, "XShapeCombineMask"));
        XMapWindow(d, video);
        apply_video_layout();
        create_cursors();
        XMapWindow(d, win);

        if (api.load(vlcErr)) {
            const char* args[] = {"--quiet", "--no-video-title-show"};
            inst = api.new_(2, args);
            if (!inst) vlcErr = "Could not start VLC engine.";
        }
        load_session();
        load_library_view_modes();
        std::string p2pRestoreError;
        if (p2p.restore_last(p2pRestoreError)) p2pUiStatus = "Previous P2P download restored.";
        else if (!p2pRestoreError.empty()) p2pUiStatus = p2pRestoreError;
        {
            std::string nougat_error;
            nougatState->node_id = nougat.node_id(nougat_error);
            nougatState->peers = nougat.peers(nougat_error);
            if (!nougat_error.empty()) nougatState->status = nougat_error;
        }
        if (mediaServer.persistent_enabled()) mediaServer.start();
        else mediaServer.refresh();
        liveTvChannels = tunerBackend.load_channels();
        liveTvPrograms = tunerBackend.load_guide();
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            serverState->status = server_control_label();
            serverState->state = mediaServer.state();
            serverState->owned = mediaServer.owns_server();
        }
        start_home_task();
        return true;
    }
    static constexpr int kCompactButtonW = 116;
    static constexpr int kCompactButtonH = 26;
    // v0.0.34: the global navigation is a real tab strip, not a row of thin
    // capsule buttons.  Keep this geometry independent from normal controls.
    static constexpr int kTopTabW = 106;
    static constexpr int kTopTabGap = 3;
    static constexpr int kTopTabH = 40;
    static constexpr int kTopBarH = 44;
    static constexpr int kTopTabPointerHalfW = 16;
    static constexpr int kTopTabPointerH = 10;
    // v0.0.35 repair: the enlarged global tab strip is now the one ruler for
    // every page. Stream's existing inner-control placement is the approved
    // baseline; all peer pages align to it rather than carrying old offsets.
    static constexpr int kPageControlY = kTopBarH + 10; // 54
    static constexpr int kPageControlBottom = kPageControlY + kCompactButtonH;

    int clamp_button_scroll(int value, int button_count, int viewport_width) const {
        const int total = std::max(0, button_count) * kCompactButtonW;
        const int maximum = std::max(0, total - std::max(kCompactButtonW, viewport_width));
        return std::max(0, std::min(value, maximum));
    }

    void layout_button_row(std::initializer_list<Rect*> buttons, int y, int& scroll) {
        const int viewport = std::max(kCompactButtonW, W - 56);
        scroll = clamp_button_scroll(scroll, static_cast<int>(buttons.size()), viewport);
        int x = 28 - scroll;
        for (Rect* rect : buttons) {
            *rect = {x, y, kCompactButtonW, kCompactButtonH};
            x += kCompactButtonW;
        }
    }

    Rect page_content_frame(ViewMode view) const {
        if (view == ViewMode::VideoPlayer) return {0,kTopBarH,W,std::max(1,H-kTopBarH)};
        return {10,kTopBarH,std::max(1,W-20),std::max(1,H-kTopBarH-10)};
    }

    void apply_page_clip(ViewMode view) {
        const Rect frame=page_content_frame(view);
        XRectangle clip{static_cast<short>(frame.x),static_cast<short>(frame.y),
                        static_cast<unsigned short>(std::max(1,frame.w)),
                        static_cast<unsigned short>(std::max(1,frame.h))};
        XSetClipRectangles(d,gc,0,0,&clip,1,Unsorted);
    }

    bool page_uses_connected_square_frame(ViewMode view) const {
        return view == ViewMode::Home || view == ViewMode::Library ||
               view == ViewMode::Discover || view == ViewMode::LiveTV ||
               view == ViewMode::Nougat || view == ViewMode::Stream ||
               view == ViewMode::Studio || view == ViewMode::Debug;
    }

    void draw_page_frame(Drawable target, ViewMode view) {
        if (view == ViewMode::VideoPlayer) return;
        const Rect frame=page_content_frame(view);
        const ViewPalette palette=palette_for(view);
        const int outerRadius = page_uses_connected_square_frame(view) ? 0 : 10;
        const int innerRadius = page_uses_connected_square_frame(view) ? 0 : 8;
        outline_round(target,frame,outerRadius,palette.border);
        Rect inner{frame.x+3,frame.y+3,std::max(1,frame.w-6),std::max(1,frame.h-6)};
        outline_round_dashed(target,inner,innerRadius,palette.buttonLight,3);
    }

    int top_nav_left_bound() {
        // v0.0.34 owner-visual repair: the whole scrollable tab mechanism begins
        // immediately after the actual brand text, with only a tiny non-touching
        // separation. The fixed Server/version side remains unchanged.
        return 28 + text_width("NOUGAT MEDIA SUITE") + 6;
    }

    void layout() {
        const int topStatusReserve = 154;
        const int topControlCount = 9;
        const int navLeft = top_nav_left_bound();
        topNavViewportW = std::max(24, W - navLeft - topStatusReserve);
        topNavClipX = navLeft;
        topNavClipRight = std::max(topNavClipX + 1, W - topStatusReserve);
        const int topTotalW = topControlCount * kTopTabW + (topControlCount - 1) * kTopTabGap;
        const int topMaxScroll = std::max(0, topTotalW - std::max(kTopTabW, topNavViewportW));
        topNavScrollX = std::max(0, std::min(topNavScrollX, topMaxScroll));
        int topX = navLeft - topNavScrollX;
        const int topStep = kTopTabW + kTopTabGap;
        homeTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        videoPlayerTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        libraryTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        discoverTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        liveTvTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        nougatTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        ytdlpTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        studioTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        debugTab = {topX,1,kTopTabW,kTopTabH};

        const int bottomY = H - 32;
        // Keep the player stack below the video instead of allowing the old
        // smaller-tab offsets to overlap the title/seek region.
        const int seekY = H - 140;
        const int controlCount = 8;
        const int controlTotalW = controlCount * kCompactButtonW;
        const int controlViewportW = std::max(kCompactButtonW, W - 20);
        controlsScrollX = clamp_button_scroll(controlsScrollX, controlCount, controlViewportW);
        int x = controlTotalW <= controlViewportW ? std::max(10, (W - controlTotalW) / 2) : 10 - controlsScrollX;
        openBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        rewindBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        previousBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        playBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        nextBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        forwardBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        stopBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        fsBtn = {x, bottomY, kCompactButtonW, kCompactButtonH};

        // v0.0.37: the approved sheet seek model now expands across the player
        // while the elapsed and total timestamps remain on the same line. The
        // exact caps/knob are preserved by draw_sheet_seek_frame(); only the
        // repeatable track spans scale to consume the available player width.
        const int currentTimeReserve = 62;
        const int totalTimeReserve = 62;
        const int seekGap = 12;
        const int sideMargin = 24;
        const int seekAvailable = std::max(220, W - sideMargin * 2 - currentTimeReserve - totalTimeReserve - seekGap * 2);
        const int seekWidth = seekAvailable;
        seekRect = {sideMargin + currentTimeReserve + seekGap, seekY, seekWidth, kSheetSeekH};

        // Exact approved VOLUME sheet component. It is intentionally fixed at
        // its native 335x47 sheet size so the housing/icons/bevel are not a
        // procedural approximation. The interactive track coordinates are the
        // actual track inside that sprite.
        const int volumeHousingW = kSheetVolumeW;
        const int volumeHousingH = kSheetVolumeH;
        const int volumePercentReserve = 58;
        const int volumeHousingX = std::max(10, (W - volumeHousingW - volumePercentReserve) / 2);
        const int volumeHousingY = H - 83;
        volumeHousingRect = {volumeHousingX, volumeHousingY, volumeHousingW, volumeHousingH};
        volRect = {volumeHousingX + 50, volumeHousingY + 15, 229, 16};

        const int promptX = std::max(20, W/2-kCompactButtonW);
        resumeBtn = {promptX, H/2+40, kCompactButtonW, kCompactButtonH};
        loadBtn = {promptX+kCompactButtonW, H/2+40, kCompactButtonW, kCompactButtonH};

        layout_button_row({&streamYoutubeTab,&streamVimeoTab,&streamRumbleTab,&streamRutubeTab,&streamVkTab,&streamOkTab},
                          kPageControlY, streamSourceScrollX);
        ytdlpUrlRect = {28, 120, std::max(240, W-56), 28};
        ytdlpOutputRect = {28, 160, std::max(240, W-56), 28};
        layout_button_row({&ytdlpDownloadBtn,&ytdlpDirectWatchBtn,&ytdlpWebpageBtn,&ytdlpClearBtn},
                          202, ytdlpButtonsScrollX);
        ytdlpFolderBtn = {0,0,0,0};

        p2pMagnetRect = {28, 148, std::max(240, W-56), 28};
        p2pOutputRect = {28, 188, std::max(240, W-56), 28};
        layout_button_row({&p2pLoadMagnetBtn,&p2pOpenTorrentBtn,&p2pPlayBtn,&p2pStopResumeBtn,&p2pRemoveBtn}, 230, p2pButtonsScrollX);
        p2pSpeedBtn={28,264,kCompactButtonW,kCompactButtonH};
        p2pSeedRulesBtn={28+kCompactButtonW,264,kCompactButtonW,kCompactButtonH};
        p2pQueueUpBtn={28+kCompactButtonW*2,264,kCompactButtonW,kCompactButtonH};
        p2pQueueDownBtn={28+kCompactButtonW*3,264,kCompactButtonW,kCompactButtonH};
        p2pReannounceBtn={28+kCompactButtonW*4,264,kCompactButtonW,kCompactButtonH};
        p2pRecheckBtn={28+kCompactButtonW*5,264,kCompactButtonW,kCompactButtonH};
        p2pPriorityBtn={28+kCompactButtonW*6,264,kCompactButtonW,kCompactButtonH};

        layout_button_row({&nougatSearchPanelTab,&nougatCrawlerPanelTab,&nougatP2PPanelTab,
                           &nougatVirusScanPanelTab,&nougatNetworkAdvancedBtn},
                          kPageControlY, nougatPanelButtonsScrollX);
        const int nougatRightColumnX = std::max(500, W-144);
        nougatSearchRect = {28, 90, std::max(220, W-400), 30};
        nougatRawBtn = {std::max(260, W-360),90,kCompactButtonW,kCompactButtonH};
        nougatPeersToggleBtn = {std::max(380, W-240),90,kCompactButtonW,kCompactButtonH};
        nougatSearchBtn = {nougatRightColumnX,90,kCompactButtonW,kCompactButtonH};
        nougatResultsBox = {28, 148, std::max(240, W-56), std::max(120, H-176)};
        nougatCrawlSeedRect = {28, 90, std::max(240, W-300), 30};
        nougatCrawlMinusBtn = {std::max(320, W-260),90,36,26};
        nougatCrawlPlusBtn = {std::max(364, W-216),90,36,26};
        nougatSameDomainBtn = {28, 132, kCompactButtonW, kCompactButtonH};
        nougatStartCrawlBtn = {28+kCompactButtonW, 132, kCompactButtonW, kCompactButtonH};
        nougatCrawlLogBox = {28, 190, std::max(240, W-56), std::max(120, H-218)};
        nougatPeerEntryRect = {28, 104, std::max(220, W-520), 30};
        nougatAddPeerBtn = {std::max(300, W-480),104,kCompactButtonW,kCompactButtonH};
        nougatRemovePeerBtn = {std::max(420, W-360),104,kCompactButtonW,kCompactButtonH};
        nougatNodeBtn = {std::max(540, W-240),104,kCompactButtonW,kCompactButtonH};
        nougatPeersToggleBtn = {std::max(660, W-120),104,kCompactButtonW,kCompactButtonH};
        nougatPeerListBox = {28, 154, std::max(240, W-56), std::max(120, H-182)};
        securityScanFileBtn = {28, 106, kCompactButtonW, kCompactButtonH};
        securityScanFolderBtn = {28+kCompactButtonW, 106, kCompactButtonW, kCompactButtonH};
        securityScanAgainBtn = {28+kCompactButtonW*2, 106, kCompactButtonW, kCompactButtonH};
        securityCommunityKeyBtn = {28+kCompactButtonW*3, 106, kCompactButtonW, kCompactButtonH};
        securityHistoryBtn = {28+kCompactButtonW*4, 106, kCompactButtonW, kCompactButtonH};
        securityResultsBox = {28, 174, std::max(240, W-56), std::max(120, H-202)};

        // Library now shares the same app-wide page-control baseline. Its
        // compact List/Grid toggles are fixed at the far right, while the main
        // Library actions horizontally scroll in the remaining row.
        const Rect libraryFrame = page_content_frame(ViewMode::Library);
        const int libraryInnerX = libraryFrame.x + 16;
        const int libraryViewRight = libraryFrame.x + libraryFrame.w - 16;
        libraryGridBtn = {libraryViewRight - 32, kPageControlY, 32, kCompactButtonH};
        libraryListViewBtn = {libraryGridBtn.x - 36, kPageControlY, 32, kCompactButtonH};
        const int libraryToolsViewport = std::max(kCompactButtonW, libraryListViewBtn.x - libraryInnerX - 8);
        libraryButtonsScrollX = clamp_button_scroll(libraryButtonsScrollX, 6, libraryToolsViewport);
        Rect* libraryTools[] = {&libraryMoviesBtn,&libraryTvBtn,&libraryAddFolderBtn,&libraryUnlinkFolderBtn,
                                &libraryRefreshBtn,&libraryBackBtn};
        int libraryToolX = libraryInnerX - libraryButtonsScrollX;
        for (Rect* tool : libraryTools) {
            *tool = {libraryToolX, kPageControlY, kCompactButtonW, kCompactButtonH};
            libraryToolX += kCompactButtonW;
        }
        // Search belongs on its own row below the green action buttons. It
        // uses the same sheet INPUT FIELD grammar as the other Nougat fields.
        const int librarySearchY = kPageControlBottom + 10;
        librarySearchBtn = {libraryViewRight - kCompactButtonW, librarySearchY, kCompactButtonW, 30};
        librarySearchRect = {libraryInnerX, librarySearchY,
                             std::max(180, librarySearchBtn.x - libraryInnerX - 8), 30};
        const int libraryBoxY = librarySearchRect.y + librarySearchRect.h + 34;
        const int libraryScrollX = libraryFrame.x + libraryFrame.w - 18;
        libraryListBox = {libraryInnerX, libraryBoxY, std::max(160, libraryScrollX-libraryInnerX-10), std::max(100, libraryFrame.y+libraryFrame.h-libraryBoxY-12)};
        libraryVerticalScrollTrack = {libraryScrollX, libraryListBox.y, 12, libraryListBox.h};

        const Rect liveFrame=page_content_frame(ViewMode::LiveTV);
        layout_button_row({&liveTvDetectBtn,&liveTvRefreshBtn,&liveTvScanBtn,&liveTvWatchBtn,&liveTvChannelsBtn,&liveTvGuideBtn,&liveTvGuideRefreshBtn,&liveTvNowBtn,&liveTvRecordBtn},
                          kPageControlY, liveTvButtonsScrollX);
        liveTvListBox={liveFrame.x+16,96,std::max(180,liveFrame.w-32),std::max(100,liveFrame.y+liveFrame.h-108)};

        layout_button_row({&discoverUsualTab,&discoverRandomTab,&discoverLocalMovieBtn,&discoverLocalTvBtn,
                           &discoverLiveTvBtn,&discoverExternalMovieBtn,&discoverExternalTvBtn,&discoverTmdbTestBtn,
                           &discoverTmdbReplaceBtn,&discoverTmdbClearBtn,&discoverMyServicesBtn},
                          kPageControlY, discoverButtonsScrollX);
        discoverResultBox = {28, 112, std::max(240, W-56), std::max(150, H-190)};
        discoverOpenBtn = {28, H-66, kCompactButtonW, kCompactButtonH};
        discoverWatchBtn = {28+kCompactButtonW, H-66, kCompactButtonW, kCompactButtonH};
        discoverServicesBackBtn = {28,kPageControlY,kCompactButtonW,kCompactButtonH};

        // v0.0.37: System owns administrative server controls. Library stays
        // focused on media/catalog actions, while Start/Stop/Refresh Server
        // live with diagnostics, logs, exports, and maintenance tools here.
        layout_button_row({&serverStartBtn,&serverStopBtn,&serverRefreshBtn,
                           &debugRunBtn,&debugRetryBtn,&debugMetadataBtn,&debugTmdbBtn,&debugLogsBtn,&debugCopyBtn,
                           &debugExportTextBtn,&debugExportJsonBtn,&debugBundleBtn},
                          kPageControlY, debugButtonsScrollX);
        debugListBox = {28, 126, std::max(240, W-56), std::max(150, H-154)};
        update_video_prompt_layout();
    }
    void update_video_prompt_layout() {
        const int promptX = std::max(12, (videoW - kCompactButtonW * 4) / 2);
        const int promptY = std::max(88, videoH/2+36);
        videoResumeBtn = {promptX, promptY, kCompactButtonW, kCompactButtonH};
        videoRestartBtn = {promptX + kCompactButtonW, promptY, kCompactButtonW, kCompactButtonH};
        videoCancelBtn = {promptX + kCompactButtonW * 2, promptY, kCompactButtonW, kCompactButtonH};
        videoLoadBtn = {promptX + kCompactButtonW * 2, promptY, kCompactButtonW, kCompactButtonH};
        videoBackLibraryBtn = {promptX + kCompactButtonW * 3, promptY, kCompactButtonW, kCompactButtonH};

        const int upNextTotalW = kCompactButtonW * 3;
        const int upNextX = std::max(12, (videoW - upNextTotalW) / 2);
        const int upNextY = std::max(122, videoH / 2 + 42);
        videoUpNextPlayBtn = {upNextX, upNextY, kCompactButtonW, kCompactButtonH};
        videoUpNextSeriesBtn = {upNextX + kCompactButtonW, upNextY, kCompactButtonW, kCompactButtonH};
        videoUpNextReplayBtn = {upNextX + kCompactButtonW * 2, upNextY, kCompactButtonW, kCompactButtonH};
    }
    void apply_video_layout() {
        if (!video) return;
        if (currentView == ViewMode::Home || currentView == ViewMode::Library || currentView == ViewMode::Discover ||
            currentView == ViewMode::Nougat || currentView == ViewMode::Stream || currentView == ViewMode::Studio || currentView == ViewMode::P2P ||
            currentView == ViewMode::Debug || currentView == ViewMode::LiveTV) {
            XUnmapWindow(d, video);
            return;
        }
        XMapWindow(d, video);
        if (fullscreen) {
            videoW = std::max(100, W);
            videoH = std::max(100, H);
            XMoveResizeWindow(d, video, 0, 0, videoW, videoH);
        } else {
            videoW = std::max(100, W-20);
            const int videoTop = kTopBarH + 6;
            const int videoBottom = std::max(videoTop + 100, seekRect.y - 10);
            videoH = std::max(100, videoBottom - videoTop);
            XMoveResizeWindow(d, video, 10, videoTop, videoW, videoH);
        }
        update_video_prompt_layout();
        apply_video_corner_shape();
    }
    void apply_video_corner_shape() {
        if (!video || !xShapeCombineMask) return;
        constexpr int kShapeBounding = 0;
        constexpr int kShapeSet = 0;
        if (fullscreen) {
            xShapeCombineMask(d, video, kShapeBounding, 0, 0, None, kShapeSet);
            return;
        }
        const int radius = 14;
        Pixmap mask = XCreatePixmap(d, video, static_cast<unsigned>(std::max(1, videoW)),
                                    static_cast<unsigned>(std::max(1, videoH)), 1);
        if (!mask) return;
        GC maskGc = XCreateGC(d, mask, 0, nullptr);
        if (!maskGc) { XFreePixmap(d, mask); return; }
        XSetForeground(d, maskGc, 0);
        XFillRectangle(d, mask, maskGc, 0, 0, static_cast<unsigned>(videoW), static_cast<unsigned>(videoH));
        XSetForeground(d, maskGc, 1);
        const int diameter = radius * 2;
        XFillRectangle(d, mask, maskGc, radius, 0, static_cast<unsigned>(std::max(1, videoW - diameter)), static_cast<unsigned>(videoH));
        XFillRectangle(d, mask, maskGc, 0, radius, static_cast<unsigned>(videoW), static_cast<unsigned>(std::max(1, videoH - diameter)));
        XFillArc(d, mask, maskGc, 0, 0, diameter, diameter, 90 * 64, 90 * 64);
        XFillArc(d, mask, maskGc, videoW - diameter, 0, diameter, diameter, 0, 90 * 64);
        XFillArc(d, mask, maskGc, 0, videoH - diameter, diameter, diameter, 180 * 64, 90 * 64);
        XFillArc(d, mask, maskGc, videoW - diameter, videoH - diameter, diameter, diameter, 270 * 64, 90 * 64);
        xShapeCombineMask(d, video, kShapeBounding, 0, 0, mask, kShapeSet);
        XFreeGC(d, maskGc);
        XFreePixmap(d, mask);
    }
    void adjust_volume(int delta) {
        if (!mp) return;
        int vol = api.get_volume(mp);
        if (vol < 0) vol = 100;
        vol = std::max(0, std::min(200, vol + delta));
        volumePercent = vol;
        api.set_volume(mp, vol);
        if (!fullscreen) draw_volume_only();
    }
    long long playback_time_ms() {
        if (paused && playbackCacheValid) return cachedPlaybackTimeMs;
        if (!mp) return playbackCacheValid ? cachedPlaybackTimeMs : 0;
        long long t = api.get_time(mp);
        if (t < 0) t = 0;
        const long long resolved = currentMediaIsYtDlpStream ? ytdlpStreamBaseMs + t : t;
        cachedPlaybackTimeMs = resolved;
        playbackCacheValid = true;
        return resolved;
    }
    long long playback_length_ms() {
        if (paused && playbackCacheValid) return cachedPlaybackLengthMs;
        if (!mp) return cachedPlaybackLengthMs;
        long long l = api.get_length(mp);
        long long resolved = l;
        if (currentMediaIsYtDlpStream) {
            if (l > 0 && ytdlpTotalDurationMs <= 0) {
                ytdlpTotalDurationMs = ytdlpStreamBaseMs > 0 ? ytdlpStreamBaseMs + l : l;
            }
            resolved = ytdlpTotalDurationMs > 0 ? ytdlpTotalDurationMs : (l > 0 ? ytdlpStreamBaseMs + l : 0);
        }
        if (resolved > 0) { cachedPlaybackLengthMs = resolved; playbackCacheValid = true; }
        return resolved;
    }
    reddmedia::LibraryNode node_from_resume_record(const ResumeRecord& record) const {
        reddmedia::LibraryNode node;
        node.id = record.item_id;
        node.name = record.title;
        node.path = record.path;
        node.series_name = record.series_name;
        node.episode_title = record.episode_title;
        node.tmdb_id = record.tmdb_id;
        node.series_id = record.series_id;
        node.series_tmdb_id = record.series_tmdb_id;
        node.primary_image_tag = record.primary_image_tag;
        node.poster_item_id = record.item_id;
        node.poster_image_tag = record.primary_image_tag;
        node.backdrop_image_tag = record.backdrop_image_tag;
        node.production_year = record.production_year;
        node.season_number = record.season_number;
        node.episode_number = record.episode_number;
        const int first = static_cast<int>(reddmedia::LibraryNodeKind::Movie);
        const int last = static_cast<int>(reddmedia::LibraryNodeKind::Episode);
        node.kind = static_cast<reddmedia::LibraryNodeKind>(std::max(first, std::min(last, record.kind)));
        return node;
    }

    std::string media_identity_for_node(const reddmedia::LibraryNode& node) const {
        if (node.kind == reddmedia::LibraryNodeKind::Episode) {
            std::ostringstream identity;
            if (!node.series_name.empty()) identity << node.series_name;
            else identity << "TV";
            if (node.season_number > 0 && node.episode_number > 0) {
                identity << "  •  S" << std::setfill('0') << std::setw(2) << node.season_number
                         << "E" << std::setw(2) << node.episode_number;
            } else if (node.episode_number > 0) {
                identity << "  •  Episode " << node.episode_number;
            }
            const std::string episode = !node.episode_title.empty() ? node.episode_title : node.name;
            if (!episode.empty()) identity << "  •  " << episode;
            return identity.str();
        }
        std::string identity = node.name;
        if (identity.empty() && !node.path.empty()) identity = stem_only(node.path);
        if (node.production_year > 0) identity += " (" + std::to_string(node.production_year) + ")";
        return identity;
    }

    std::string current_media_identity() const {
        if (activeLibraryItemValid && !activeLibraryItem.path.empty() && activeLibraryItem.path == currentPath) {
            return media_identity_for_node(activeLibraryItem);
        }
        if (!currentPath.empty()) return stem_only(currentPath);
        if (currentMediaIsLiveTv && !liveTvPlayingLabel.empty()) return liveTvPlayingLabel;
        if (currentMediaIsYtDlpStream && !ytdlpUrl.empty()) return ytdlpUrl;
        return "";
    }

    ResumeRecord current_resume_record(long long position_override=-1,
                                       long long duration_override=-1) {
        ResumeRecord record;
        if (currentPath.empty()) return record;
        record.path = currentPath;
        record.item_id = currentPath;
        record.title = stem_only(currentPath);
        if (activeLibraryItemValid && activeLibraryItem.path == currentPath) {
            record.item_id = activeLibraryItem.id.empty() ? currentPath : activeLibraryItem.id;
            record.title = activeLibraryItem.name.empty() ? stem_only(currentPath) : activeLibraryItem.name;
            record.series_name = activeLibraryItem.series_name;
            record.episode_title = activeLibraryItem.episode_title;
            record.tmdb_id = activeLibraryItem.tmdb_id;
            record.series_id = activeLibraryItem.series_id;
            record.series_tmdb_id = activeLibraryItem.series_tmdb_id;
            record.primary_image_tag = activeLibraryItem.primary_image_tag;
            record.backdrop_image_tag = activeLibraryItem.backdrop_image_tag;
            record.kind = static_cast<int>(activeLibraryItem.kind);
            record.production_year = activeLibraryItem.production_year;
            record.season_number = activeLibraryItem.season_number;
            record.episode_number = activeLibraryItem.episode_number;
        }
        record.position_ms = position_override >= 0 ? position_override : playback_time_ms();
        record.duration_ms = duration_override >= 0 ? duration_override : playback_length_ms();
        record.last_watched = static_cast<long long>(std::time(nullptr));
        record.completed = false;
        return record;
    }

    void persist_current_resume(bool force=false) {
        if (!hasMedia || currentPath.empty() || currentMediaIsNetwork || currentMediaIsP2P || currentMediaIsYtDlpStream) return;
        const long long now = now_ms();
        if (!force && now - lastResumePersistMs < 10000) return;
        lastResumePersistMs = now;
        const long long position = stoppedPlaybackVisible ? stoppedPlaybackPositionMs : playback_time_ms();
        const long long duration = playback_length_ms();
        if (playbackEndHandled && duration > 0 && position >= std::max<long long>(0, duration - 1500)) {
            resumeStore.mark_completed(currentPath);
            homeNeedsRefresh.store(true);
            return;
        }
        ResumeRecord record = current_resume_record(position, duration);
        resumeStore.update(record);
        homeNeedsRefresh.store(true);
    }

    void mark_current_resume_completed() {
        if (!currentPath.empty() && !currentMediaIsNetwork && !currentMediaIsP2P && !currentMediaIsYtDlpStream) {
            resumeStore.mark_completed(currentPath);
            homeNeedsRefresh.store(true);
        }
    }

    bool resume_record_is_useful(const ResumeRecord& record) const {
        if (record.completed || record.path.empty() || !exists_file(record.path)) return false;
        if (record.position_ms < 30000) return false;
        if (record.duration_ms > 0 && record.position_ms >= std::max<long long>(0, record.duration_ms - 30000)) return false;
        return true;
    }

    void request_local_playback(const std::string& path,
                                const reddmedia::LibraryNode* node,
                                bool offer_resume=true) {
        if (path.empty() || !exists_file(path)) return;
        persist_current_resume(true);
        cancel_tv_autoplay();
        if (node) {
            activeLibraryItem = *node;
            activeLibraryItemValid = true;
        } else {
            activeLibraryItem = inferred_episode_node_for_path(path);
            if (activeLibraryItem.path.empty()) {
                activeLibraryItem = reddmedia::LibraryNode{};
                activeLibraryItem.path = path;
                activeLibraryItem.name = stem_only(path);
                activeLibraryItem.kind = reddmedia::LibraryNodeKind::Movie;
            }
            activeLibraryItemValid = true;
        }
        if (activeLibraryItem.kind == reddmedia::LibraryNodeKind::Episode) {
            prepare_tv_autoplay(activeLibraryItem);
        }
        ResumeRecord record;
        if (offer_resume && resumeStore.find(path, record) && resume_record_is_useful(record)) {
            resumePromptOrigin = currentView;
            pendingResumeRecord = record;
            pendingResumeNode = node ? *node : node_from_resume_record(record);
            if (pendingResumeNode.path.empty()) pendingResumeNode.path = path;
            resumePromptVisible = true;
            stoppedPlaybackVisible = false;
            if (currentView != ViewMode::VideoPlayer) switch_view(ViewMode::VideoPlayer);
            else redraw();
            return;
        }
        if (currentView != ViewMode::VideoPlayer) switch_view(ViewMode::VideoPlayer);
        open_media(path, 0);
    }

    void continue_pending_resume() {
        if (!resumePromptVisible) return;
        const ResumeRecord record = pendingResumeRecord;
        activeLibraryItem = pendingResumeNode;
        activeLibraryItem.path = record.path;
        activeLibraryItemValid = true;
        resumePromptVisible = false;
        open_media(record.path, record.position_ms);
    }

    void start_over_pending_resume() {
        if (!resumePromptVisible) return;
        const ResumeRecord record = pendingResumeRecord;
        activeLibraryItem = pendingResumeNode;
        activeLibraryItem.path = record.path;
        activeLibraryItemValid = true;
        resumeStore.clear_position(record.path);
        homeNeedsRefresh.store(true);
        resumePromptVisible = false;
        open_media(record.path, 0);
    }

    void cancel_pending_resume() {
        if (!resumePromptVisible) return;
        const ViewMode return_view = resumePromptOrigin;
        resumePromptVisible = false;
        pendingResumeRecord = ResumeRecord{};
        if (return_view != ViewMode::VideoPlayer) switch_view(return_view);
        else redraw();
    }

    long long resolve_ytdlp_duration_ms(const std::string& engine, const std::string& url) {
        int outputPipe[2];
        if (pipe(outputPipe) != 0) return 0;
        pid_t pid = fork();
        if (pid == 0) {
            dup2(outputPipe[1], STDOUT_FILENO);
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
            close(outputPipe[0]); close(outputPipe[1]);
            execl(engine.c_str(), engine.c_str(),
                  "--ignore-config", "--no-playlist", "--skip-download",
                  "--print", "duration", url.c_str(), (char*)nullptr);
            _exit(127);
        }
        close(outputPipe[1]);
        if (pid < 0) { close(outputPipe[0]); return 0; }
        fcntl(outputPipe[0], F_SETFL, fcntl(outputPipe[0], F_GETFL, 0) | O_NONBLOCK);
        std::string output;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
        int status = 0;
        bool childDone = false;
        while (std::chrono::steady_clock::now() < deadline) {
            char buffer[128];
            for (;;) {
                ssize_t n = read(outputPipe[0], buffer, sizeof(buffer));
                if (n > 0) output.append(buffer, static_cast<std::size_t>(n));
                else break;
            }
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == pid || result == -1) { childDone = true; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!childDone) {
            kill(pid, SIGTERM);
            for (int i=0;i<10;++i) {
                pid_t result=waitpid(pid,&status,WNOHANG);
                if (result==pid || result==-1) { childDone=true; break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            if (!childDone) { kill(pid,SIGKILL); waitpid(pid,&status,0); }
        }
        char tail[128];
        for (;;) {
            ssize_t n=read(outputPipe[0],tail,sizeof(tail));
            if(n>0) output.append(tail,static_cast<std::size_t>(n)); else break;
        }
        close(outputPipe[0]);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return 0;
        char* end=nullptr;
        errno=0;
        const double seconds=std::strtod(output.c_str(),&end);
        if (errno!=0 || end==output.c_str() || seconds<=0.0 || seconds>864000.0) return 0;
        return static_cast<long long>(seconds*1000.0 + 0.5);
    }

    void release_player_instance_only() {
        if (mp) {
            api.stop(mp);
            api.player_release(mp);
            mp = nullptr;
        }
        hasMedia = false;
        paused = false;
        pendingSeek = false;
        subtitlesOn = false;
        chapterMarksMs.clear();
        chapterNames.clear();
        chapterMarksAreReal = false;
        chapterScanComplete = false;
        playbackCacheValid = false;
        cachedPlaybackTimeMs = 0;
        cachedPlaybackLengthMs = 0;
    }
    bool start_ytdlp_cache_playback_at(long long targetMs) {
        if (ytdlpUrl.empty()) return false;
        if (targetMs < 0) targetMs = 0;
        if (ytdlpTotalDurationMs > 0 && targetMs > ytdlpTotalDurationMs) targetMs = ytdlpTotalDurationMs;

        // Tear down the localhost stream first. This disconnects libVLC's network
        // reader before we stop/release the player and avoids close/seek deadlocks.
        ytdlpStream.stop();
        release_player_instance_only();
        currentMediaIsYtDlpStream = false;
        currentMediaIsNetwork = true;

        std::string engine = ytdlp_engine_path();
        std::string error;
        if (!ytdlpStream.start(engine, ytdlpUrl, targetMs, error)) {
            ytdlpSeekBuffering = false;
            ytdlpStatus = error.empty() ? "Could not start Stream cache bridge." : error;
            switch_view(ViewMode::Stream);
            redraw();
            return false;
        }
        ytdlpStreamBaseMs = targetMs;
        ytdlpSeekTargetMs = targetMs;
        ytdlpSeekStartedAtMs = now_ms();
        ytdlpSeekBuffering = true;
        ytdlpStatus = targetMs > 0 ? "Buffering Stream seek..."
                                   : "Buffering Stream cache at up to 1080p...";
        redraw();
        return true;
    }
    void finish_ytdlp_buffer_if_ready() {
        if (!ytdlpSeekBuffering || !ytdlpStream.running()) return;
        constexpr std::uint64_t kStartupBytes = 512ULL * 1024ULL;
        const std::uint64_t bytes = ytdlpStream.cache_bytes();
        const bool feederDone = !ytdlpStream.feeder_running();
        const bool timedOut = now_ms() - ytdlpSeekStartedAtMs >= 20000;
        const bool enough = bytes >= kStartupBytes || ((feederDone || timedOut) && bytes >= 65536);
        if (!enough && !feederDone && !timedOut) return;

        append_ytdlp_log(ytdlpStream.take_log());
        if (!enough) {
            ytdlpSeekBuffering = false;
            ytdlpStream.stop();
            ytdlpStatus = ytdlpStream.failed() ? "Stream cache feeder failed before playback could start."
                                               : "Stream cache did not buffer enough media.";
            switch_view(ViewMode::Stream);
            redraw();
            return;
        }

        const long long targetMs = ytdlpSeekTargetMs;
        ytdlpSeekBuffering = false;
        if (!open_ytdlp_cache_location(ytdlpStream.url(), targetMs)) {
            ytdlpStream.stop();
            ytdlpStatus = "VLC could not start the Stream cache. See log.";
            switch_view(ViewMode::Stream);
            redraw();
            return;
        }
        ytdlpStatus = targetMs > 0 ? "Seek buffered. Playing through local cache bridge."
                                   : "Playing through local cache bridge at up to 1080p.";
    }
    void seek_to_ms(long long targetMs) {
        if (targetMs < 0) targetMs = 0;
        long long l = ytdlpTotalDurationMs > 0 ? ytdlpTotalDurationMs : playback_length_ms();
        if (l > 0 && targetMs > l) targetMs = l;
        if (currentMediaIsYtDlpStream || ytdlpSeekBuffering) {
            // Every YouTube seek gets a fresh keyframe-aware feeder. Never ask
            // libVLC to seek inside a growing cache because that can feed a
            // decoder from a stale or incomplete container position.
            start_ytdlp_cache_playback_at(targetMs);
            return;
        }
        if (!mp) return;
        api.set_time(mp, targetMs);
        if (!fullscreen) draw_seek_time_only();
    }
    void seek_relative(long long deltaMs) {
        if (currentMediaIsYtDlpStream && mp) {
            seek_to_ms(playback_time_ms() + deltaMs);
            return;
        }
        if (ytdlpSeekBuffering) {
            seek_to_ms(ytdlpSeekTargetMs + deltaMs);
            return;
        }
        if (!mp) return;
        seek_to_ms(playback_time_ms() + deltaMs);
    }
    void tick_resume_seek() {
        if (!pendingSeek || !mp) return;
        if (pendingSeekMs <= 0) { pendingSeek=false; return; }
        api.set_time(mp, pendingSeekMs);
        long long current = api.get_time(mp);
        if (current >= pendingSeekMs - 1000 || time(nullptr) >= pendingSeekDeadline) {
            pendingSeek = false;
        }
    }
    void create_cursors() {
        normalCursor = XCreateFontCursor(d, XC_left_ptr);
        Pixmap bm = XCreateBitmapFromData(d, win, "\0", 1, 1);
        XColor black; memset(&black,0,sizeof(black));
        blankCursor = XCreatePixmapCursor(d, bm, bm, &black, &black, 0, 0);
        XFreePixmap(d, bm);
    }
    void load_session() {
        std::ifstream f(session_file()); if (!f) return;
        std::stringstream ss; ss << f.rdbuf(); std::string s = ss.str();
        sessionPath = json_value_string(s, "path");
        sessionTime = json_value_number(s, "time_ms");
        if (!sessionPath.empty() && exists_file(sessionPath)) needResumePrompt = true;
    }
    void save_session() {
        if (!hasMedia || currentPath.empty() || currentMediaIsP2P || currentMediaIsNetwork) return;
        ensure_config_dir();
        long long t = 0;
        if (shuttingDown) t = playbackCacheValid ? cachedPlaybackTimeMs : 0;
        else if (paused && playbackCacheValid) t = cachedPlaybackTimeMs;
        else if (mp) t = playback_time_ms();
        std::ofstream f(session_file());
        f << "{\n";
        f << "  \"path\": \"" << json_escape(currentPath) << "\",\n";
        f << "  \"title\": \"" << json_escape(basename_only(currentPath)) << "\",\n";
        f << "  \"time_ms\": " << t << ",\n";
        f << "  \"saved_at\": " << (long long)time(nullptr) << "\n";
        f << "}\n";
    }
    void stop_ytdlp_stream_process() {
        ytdlpSeekBuffering = false;
        ytdlpStream.stop();
        ytdlpStreamBaseMs = 0;
        ytdlpSeekTargetMs = 0;
        ytdlpSeekStartedAtMs = 0;
    }
    void cleanup_player() {
        // If YouTube owns the localhost bridge, disconnect the bridge before
        // stopping libVLC so its network reader cannot hold shutdown hostage.
        if (currentMediaIsYtDlpStream || ytdlpSeekBuffering || ytdlpStream.running()) {
            stop_ytdlp_stream_process();
        }
        release_player_instance_only();
        currentMediaIsYtDlpStream=false;
        currentMediaIsNetwork=false;
        if (currentMediaIsLiveTv) {
            currentMediaIsLiveTv=false;
            liveTvPlayingChannel=-1;
            liveTvPlayingLabel.clear();
            liveTvTunerUse=LiveTvTunerUse::Idle;
        }
    }
    bool open_media(const std::string& path, long long seek=0) {
        if (!inst || !api.media_new_path || path.empty()) return false;
        if (!tvAutoplayArmed && exists_file(path)) {
            reddmedia::LibraryNode candidate;
            if (activeLibraryItemValid && activeLibraryItem.path == path) candidate = activeLibraryItem;
            else candidate = inferred_episode_node_for_path(path);
            if (candidate.kind == reddmedia::LibraryNodeKind::Episode) prepare_tv_autoplay(candidate);
        }
        if (!currentPath.empty() && currentPath != path) persist_current_resume(true);
        p2pStream.stop();
        cleanup_player();

        // Local episode transitions can arrive immediately after libVLC reports
        // Ended/Stopped.  Give the player one clean retry instead of silently
        // accepting a failed media-player start and losing TV autoplay.
        for (int attempt = 0; attempt < 2; ++attempt) {
            libvlc_media_t* media = api.media_new_path(inst, path.c_str());
            if (!media) {
                if (attempt == 0) std::this_thread::sleep_for(std::chrono::milliseconds(80));
                continue;
            }
            mp = api.player_new_from_media(media);
            api.media_release(media);
            if (!mp) {
                if (attempt == 0) std::this_thread::sleep_for(std::chrono::milliseconds(80));
                continue;
            }
            api.set_xwindow(mp, (unsigned int)video);
            api.set_volume(mp, volumePercent);
            playbackEndHandled = false;
            currentPath = path; currentMediaIsP2P=false; currentMediaIsNetwork=false; hasMedia=true; paused=false; needResumePrompt=false;
            resumePromptVisible=false; stoppedPlaybackVisible=false; stoppedPlaybackPositionMs=0;
            subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
            chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false;
            playbackCacheValid=false; cachedPlaybackTimeMs=0; cachedPlaybackLengthMs=0;
            lastLocalPlaybackPositionMs=0; lastLocalPlaybackLengthMs=0;
            const int play_result = api.play(mp);
            if (play_result == 0) {
                auto_load_subtitle_for_current_media();
                if (seek > 0) {
                    pendingSeek = true;
                    pendingSeekMs = seek;
                    pendingSeekDeadline = time(nullptr) + 6;
                }
                redraw();
                return true;
            }
            api.player_release(mp);
            mp=nullptr;
            hasMedia=false;
            currentPath.clear();
            if (attempt == 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }
    bool open_p2p_stream_location(const std::string& url) {
        cancel_tv_autoplay();
        if (!inst || !api.media_new_location || url.empty()) return false;
        cleanup_player();
        libvlc_media_t* media = api.media_new_location(inst, url.c_str());
        if (!media) return false;
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, volumePercent);
        playbackEndHandled = false;
        currentPath.clear(); currentMediaIsP2P=true; currentMediaIsNetwork=true; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false;
        playbackCacheValid=false; cachedPlaybackTimeMs=0; cachedPlaybackLengthMs=0;
        api.play(mp);
        redraw();
        return true;
    }
    bool open_live_tv_location(const std::string& mrl, const std::vector<std::string>& options,
                               int channelIndex, const std::string& label) {
        cancel_tv_autoplay();
        if (!inst || !api.media_new_location || mrl.empty()) return false;
        p2pStream.stop();
        cleanup_player();
        libvlc_media_t* media = api.media_new_location(inst, mrl.c_str());
        if (!media) return false;
        if (api.media_add_option) {
            for (const std::string& option : options) api.media_add_option(media, option.c_str());
        }
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, volumePercent);
        playbackEndHandled = false;
        currentPath.clear();
        currentMediaIsP2P=false;
        currentMediaIsYtDlpStream=false;
        currentMediaIsNetwork=true;
        currentMediaIsLiveTv=true;
        liveTvPlayingChannel=channelIndex;
        liveTvPlayingLabel=label;
        liveTvTunerUse=LiveTvTunerUse::Watching;
        hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false;
        playbackCacheValid=false; cachedPlaybackTimeMs=0; cachedPlaybackLengthMs=0;
        const int rc=api.play(mp);
        if (rc != 0) {
            api.player_release(mp); mp=nullptr; hasMedia=false;
            currentMediaIsNetwork=false; currentMediaIsLiveTv=false;
            liveTvPlayingChannel=-1; liveTvPlayingLabel.clear();
            liveTvTunerUse=LiveTvTunerUse::Idle;
            return false;
        }
        switch_view(ViewMode::VideoPlayer);
        redraw();
        return true;
    }

    bool open_ytdlp_cache_location(const std::string& url, long long baseMs) {
        if (!inst || !api.media_new_location || url.empty()) return false;
        p2pStream.stop();
        libvlc_media_t* media = api.media_new_location(inst, url.c_str());
        if (!media) return false;
        if (api.media_add_option) api.media_add_option(media, ":network-caching=5000");
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, volumePercent);
        playbackEndHandled = false;
        ytdlpStreamBaseMs = std::max<long long>(0, baseMs);
        currentPath.clear(); currentMediaIsP2P=false; currentMediaIsNetwork=true; currentMediaIsYtDlpStream=true; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false;
        playbackCacheValid=false; cachedPlaybackTimeMs=0; cachedPlaybackLengthMs=0;
        const int rc = api.play(mp);
        if (rc != 0) {
            api.player_release(mp);
            mp=nullptr;
            currentMediaIsYtDlpStream=false;
            hasMedia=false;
            currentMediaIsNetwork=false;
            return false;
        }
        switch_view(ViewMode::VideoPlayer);
        redraw();
        return true;
    }
    void cancel_tv_autoplay() {
        tvAutoplayArmed=false;
        tvAutoplayQueue.clear();
        tvAutoplayIndex=-1;
        tvAutoplayRetryIndex=-1;
        tvAutoplayRetryAttempts=0;
        tvAutoplayRetryAtMs=0;
        lastLocalPlaybackPositionMs=0;
        lastLocalPlaybackLengthMs=0;
        upNextVisible=false;
        upNextHasEpisode=false;
        upNextTargetIndex=-1;
        upNextDeadlineMs=0;
        upNextLastDisplayedSeconds=-1;
        upNextMessage.clear();
        activeLibraryItemValid=false;
        playbackEndHandled=false;
    }
    void do_open() {
        std::string p = choose_file_dialog();
        if (!p.empty()) { cancel_tv_autoplay(); request_local_playback(p, nullptr, true); }
    }
    void toggle_play() {
        if (!mp) return;
        if (!paused) {
            cachedPlaybackTimeMs = playback_time_ms();
            const long long length = playback_length_ms();
            if (length > 0) cachedPlaybackLengthMs = length;
            playbackCacheValid = true;
            api.set_pause(mp, 1);
            paused = true;
            persist_current_resume(true);
        } else {
            api.set_pause(mp, 0);
            paused = false;
        }
        redraw();
    }
    void stop_media() {
        if (currentMediaIsYtDlpStream) {
            cancel_tv_autoplay();
            cleanup_player();
            currentMediaIsNetwork=false;
            ytdlpStatus="Play stream stopped.";
            redraw();
            return;
        }
        if (!mp) return;
        const reddmedia::LibraryNode saved_item = activeLibraryItem;
        const bool saved_item_valid = activeLibraryItemValid;
        const long long stopped_at = playback_time_ms();
        const long long duration = playback_length_ms();
        ResumeRecord record = current_resume_record(stopped_at, duration);
        if (!record.path.empty()) resumeStore.update(record);
        cancel_tv_autoplay();
        activeLibraryItem = saved_item;
        activeLibraryItemValid = saved_item_valid;
        api.stop(mp);
        paused=false;
        cachedPlaybackTimeMs=stopped_at;
        cachedPlaybackLengthMs=duration;
        playbackCacheValid=true;
        stoppedPlaybackPositionMs=stopped_at;
        stoppedPlaybackVisible=true;
        redraw();
    }

    std::vector<TrackChoice> audio_tracks() {
        std::vector<TrackChoice> out;
        if (!mp || !api.audio_get_track_description) return out;
        libvlc_track_description_t* list = api.audio_get_track_description(mp);
        for (libvlc_track_description_t* n=list; n; n=n->p_next) {
            TrackChoice t; t.id=n->i_id; t.name=n->psz_name ? n->psz_name : "Audio Track"; out.push_back(t);
        }
        if (list && api.track_description_list_release) api.track_description_list_release(list);
        return out;
    }
    std::vector<TrackChoice> subtitle_tracks() {
        std::vector<TrackChoice> out;
        if (!mp || !api.video_get_spu_description) return out;
        libvlc_track_description_t* list = api.video_get_spu_description(mp);
        for (libvlc_track_description_t* n=list; n; n=n->p_next) {
            TrackChoice t; t.id=n->i_id; t.name=n->psz_name ? n->psz_name : "Subtitle Track"; out.push_back(t);
        }
        if (list && api.track_description_list_release) api.track_description_list_release(list);
        return out;
    }
    int current_audio_track() { return (mp && api.audio_get_track) ? api.audio_get_track(mp) : -9999; }
    int current_subtitle_track() { return (mp && api.video_get_spu) ? api.video_get_spu(mp) : -1; }

    bool load_subtitle_file(const std::string& path) {
        if (!mp || !api.video_set_subtitle_file || path.empty() || !exists_file(path)) return false;
        int ok = api.video_set_subtitle_file(mp, path.c_str());
        if (ok != 0) {
            subtitlePath = path;
            subtitleFolder = dirname_only(path);
            subtitlesOn = true;
            if (api.video_set_spu_delay) api.video_set_spu_delay(mp, subtitleDelayUs);
            redraw();
            return true;
        }
        return false;
    }
    std::string find_subtitle_in_dir(const std::string& dir, const std::string& stem) {
        if (dir.empty()) return "";
        std::vector<std::string> candidates = {
            dir + "/" + stem + ".en.srt",
            dir + "/" + stem + ".eng.srt",
            dir + "/" + stem + ".english.srt",
            dir + "/" + stem + ".srt"
        };
        for (const std::string& c : candidates) if (exists_file(c)) return c;
        DIR* dp = opendir(dir.c_str());
        if (!dp) return "";
        std::string lowerStem = lower_copy(stem);
        std::string best;
        while (dirent* ent = readdir(dp)) {
            std::string name = ent->d_name;
            std::string low = lower_copy(name);
            if (!ends_with_lower(name, ".srt")) continue;
            if (low.find(lowerStem) == std::string::npos) continue;
            bool english = low.find(".en.") != std::string::npos || low.find("_en.") != std::string::npos ||
                           low.find("english") != std::string::npos || low.find("eng") != std::string::npos;
            if (english) { best = dir + "/" + name; break; }
            if (best.empty()) best = dir + "/" + name;
        }
        closedir(dp);
        return best;
    }
    std::string find_auto_subtitle(const std::string& mediaPath) {
        std::string dir = dirname_only(mediaPath);
        std::string stem = stem_only(mediaPath);
        std::string found = find_subtitle_in_dir(dir, stem);
        if (!found.empty()) return found;
        const char* subdirs[] = {"Subs", "subs", "Subtitles", "subtitles", nullptr};
        for (int i=0; subdirs[i]; ++i) {
            found = find_subtitle_in_dir(dir + "/" + subdirs[i], stem);
            if (!found.empty()) return found;
        }
        return "";
    }
    void auto_load_subtitle_for_current_media() {
        if (!mp || currentPath.empty()) return;
        std::string srt = find_auto_subtitle(currentPath);
        if (!srt.empty()) load_subtitle_file(srt);
    }
    void choose_and_load_subtitle_file() {
        std::string p = choose_subtitle_file_dialog();
        if (!p.empty()) load_subtitle_file(p);
    }
    void choose_and_load_subtitle_folder() {
        std::string folder = choose_folder_dialog();
        if (folder.empty()) return;
        subtitleFolder = folder;
        if (!currentPath.empty()) {
            std::string srt = find_subtitle_in_dir(folder, stem_only(currentPath));
            if (!srt.empty()) load_subtitle_file(srt);
        }
    }
    void set_subtitles_enabled(bool on) {
        if (!mp) return;
        if (!on) {
            if (api.video_set_spu) api.video_set_spu(mp, -1);
            subtitlesOn = false;
            redraw();
            return;
        }
        if (!subtitlePath.empty() && exists_file(subtitlePath) && load_subtitle_file(subtitlePath)) return;
        std::vector<TrackChoice> tracks = subtitle_tracks();
        for (const TrackChoice& t : tracks) {
            if (t.id >= 0 && api.video_set_spu) { api.video_set_spu(mp, t.id); subtitlesOn = true; redraw(); return; }
        }
    }
    void toggle_subtitles() { set_subtitles_enabled(!subtitlesOn); }
    void set_subtitle_track(int id) {
        if (!mp || !api.video_set_spu) return;
        api.video_set_spu(mp, id);
        subtitlesOn = id >= 0;
        redraw();
    }
    void set_audio_track(int id) {
        if (mp && api.audio_set_track) api.audio_set_track(mp, id);
    }
    void change_subtitle_delay(long long deltaUs) {
        if (mp && api.video_get_spu_delay) subtitleDelayUs = api.video_get_spu_delay(mp);
        subtitleDelayUs += deltaUs;
        if (subtitleDelayUs > 60000000LL) subtitleDelayUs = 60000000LL;
        if (subtitleDelayUs < -60000000LL) subtitleDelayUs = -60000000LL;
        if (mp && api.video_set_spu_delay) api.video_set_spu_delay(mp, subtitleDelayUs);
        redraw();
    }
    void reset_subtitle_delay() {
        subtitleDelayUs = 0;
        if (mp && api.video_set_spu_delay) api.video_set_spu_delay(mp, 0);
        redraw();
    }
    std::string subtitle_delay_label() {
        long long us = subtitleDelayUs;
        if (mp && api.video_get_spu_delay) us = api.video_get_spu_delay(mp);
        char b[64];
        snprintf(b, sizeof(b), "Delay: %+.1fs", (double)us / 1000000.0);
        return b;
    }

    void toggle_fullscreen() {
        Atom wm_state = XInternAtom(d, "_NET_WM_STATE", False);
        Atom fs_atom = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);
        XEvent xev; memset(&xev,0,sizeof(xev));
        xev.type = ClientMessage; xev.xclient.window = win; xev.xclient.message_type = wm_state; xev.xclient.format = 32;
        xev.xclient.data.l[0] = 2; // toggle
        xev.xclient.data.l[1] = fs_atom;
        xev.xclient.data.l[2] = 0;
        XSendEvent(d, DefaultRootWindow(d), False, SubstructureNotifyMask|SubstructureRedirectMask, &xev);
        fullscreen = !fullscreen;
        apply_video_layout();
        redraw();
    }
    void exit_fullscreen() {
        if (!fullscreen) return;
        Atom wm_state = XInternAtom(d, "_NET_WM_STATE", False);
        Atom fs_atom = XInternAtom(d, "_NET_WM_STATE_FULLSCREEN", False);
        XEvent xev; memset(&xev,0,sizeof(xev));
        xev.type = ClientMessage; xev.xclient.window = win; xev.xclient.message_type = wm_state; xev.xclient.format = 32;
        xev.xclient.data.l[0] = 0; // remove
        xev.xclient.data.l[1] = fs_atom;
        XSendEvent(d, DefaultRootWindow(d), False, SubstructureNotifyMask|SubstructureRedirectMask, &xev);
        fullscreen=false;
        apply_video_layout();
        redraw();
    }
    void resize(int w, int h) {
        W=w; H=h; layout();
        apply_video_layout();
        redraw();
    }
    void draw_video_message() {
        if (upNextVisible) {
            // Countdown updates used to XClearWindow(video) once per second, exposing
            // a blank frame before the overlay was redrawn. Compose the complete Up Next
            // frame offscreen and copy it to the video child in one operation instead.
            Pixmap upNextBuffer = XCreatePixmap(d, video, std::max(1, videoW), std::max(1, videoH), DefaultDepth(d, screen));
            if (!upNextBuffer) return;
            fill(upNextBuffer, {0, 0, std::max(1, videoW), std::max(1, videoH)}, rgb8(0,0,0));
            const Drawable overlayTarget = upNextBuffer;
            const int cardW = std::max(360, std::min(680, videoW - 40));
            const int cardH = 164;
            const int cardX = std::max(12, (videoW - cardW) / 2);
            const int cardY = std::max(26, (videoH - cardH) / 2 - 12);
            const Rect card{cardX, cardY, cardW, cardH};
            fill_round(overlayTarget, card, 12, rgb8(55,34,22));
            outline_round(overlayTarget, card, 12, rgb8(201,130,44));
            Rect inset{card.x+3, card.y+3, card.w-6, card.h-6};
            outline_round(overlayTarget, inset, 9, rgb8(244,229,205));

            if (upNextHasEpisode) {
                const std::string title = "Up Next: " + library_display_title(upNextEpisode);
                text(overlayTarget, card.x + 20, card.y + 36, head_to_width(title, card.w - 40), rgb8(248,235,214));
                text(overlayTarget, card.x + 20, card.y + 64,
                     upNextMessage.empty() ? "Playing automatically in 10 seconds." : upNextMessage,
                     rgb8(224,188,132));
                button_on(overlayTarget, videoUpNextPlayBtn, "Play Next");
            } else {
                text(overlayTarget, card.x + 20, card.y + 40, "Up Next", rgb8(248,235,214));
                text(overlayTarget, card.x + 20, card.y + 70,
                     head_to_width(upNextMessage.empty() ? "No next episode found." : upNextMessage, card.w - 40),
                     rgb8(224,188,132));
            }
            button_on(overlayTarget, videoUpNextSeriesBtn, "Back to Series");
            button_on(overlayTarget, videoUpNextReplayBtn, "Replay");
            XCopyArea(d, upNextBuffer, video, gc, 0, 0, std::max(1, videoW), std::max(1, videoH), 0, 0);
            XFreePixmap(d, upNextBuffer);
            XFlush(d);
            return;
        }

        if (resumePromptVisible) {
            XClearWindow(d, video);
            const std::string identity = media_identity_for_node(pendingResumeNode);
            text(video, 28, 44, "CONTINUE WATCHING?", rgb8(248,235,214));
            text(video, 28, 70, head_to_width(identity, videoW - 56), rgb8(238,218,190));
            text(video, 28, 94, "Last watched at " + format_time(pendingResumeRecord.position_ms), rgb8(206,176,142));
            button_on(video, videoResumeBtn, "Continue");
            button_on(video, videoRestartBtn, "Start Over");
            button_on(video, videoCancelBtn, "Cancel");
            return;
        }
        if (stoppedPlaybackVisible) {
            XClearWindow(d, video);
            text(video, 28, 44, "PLAYBACK STOPPED", rgb8(248,235,214));
            text(video, 28, 70, head_to_width(current_media_identity(), videoW - 56), rgb8(238,218,190));
            text(video, 28, 94, "Stopped at " + format_time(stoppedPlaybackPositionMs), rgb8(206,176,142));
            button_on(video, videoResumeBtn, "Resume");
            button_on(video, videoRestartBtn, "Restart");
            button_on(video, videoLoadBtn, "Load Different");
            button_on(video, videoBackLibraryBtn, "Back to Library");
            return;
        }
        if (fullscreen && hasMedia && vlcErr.empty() &&
            now_ms() - lastFullscreenOverlayMotionMs < 3000) {
            const std::string identity = current_media_identity();
            if (!identity.empty()) {
                const Rect label{18, 18, std::min(videoW - 36, std::max(220, text_width(identity) + 28)), 34};
                fill_round(video, label, 8, rgb8(32,25,21));
                outline_round(video, label, 8, rgb8(184,111,43));
                text(video, label.x + 12, label.y + 22, head_to_width(identity, label.w - 24), rgb8(248,235,214));
            }
            return;
        }
        if (hasMedia && !needResumePrompt && vlcErr.empty()) return;
        XClearWindow(d, video);
        if (!vlcErr.empty()) {
            text(video, 22, 40, vlcErr, col(0xffff,0xdddd,0xdddd));
            text(video, 22, 64, "On Ubuntu: install VLC, then reopen this executable.", col(0xdddd,0xdddd,0xdddd));
            return;
        }
        if (needResumePrompt) {
            text(video, 28, 44, "CONTINUE WATCHING?", col(0xeeee,0xeeee,0xeeee));
            text(video, 28, 68, head_to_width(stem_only(sessionPath), videoW - 56), col(0xdddd,0xdddd,0xdddd));
            text(video, 28, 90, "Last watched at " + format_time(sessionTime), col(0xcccc,0xcccc,0xcccc));
            button_on(video, videoResumeBtn, "Continue");
            button_on(video, videoRestartBtn, "Start Over");
            button_on(video, videoCancelBtn, "Cancel");
            return;
        }
        if (!hasMedia) {
            text(video, 28, 44, "Open a media file to start.", col(0xeeee,0xeeee,0xeeee));
            text(video, 28, 68, "File menu or Open button.", col(0xcccc,0xcccc,0xcccc));
        }
    }
    unsigned long tab_accent(ViewMode view) {
        if (view == ViewMode::Home) return col(0xb67a,0x7a7a,0xc7c7);
        if (view == ViewMode::VideoPlayer) return col(0xd2a5,0xa5a5,0x6d6d);
        if (view == ViewMode::Library) return col(0x8faa,0xaaaa,0x7777);
        if (view == ViewMode::Discover) return col(0xbd7a,0x7a7a,0xd1d1);
        if (view == ViewMode::LiveTV) return rgb8(57,135,151);
        if (view == ViewMode::Nougat) return col(0xd2a5,0xa5a5,0x6d6d);
        if (view == ViewMode::Stream) return stream_palette_for(streamPlatform).accent;
        if (view == ViewMode::P2P) return col(0x60a7,0xa7a7,0xd7d7);
        return col(0xf0b4,0xb4b4,0x2b2b);
    }

    void draw_top_bar(Drawable target) {
        const unsigned long topText = rgb8(72, 39, 20);
        const unsigned long divider = rgb8(174, 132, 87);
        const unsigned long headerTan = rgb8(227, 204, 172);
        fill(target, {0, 0, W, kTopBarH}, headerTan);
        line(target, 0, 1, W, 1, rgb8(250, 235, 211));

        const int badgeY = (kTopBarH - nougat_media_suite_icon::kTopBar14Size) / 2;
        const int headerBaseline = kTopBarH / 2 + 5;
        draw_suite_badge(target, 8, badgeY, 227, 204, 172);
        text(target, 28, headerBaseline, "NOUGAT MEDIA SUITE", topText);

        // Fixed brand and server/version areas never scroll. The tab row is
        // hard-clipped to the center lane, so a tab disappears at either edge
        // instead of painting over the Nougat identity or the version block.
        const std::string versionLabel = "v0.0.37";
        const int versionWidth = text_width(versionLabel);
        const int versionX = W - 10 - versionWidth;
        bool serverBusy = false;
        reddmedia::MediaServerState serverStateValue = reddmedia::MediaServerState::Stopped;
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            serverBusy = serverState->busy;
            serverStateValue = serverState->state;
        }
        unsigned long light = rgb8(190, 75, 67);
        if (serverStateValue == reddmedia::MediaServerState::Ready && !serverBusy) light = rgb8(134,151,84);
        else if (serverStateValue == reddmedia::MediaServerState::Starting || serverBusy) light = rgb8(201,145,73);
        const std::string serverLabel = "Server:";
        const int serverX = versionX - 42 - text_width(serverLabel);
        if (serverX > 4) {
            text(target, serverX, headerBaseline, serverLabel, topText);
            const int statusX = serverX + text_width(serverLabel) + 8;
            const int statusY = (kTopBarH - kServerStatusDiameter) / 2;
            draw_sheet_status_circle(target, statusX, statusY, kServerStatusDiameter, light);
        }
        text(target, versionX, headerBaseline, versionLabel, topText);

        // Paint the divider before the tabs so the active downward notch sits cleanly over it.
        line(target, 0, kTopBarH - 2, W, kTopBarH - 2, divider);

        topNavClipX = std::max(0, std::min(top_nav_left_bound(), W));
        topNavClipRight = std::max(topNavClipX + 1, std::min(W, serverX - 8));
        XRectangle navClip{static_cast<short>(topNavClipX),0,
                           static_cast<unsigned short>(std::max(1,topNavClipRight-topNavClipX)),kTopBarH};
        XSetClipRectangles(d, gc, 0, 0, &navClip, 1, Unsorted);
        const auto draw_tab = [&](const Rect& tab, const char* label, ViewMode view) {
            const bool active = currentView == view;
            const bool hover = tab.contains(pointerWindowX, pointerWindowY);
            const ViewPalette tabPalette = palette_for(view);
            Rect surface{tab.x, tab.y, tab.w, tab.h};
            draw_top_nav_tab_surface(target, surface, tabPalette, active, hover);
            const Rect visual{surface.x + 2, surface.y + 1, std::max(1, surface.w - 4), std::max(1, surface.h - 4)};
            const int labelX = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
            text(target, labelX, visual.y + visual.h / 2 + 5, label, tabPalette.buttonText);
        };
        draw_tab(homeTab,"Home",ViewMode::Home);
        draw_tab(videoPlayerTab,"Video Player",ViewMode::VideoPlayer);
        draw_tab(libraryTab,"Library",ViewMode::Library);
        draw_tab(discoverTab,"Discover",ViewMode::Discover);
        draw_tab(liveTvTab,"Live TV",ViewMode::LiveTV);
        draw_tab(nougatTab,"Search",ViewMode::Nougat);
        draw_tab(ytdlpTab,"Stream",ViewMode::Stream);
        draw_tab(studioTab,"Studio",ViewMode::Studio);
        draw_tab(debugTab,"System",ViewMode::Debug);
        XSetClipMask(d, gc, None);
    }

    void draw_active_top_tab_pointer(Drawable target) {
        const Rect* tab = nullptr;
        switch (currentView) {
            case ViewMode::Home: tab = &homeTab; break;
            case ViewMode::VideoPlayer: tab = &videoPlayerTab; break;
            case ViewMode::Library: tab = &libraryTab; break;
            case ViewMode::Discover: tab = &discoverTab; break;
            case ViewMode::LiveTV: tab = &liveTvTab; break;
            case ViewMode::Nougat: tab = &nougatTab; break;
            case ViewMode::Stream: tab = &ytdlpTab; break;
            case ViewMode::Studio: tab = &studioTab; break;
            case ViewMode::Debug: tab = &debugTab; break;
            case ViewMode::P2P: return;
        }
        if (!tab) return;
        const int cx = tab->x + tab->w / 2;
        if (cx < topNavClipX || cx >= topNavClipRight) return;
        const ViewPalette palette = palette_for(currentView);
        const int baseY = tab->y + tab->h - 2;
        const int tipY = tab->y + tab->h + kTopTabPointerH;
        const int half = kTopTabPointerHalfW;
        XPoint outer[5] = {
            {static_cast<short>(cx - half), static_cast<short>(baseY - 2)},
            {static_cast<short>(cx - 9), static_cast<short>(baseY + 2)},
            {static_cast<short>(cx), static_cast<short>(tipY)},
            {static_cast<short>(cx + 9), static_cast<short>(baseY + 2)},
            {static_cast<short>(cx + half), static_cast<short>(baseY - 2)}
        };
        XSetForeground(d, gc, palette.buttonDark);
        XFillPolygon(d, target, gc, outer, 5, Convex, CoordModeOrigin);
        XPoint inner[5] = {
            {static_cast<short>(cx - 12), static_cast<short>(baseY - 1)},
            {static_cast<short>(cx - 7), static_cast<short>(baseY + 2)},
            {static_cast<short>(cx), static_cast<short>(tipY - 3)},
            {static_cast<short>(cx + 7), static_cast<short>(baseY + 2)},
            {static_cast<short>(cx + 12), static_cast<short>(baseY - 1)}
        };
        XSetForeground(d, gc, palette.button);
        XFillPolygon(d, target, gc, inner, 5, Convex, CoordModeOrigin);
        line(target, cx - 7, baseY + 1, cx, tipY - 3, palette.buttonLight);
        line(target, cx, tipY - 3, cx + 7, baseY + 1, palette.buttonLight);
    }

    void update_chapter_marks(bool force=false) {
        (void)force;
        if (!mp) { chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false; return; }
        if (paused || chapterScanComplete) return;
        long long l = playback_length_ms();
        if (l <= 0) return;

        std::vector<long long> realMarks;
        std::vector<std::string> realNames;
        if (api.get_full_chapter_descriptions && api.chapter_descriptions_release) {
            int title = api.get_title ? api.get_title(mp) : -1;
            libvlc_chapter_description_t** chapters = nullptr;
            int count = api.get_full_chapter_descriptions(mp, title, &chapters);
            if (count > 0 && chapters) {
                for (int i=0; i<count; ++i) {
                    if (!chapters[i]) continue;
                    long long offset = chapters[i]->i_time_offset;
                    if (offset >= 0 && offset < l) {
                        realMarks.push_back(offset);
                        std::string name = chapters[i]->psz_name ? chapters[i]->psz_name : "";
                        if (name.empty()) { char b[40]; snprintf(b, sizeof(b), "Chapter %d", i+1); name = b; }
                        realNames.push_back(name);
                    }
                }
                api.chapter_descriptions_release(chapters, static_cast<unsigned>(count));
            }
        }
        if (realMarks.size() > 1) {
            chapterMarksMs = realMarks;
            chapterNames = realNames;
            chapterMarksAreReal = true;
            chapterScanComplete = true;
            return;
        }

        chapterMarksMs.clear();
        chapterNames.clear();
        chapterMarksAreReal = false;
        long long chapterEveryMs = 300000;
        if (l > 7200000) chapterEveryMs = 900000;
        else if (l > 3600000) chapterEveryMs = 600000;
        for (long long markMs = chapterEveryMs; markMs < l; markMs += chapterEveryMs) {
            chapterMarksMs.push_back(markMs);
            chapterNames.push_back("Default mark");
        }
        chapterScanComplete = true;
    }
    void jump_to_chapter_index(int idx) {
        if (!mp || !chapterMarksAreReal || idx < 0 || idx >= (int)chapterMarksMs.size()) return;
        api.set_time(mp, chapterMarksMs[(size_t)idx]);
        cachedPlaybackTimeMs = chapterMarksMs[(size_t)idx];
        playbackCacheValid = true;
        draw_seek_time_only();
    }
    int current_chapter_index() {
        if (!mp || !chapterMarksAreReal || chapterMarksMs.empty()) return -1;
        long long t = playback_time_ms();
        int idx = 0;
        for (size_t i=0; i<chapterMarksMs.size(); ++i) if (chapterMarksMs[i] <= t) idx = (int)i;
        return idx;
    }
    void previous_chapter() {
        if (!chapterMarksAreReal) return;
        int idx = current_chapter_index();
        long long t = mp ? playback_time_ms() : 0;
        if (idx > 0 && idx < (int)chapterMarksMs.size() && t - chapterMarksMs[(size_t)idx] < 3000) idx--;
        jump_to_chapter_index(std::max(0, idx));
    }
    void next_chapter() {
        if (!chapterMarksAreReal) return;
        int idx = current_chapter_index();
        jump_to_chapter_index(std::min((int)chapterMarksMs.size()-1, idx+1));
    }

    std::string seek_preview_chapter_name(long long target_ms) const {
        if (!chapterMarksAreReal || chapterMarksMs.empty()) return {};
        int index = -1;
        for (std::size_t i=0; i<chapterMarksMs.size(); ++i) {
            if (chapterMarksMs[i] <= target_ms) index = static_cast<int>(i);
            else break;
        }
        if (index < 0 || index >= static_cast<int>(chapterNames.size())) return {};
        return chapterNames[static_cast<std::size_t>(index)];
    }

    void clear_seek_preview_hover() {
        if (!seekPreviewHover && !seekPreviewWindow) return;
        seekPreviewHover = false;
        seekPreviewHoverStartedMs = 0;
        seekPreviewTargetMs = 0;
        {
            std::lock_guard<std::mutex> lock(seekPreviewState->mutex);
            ++seekPreviewState->generation;
            seekPreviewState->has_frame = false;
            seekPreviewState->updated = false;
        }
        if (seekPreviewWindow) XUnmapWindow(d, seekPreviewWindow);
    }

    void update_seek_preview_hover(int x, int y) {
        if (currentView != ViewMode::VideoPlayer || fullscreen || !mp || !seekRect.contains(x,y)) {
            clear_seek_preview_hover();
            return;
        }
        const long long length = playback_length_ms();
        if (length <= 0) { clear_seek_preview_hover(); return; }
        const long long target = std::max<long long>(0, std::min<long long>(length,
            static_cast<long long>((static_cast<double>(x - seekRect.x) / std::max(1,seekRect.w)) * length)));
        const long long quantized = (target / 5000LL) * 5000LL;
        if (!seekPreviewHover || quantized != seekPreviewTargetMs) {
            seekPreviewHover = true;
            seekPreviewHoverStartedMs = now_ms();
            seekPreviewTargetMs = quantized;
            std::lock_guard<std::mutex> lock(seekPreviewState->mutex);
            ++seekPreviewState->generation;
            seekPreviewState->path = currentPath;
            seekPreviewState->target_ms = quantized;
            const auto cached = seekPreviewState->cache.find(quantized);
            if (cached != seekPreviewState->cache.end()) {
                seekPreviewState->frame = cached->second;
                seekPreviewState->has_frame = true;
                seekPreviewState->updated = true;
            } else {
                seekPreviewState->has_frame = false;
            }
        }
        draw_seek_preview_window();
    }

    void poll_seek_preview() {
        if (!seekPreviewHover || currentView != ViewMode::VideoPlayer || fullscreen || currentPath.empty() || !exists_file(currentPath)) return;
        if (now_ms() - seekPreviewHoverStartedMs < 140) return;
        const long long target = seekPreviewTargetMs;
        std::string path;
        int generation = 0;
        {
            std::lock_guard<std::mutex> lock(seekPreviewState->mutex);
            if (seekPreviewState->has_frame || seekPreviewState->busy) return;
            const auto cached = seekPreviewState->cache.find(target);
            if (cached != seekPreviewState->cache.end()) {
                seekPreviewState->frame = cached->second;
                seekPreviewState->has_frame = true;
                seekPreviewState->updated = true;
                return;
            }
            seekPreviewState->busy = true;
            path = currentPath;
            generation = seekPreviewState->generation;
        }
        const std::shared_ptr<FramePreviewState> state = seekPreviewState;
        std::thread([state, path, target, generation]() {
            std::string bytes;
            reddmedia::LibraryPoster frame;
            std::string error;
            const bool ok = extract_video_frame_bmp(path, target, 320, 180, bytes) &&
                            reddmedia::decode_library_poster_bmp(bytes, frame, error);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (generation == state->generation && state->path == path && target == state->target_ms && ok) {
                state->frame = frame;
                state->cache[target] = std::move(frame);
                if (state->cache.size() > 48U) state->cache.erase(state->cache.begin());
                state->has_frame = true;
                state->updated = true;
            }
            state->busy = false;
        }).detach();
    }

    void poll_seek_preview_update() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(seekPreviewState->mutex);
            updated = seekPreviewState->updated;
            if (updated) seekPreviewState->updated = false;
        }
        if (updated) draw_seek_preview_window();
    }

    void draw_seek_preview_window() {
        if (!seekPreviewWindow || !seekPreviewHover || currentView != ViewMode::VideoPlayer || fullscreen) {
            if (seekPreviewWindow) XUnmapWindow(d, seekPreviewWindow);
            return;
        }
        const int popup_w = 260;
        const int popup_h = 176;
        const long long length = std::max<long long>(1, playback_length_ms());
        const double fraction = std::max(0.0, std::min(1.0, static_cast<double>(seekPreviewTargetMs) / static_cast<double>(length)));
        const int center_x = seekRect.x + static_cast<int>(seekRect.w * fraction);
        const int popup_x = std::max(8, std::min(W - popup_w - 8, center_x - popup_w / 2));
        const int popup_y = std::max(38, seekRect.y - popup_h - 8);
        XMoveResizeWindow(d, seekPreviewWindow, popup_x, popup_y, popup_w, popup_h);
        XMapRaised(d, seekPreviewWindow);
        fill(seekPreviewWindow, {0,0,popup_w,popup_h}, rgb8(48,31,24));
        fill_round(seekPreviewWindow, {3,3,popup_w-6,popup_h-6}, 9, rgb8(83,50,34));
        const Rect frame_area{8,8,popup_w-16,137};
        fill(seekPreviewWindow, frame_area, rgb8(10,10,10));
        reddmedia::LibraryPoster frame;
        bool has_frame = false;
        {
            std::lock_guard<std::mutex> lock(seekPreviewState->mutex);
            has_frame = seekPreviewState->has_frame;
            if (has_frame) frame = seekPreviewState->frame;
        }
        if (has_frame) draw_cover_pixels(seekPreviewWindow, frame_area, frame);
        else text(seekPreviewWindow, 82, 80, "LOADING PREVIEW", rgb8(235,220,199));
        std::string footer = format_time(seekPreviewTargetMs);
        const std::string chapter = seek_preview_chapter_name(seekPreviewTargetMs);
        if (!chapter.empty()) footer += "  •  " + chapter;
        text(seekPreviewWindow, 10, 164, head_to_width(footer, popup_w - 20), rgb8(247,236,217));
        XFlush(d);
    }

    void draw_seek_time_row(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        const unsigned long caramel = rgb8(170,91,24);
        const unsigned long caramelLight = rgb8(218,156,82);
        const unsigned long creamTrack = rgb8(247,236,217);
        const unsigned long trackBorder = rgb8(153,91,35);
        const unsigned long markDark = rgb8(121,88,56);
        const unsigned long markReal = rgb8(255,244,224);

        draw_quilted_background(target, {0, std::max(0, seekRect.y-7), W, seekRect.h+46}, ViewMode::VideoPlayer);

        long long t=0,l=0;
        if (mp) { t=playback_time_ms(); l=playback_length_ms(); }
        int pos = 0;
        int seekPercent = 0;
        if (l > 0) {
            update_chapter_marks(false);
            pos = std::max(0, std::min(seekRect.w, (int)((double)t / (double)l * seekRect.w)));
            seekPercent = std::max(0, std::min(100, static_cast<int>((t * 100 + l / 2) / l)));
        }

        if (sheetSeekLoaded) {
            // Pixel-derived from the literal approved SEEKBAR (PROGRESS)
            // component. v0.0.37 preserves the native end caps and knob while
            // stretching only the repeatable track spans to player width.
            draw_sheet_seek_frame(target, seekPercent);
        } else {
            // Narrow-window/missing-asset fallback. Normal owner geometry uses
            // the exact sheet-pixel sprite above at its native 378px width.
            draw_sheet_track(target, seekRect, trackBorder, creamTrack, rgb8(255,246,227));
            if (pos > 0) draw_sheet_track_fill(target, seekRect, pos, trackBorder, caramel, caramelLight);
            const int knobD = 26;
            const int knobCenterX = std::max(seekRect.x + knobD / 2,
                std::min(seekRect.x + seekRect.w - knobD / 2, seekRect.x + pos));
            draw_sheet_knob(target, knobCenterX, seekRect.y + seekRect.h / 2, knobD,
                            trackBorder, rgb8(225,188,132), rgb8(249,222,177));
        }
        if (l > 0) {
            for (long long markMs : chapterMarksMs) {
                if (markMs <= 0 || markMs >= l) continue;
                int mx = seekRect.x + (int)((double)markMs / (double)l * seekRect.w);
                line(target, mx, seekRect.y+3, mx, seekRect.y + seekRect.h-3,
                     chapterMarksAreReal ? markReal : markDark);
            }
        }

        const std::string currentText = format_time(t);
        const std::string totalText = format_time(l);
        const int timeY = seekRect.y + seekRect.h / 2 + 5;
        const int sideGap = 12;
        text(target, std::max(10, seekRect.x - sideGap - text_width(currentText)),
             timeY, currentText, palette.text);
        text(target, std::min(W - text_width(totalText) - 10, seekRect.x + seekRect.w + sideGap),
             timeY, totalText, palette.text);
    }

    bool load_sheet_seek_frames() {
        const std::string path = exe_dir() + "/assets/ui/nougat_seek_sheet_frames.bin";
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        char magic[8] = {};
        std::uint32_t width=0, height=0, frames=0, channels=0;
        in.read(magic, 8);
        in.read(reinterpret_cast<char*>(&width), sizeof(width));
        in.read(reinterpret_cast<char*>(&height), sizeof(height));
        in.read(reinterpret_cast<char*>(&frames), sizeof(frames));
        in.read(reinterpret_cast<char*>(&channels), sizeof(channels));
        if (!in || std::memcmp(magic, "NSEEKSP1", 8) != 0 ||
            width != kSheetSeekW || height != kSheetSeekSpriteH ||
            frames != kSheetSeekFrames || channels != 4U) return false;
        const std::size_t expected = static_cast<std::size_t>(width) * height * frames * channels;
        sheetSeekRgba.assign(expected, 0);
        in.read(reinterpret_cast<char*>(sheetSeekRgba.data()), static_cast<std::streamsize>(expected));
        if (!in || static_cast<std::size_t>(in.gcount()) != expected) { sheetSeekRgba.clear(); return false; }
        return true;
    }

    void draw_sheet_seek_frame(Drawable target, int percent) {
        if (!sheetSeekLoaded || sheetSeekRgba.empty() || seekRect.w <= 0) return;
        percent = std::max(0, std::min(100, percent));
        const std::size_t frameBytes = static_cast<std::size_t>(kSheetSeekW) * kSheetSeekSpriteH * 4U;
        const unsigned char* source = sheetSeekRgba.data() + frameBytes * static_cast<std::size_t>(percent);
        const int targetW = seekRect.w;
        const int targetH = kSheetSeekSpriteH;
        const int cap = 14;
        const int knobHalf = 14;
        const int srcTrackStart = cap;
        const int srcTrackEnd = kSheetSeekW - cap;
        const int dstTrackStart = cap;
        const int dstTrackEnd = std::max(cap + 1, targetW - cap);
        const int srcCenter = srcTrackStart + (srcTrackEnd - srcTrackStart) * percent / 100;
        const int dstCenter = dstTrackStart + (dstTrackEnd - dstTrackStart) * percent / 100;

        std::vector<unsigned char> scaled(static_cast<std::size_t>(targetW) * targetH * 4U, 0U);
        const auto map_segment = [](int x, int dl, int dr, int sl, int sr) {
            if (dr <= dl || sr <= sl) return sl;
            const long long num = static_cast<long long>(x - dl) * (sr - sl);
            return sl + static_cast<int>(num / (dr - dl));
        };
        for (int y=0; y<targetH; ++y) {
            for (int x=0; x<targetW; ++x) {
                int sx=0;
                if (x < cap) sx = std::min(kSheetSeekW-1,x);
                else if (x >= targetW-cap) sx = std::max(0,kSheetSeekW-(targetW-x));
                else if (x >= dstCenter-knobHalf && x <= dstCenter+knobHalf) {
                    sx = srcCenter + (x-dstCenter);
                } else if (x < dstCenter-knobHalf) {
                    sx = map_segment(x,cap,std::max(cap+1,dstCenter-knobHalf),
                                     cap,std::max(cap+1,srcCenter-knobHalf));
                } else {
                    sx = map_segment(x,std::min(targetW-cap-1,dstCenter+knobHalf),targetW-cap,
                                     std::min(kSheetSeekW-cap-1,srcCenter+knobHalf),kSheetSeekW-cap);
                }
                sx=std::max(0,std::min(kSheetSeekW-1,sx));
                const std::size_t si=(static_cast<std::size_t>(y)*kSheetSeekW+static_cast<std::size_t>(sx))*4U;
                const std::size_t di=(static_cast<std::size_t>(y)*targetW+static_cast<std::size_t>(x))*4U;
                scaled[di]=source[si]; scaled[di+1U]=source[si+1U]; scaled[di+2U]=source[si+2U]; scaled[di+3U]=source[si+3U];
            }
        }

        const int depth = DefaultDepth(d, screen);
        const int bytesPerPixel = depth <= 16 ? 2 : 4;
        char* imageData = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(targetW) * targetH, static_cast<std::size_t>(bytesPerPixel)));
        if (!imageData) return;
        for (int y=0; y<targetH; ++y) {
            for (int x=0; x<targetW; ++x) {
                const std::size_t si=(static_cast<std::size_t>(y)*targetW+static_cast<std::size_t>(x))*4U;
                const unsigned long pixel=visual_pixel(scaled[si],scaled[si+1U],scaled[si+2U]);
                std::memcpy(imageData +
                    (static_cast<std::size_t>(y)*targetW+static_cast<std::size_t>(x))*static_cast<std::size_t>(bytesPerPixel),
                    &pixel, static_cast<std::size_t>(bytesPerPixel));
            }
        }
        XImage* image=XCreateImage(d,DefaultVisual(d,screen),depth,ZPixmap,0,imageData,targetW,targetH,32,0);
        if (!image) { std::free(imageData); return; }
        const int spriteY = seekRect.y - (targetH - kSheetSeekH) / 2;

        // X11 ZPixmap has no alpha compositing. Copy only solid interior runs.
        // The old >=128 span conversion promoted translucent anti-alias pixels
        // into an opaque white halo. A solid threshold plus boundary-white
        // suppression keeps the sheet model while removing that rectangle/halo.
        for (int row=0; row<targetH; ++row) {
            int run=-1;
            for (int x=0; x<=targetW; ++x) {
                bool opaque=false;
                if (x<targetW) {
                    const std::size_t i=(static_cast<std::size_t>(row)*targetW+static_cast<std::size_t>(x))*4U;
                    const unsigned a=scaled[i+3U];
                    opaque=a>=220U;
                    if (opaque && scaled[i]>238U && scaled[i+1U]>238U && scaled[i+2U]>232U) {
                        bool exterior=false;
                        for (int oy=-1;oy<=1 && !exterior;++oy) for (int ox=-1;ox<=1;++ox) {
                            const int nx=x+ox, ny=row+oy;
                            if (nx<0||nx>=targetW||ny<0||ny>=targetH) { exterior=true; break; }
                            const std::size_t ni=(static_cast<std::size_t>(ny)*targetW+static_cast<std::size_t>(nx))*4U;
                            if (scaled[ni+3U]<80U) { exterior=true; break; }
                        }
                        if (exterior) opaque=false;
                    }
                }
                if (opaque && run<0) run=x;
                if ((!opaque || x==targetW) && run>=0) {
                    const int last=x-1;
                    XPutImage(d,target,gc,image,run,row,seekRect.x+run,spriteY+row,
                              static_cast<unsigned>(last-run+1),1);
                    run=-1;
                }
            }
        }
        XDestroyImage(image);
    }

    bool load_sheet_volume_frames() {
        const std::string path = exe_dir() + "/assets/ui/nougat_volume_sheet_frames.bin";
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        char magic[8] = {};
        std::uint32_t width=0, height=0, frames=0, channels=0;
        in.read(magic, 8);
        in.read(reinterpret_cast<char*>(&width), sizeof(width));
        in.read(reinterpret_cast<char*>(&height), sizeof(height));
        in.read(reinterpret_cast<char*>(&frames), sizeof(frames));
        in.read(reinterpret_cast<char*>(&channels), sizeof(channels));
        if (!in || std::memcmp(magic, "NVOLSPR1", 8) != 0 ||
            width != kSheetVolumeW || height != kSheetVolumeH ||
            frames != kSheetVolumeFrames || channels != 3U) return false;
        const std::size_t expected = static_cast<std::size_t>(width) * height * frames * channels;
        sheetVolumeRgb.assign(expected, 0);
        in.read(reinterpret_cast<char*>(sheetVolumeRgb.data()), static_cast<std::streamsize>(expected));
        if (!in || static_cast<std::size_t>(in.gcount()) != expected) { sheetVolumeRgb.clear(); return false; }
        return true;
    }

    void draw_sheet_volume_frame(Drawable target, int volume) {
        if (!sheetVolumeLoaded || sheetVolumeRgb.empty()) return;
        volume = std::max(0, std::min(200, volume));
        const std::size_t frameBytes = static_cast<std::size_t>(kSheetVolumeW) * kSheetVolumeH * 3U;
        const unsigned char* source = sheetVolumeRgb.data() + frameBytes * static_cast<std::size_t>(volume);
        const int depth = DefaultDepth(d, screen);
        const int bytesPerPixel = depth <= 16 ? 2 : 4;
        char* imageData = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(kSheetVolumeW) * kSheetVolumeH,
            static_cast<std::size_t>(bytesPerPixel)));
        if (!imageData) return;
        for (int y=0; y<kSheetVolumeH; ++y) {
            for (int x=0; x<kSheetVolumeW; ++x) {
                const std::size_t si=(static_cast<std::size_t>(y)*kSheetVolumeW+static_cast<std::size_t>(x))*3U;
                const unsigned long pixel=visual_pixel(source[si],source[si+1U],source[si+2U]);
                std::memcpy(imageData +
                    (static_cast<std::size_t>(y)*kSheetVolumeW+static_cast<std::size_t>(x))*static_cast<std::size_t>(bytesPerPixel),
                    &pixel, static_cast<std::size_t>(bytesPerPixel));
            }
        }
        XImage* image=XCreateImage(d,DefaultVisual(d,screen),depth,ZPixmap,0,imageData,
                                   kSheetVolumeW,kSheetVolumeH,32,0);
        if (!image) { std::free(imageData); return; }
        const int cornerRadius = 11;
        for (int row=1; row<kSheetVolumeH-1; ++row) {
            const int edgeRow = std::min(row, kSheetVolumeH - 1 - row);
            const int roundedInset = edgeRow >= cornerRadius ? 0 : rounded_top_inset_for_row(edgeRow, cornerRadius);
            const int inset = std::max(1, roundedInset + 1);
            const int width = std::max(0, kSheetVolumeW - inset * 2);
            if (width <= 0) continue;
            XPutImage(d,target,gc,image,inset,row,
                      volumeHousingRect.x+inset,volumeHousingRect.y+row,
                      static_cast<unsigned>(width),1);
        }
        XDestroyImage(image);
    }

    void draw_sheet_volume_housing(Drawable target, const Rect& r, const ViewPalette& palette) {
        const unsigned long cream = rgb8(244, 231, 205);
        const unsigned long border = rgb8(166, 112, 56);
        Rect shadow{r.x, r.y + 3, r.w, r.h};
        fill_round(target, shadow, 8, rgb8(181, 145, 104));
        fill_round(target, r, 8, cream);
        outline_round(target, r, 8, border);
        Rect inner{r.x + 2, r.y + 2, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        outline_round(target, inner, 6, rgb8(255, 246, 227));
        line(target, r.x + 9, r.y + 3, r.x + r.w - 10, r.y + 3, rgb8(255, 248, 234));
        line(target, r.x + 9, r.y + r.h - 3, r.x + r.w - 10, r.y + r.h - 3, rgb8(191, 151, 105));
        (void)palette;
    }

    void draw_volume_bar(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        const unsigned long caramel = rgb8(170, 91, 24);
        const unsigned long caramelLight = rgb8(218, 156, 82);
        const unsigned long creamTrack = rgb8(247, 236, 217);
        const unsigned long trackBorder = rgb8(153, 91, 35);

        const int y0 = std::max(0, volumeHousingRect.y - 21);
        draw_quilted_background(target, {0, y0, W, volumeHousingRect.h + 27}, ViewMode::VideoPlayer);

        int vol = mp ? api.get_volume(mp) : volumePercent;
        if (vol < 0) vol = volumePercent;
        vol = std::max(0,std::min(200,vol));
        volumePercent = vol;

        // Actual sheet label is above the housing, left aligned.
        text(target, volumeHousingRect.x + 8, volumeHousingRect.y - 5, "VOLUME", palette.text);
        if (sheetVolumeLoaded) {
            draw_sheet_volume_frame(target, vol);
        } else {
            // Safety fallback only. The v0.0.35 repair contract requires the
            // sheet-derived asset to be installed, so normal builds never use
            // this procedural path.
            draw_sheet_volume_housing(target, volumeHousingRect, palette);
            draw_sheet_track(target, volRect, trackBorder, creamTrack, rgb8(255,246,227));
            const int filledW = std::max(0,std::min(volRect.w,volRect.w*vol/200));
            if (filledW > 0) draw_sheet_track_fill(target, volRect, filledW, trackBorder, caramel, caramelLight);
            const int knobD = 26;
            const int knobCenterX = std::max(volRect.x + knobD / 2,
                std::min(volRect.x + volRect.w - knobD / 2, volRect.x + filledW));
            draw_sheet_knob(target, knobCenterX, volRect.y + volRect.h / 2, knobD,
                            trackBorder, rgb8(225,188,132), rgb8(249,222,177));
            const unsigned long icon = rgb8(119, 63, 22);
            draw_speaker_icon(target, volumeHousingRect.x + 14, volumeHousingRect.y + 14, false, icon);
            draw_speaker_icon(target, volumeHousingRect.x + volumeHousingRect.w - 34, volumeHousingRect.y + 14, true, icon);
        }
        text(target, volumeHousingRect.x + volumeHousingRect.w + 10, volumeHousingRect.y + 27,
             std::to_string(vol) + "%", rgb8(166, 95, 28));
    }

    bool episode_navigation_available(int delta) const {
        if (!tvAutoplayArmed || tvAutoplayIndex < 0 || delta == 0) return false;
        const int target = tvAutoplayIndex + delta;
        return target >= 0 && target < static_cast<int>(tvAutoplayQueue.size()) &&
               exists_file(tvAutoplayQueue[static_cast<std::size_t>(target)].path);
    }

    void play_relative_episode(int delta) {
        if (delta == 0) return;
        if ((!tvAutoplayArmed || tvAutoplayIndex < 0) && activeLibraryItemValid &&
            activeLibraryItem.kind == reddmedia::LibraryNodeKind::Episode) {
            prepare_tv_autoplay(activeLibraryItem);
        }
        if (!episode_navigation_available(delta)) return;
        start_tv_autoplay_index(tvAutoplayIndex + delta);
        if (!fullscreen) redraw();
    }

    void draw_controls(Drawable target) {
        draw_top_bar(target);
        if (currentView != ViewMode::VideoPlayer) return;
        // v0.0.29 player surround: the Video Player page background itself
        // continues uniformly around all four sides of the video child.  Do not
        // draw a separate partial brown rail/matte around only part of the frame.
        draw_quilted_background(target, {0, kTopBarH, W, std::max(1, H - kTopBarH)}, ViewMode::VideoPlayer);
        button_on(target, openBtn, "Open");
        button_on(target, rewindBtn, "Rewind 10s");
        if (episode_navigation_available(-1)) button_on(target, previousBtn, "Previous");
        else draw_disabled_player_button(target, previousBtn, "Previous");
        button_on(target, playBtn, "Play/Pause");
        if (episode_navigation_available(1)) button_on(target, nextBtn, "Next");
        else draw_disabled_player_button(target, nextBtn, "Next");
        button_on(target, forwardBtn, "Fast Forward 10s");
        button_on(target, stopBtn, "Stop");
        button_on(target, fsBtn, "Fullscreen");
        const int titleStripY = std::max(kTopBarH + 8, seekRect.y - 30);
        draw_quilted_background(target, {10, titleStripY, std::max(1, W - 20), 20}, ViewMode::VideoPlayer);
        const std::string identity = current_media_identity();
        if (!identity.empty()) text(target, 16, titleStripY + 15, head_to_width(identity, W - 32), palette_for(ViewMode::VideoPlayer).text);
        draw_seek_time_row(target);
        draw_volume_bar(target);
    }

    bool loading_state(double& progress, bool& determinate, std::string& label) {
        progress = 0.0;
        determinate = false;
        label.clear();
        if (currentView == ViewMode::Home) {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            if (homeState->busy) { progress = homeState->progress; label = "Loading..."; return true; }
        }
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            if (serverState->busy) {
                progress = serverState->progress;
                determinate = serverState->progress_determinate;
                label = serverState->progress_label;
                return true;
            }
        }
        if (currentView == ViewMode::Library) {
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                if (libraryState->busy) {
                    progress = libraryState->progress;
                    determinate = libraryState->progress_determinate;
                    label = libraryState->progress_label;
                    return true;
                }
            }
            {
                std::lock_guard<std::mutex> lock(posterState->mutex);
                if (posterState->busy) {
                    progress = posterState->progress;
                    determinate = posterState->progress_determinate;
                    label = posterState->progress_label;
                    return true;
                }
            }
        }
        if (currentView == ViewMode::Discover) {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) { progress = discoverState->progress; label = "Working..."; return true; }
        }
        if (currentView == ViewMode::Debug) {
            {
                std::lock_guard<std::mutex> lock(debugState->mutex);
                if (debugState->busy) { progress = debugState->progress; label = "Checking..."; return true; }
            }
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                if (libraryState->busy) {
                    progress = libraryState->progress;
                    determinate = libraryState->progress_determinate;
                    label = libraryState->progress_label;
                    return true;
                }
            }
            {
                std::lock_guard<std::mutex> lock(posterState->mutex);
                if (posterState->busy) {
                    progress = posterState->progress;
                    determinate = posterState->progress_determinate;
                    label = posterState->progress_label;
                    return true;
                }
            }
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                if (discoverState->busy) { progress = discoverState->progress; label = "Working..."; return true; }
            }
        }
        return false;
    }

    void draw_loading_bar(Drawable target) {
        double progress = 0.0;
        bool determinate = false;
        std::string label;
        if (!loading_state(progress, determinate, label)) return;

        // Keep the activity surface below the top-tab notch.  The old y=27 bar
        // painted over the selected Home/Library/Debug arrow while work was active.
        const int strip_y = kTopBarH;
        const int label_w = determinate ? 58 : 0;
        const int track_w = std::max(1, W - label_w);
        const Rect track = {2, strip_y + 1, std::max(1, track_w - 4), 10};
        const unsigned long trackBorder = rgb8(166,112,56);
        const unsigned long creamTrack = rgb8(244,229,205);
        const unsigned long caramel = rgb8(184,111,43);
        draw_sheet_track(target, track, trackBorder, creamTrack, rgb8(255,246,227));
        if (determinate) {
            progress = std::max(0.0, std::min(1.0, progress));
            const int loaded = std::max(progress > 0.0 ? 2 : 0,
                                        std::min(track.w, static_cast<int>(progress * track.w)));
            draw_sheet_track_fill(target, track, loaded, trackBorder, caramel, rgb8(224,173,105));
            const int percent = static_cast<int>(std::lround(progress * 100.0));
            Rect chip{track_w, strip_y, label_w, 18};
            draw_concept_field(target, chip, creamTrack, trackBorder, false);
            const std::string percent_text = std::to_string(percent) + "%";
            text(target, chip.x + std::max(4, (chip.w - text_width(percent_text)) / 2),
                 chip.y + 14, percent_text, rgb8(72,39,20));
        } else {
            const int chunk = std::max(80, track.w / 5);
            const int position = static_cast<int>((now_ms() / 8) % (track.w + chunk)) - chunk;
            const int begin = std::max(0, position);
            const int end = std::min(track.w, position + chunk);
            if (end > begin) draw_sheet_track_segment(target, track, begin, end - begin, trackBorder, caramel, rgb8(224,173,105));
        }
    }

    void draw_player_controls_only() {
        if (fullscreen || currentView != ViewMode::VideoPlayer) return;
        // v0.0.36 owner-visible repair: seek/time, VOLUME, percentage, and the
        // entire transport row are one repaint unit. The older independent
        // partial rectangles alternately clipped the top of the 47px sheet
        // VOLUME housing or the time text as pointer/drag updates arrived.
        // Keep a single stable region from above the seek row through the
        // bottom buttons and always remove any stale page clip before drawing.
        const int y0 = std::max(kTopBarH, seekRect.y - 10);
        const int h = std::max(0, H - y0);
        if (h <= 0) return;
        XSetClipMask(d, gc, None);
        Pixmap buffer = XCreatePixmap(d, win, W, H, DefaultDepth(d, screen));
        draw_quilted_background(buffer, {0, y0, W, h}, ViewMode::VideoPlayer);
        draw_seek_time_row(buffer);
        draw_volume_bar(buffer);
        button_on(buffer, openBtn, "Open");
        button_on(buffer, rewindBtn, "Rewind 10s");
        if (episode_navigation_available(-1)) button_on(buffer, previousBtn, "Previous");
        else draw_disabled_player_button(buffer, previousBtn, "Previous");
        button_on(buffer, playBtn, "Play/Pause");
        if (episode_navigation_available(1)) button_on(buffer, nextBtn, "Next");
        else draw_disabled_player_button(buffer, nextBtn, "Next");
        button_on(buffer, forwardBtn, "Fast Forward 10s");
        button_on(buffer, stopBtn, "Stop");
        button_on(buffer, fsBtn, "Fullscreen");
        XSetClipMask(d, gc, None);
        XCopyArea(d, buffer, win, gc, 0, y0, W, h, 0, y0);
        XFreePixmap(d, buffer);
        XFlush(d);
    }

    void draw_seek_time_only() {
        draw_player_controls_only();
    }

    void draw_volume_only() {
        draw_player_controls_only();
    }

    const char* stream_platform_name(StreamPlatform platform) const {
        switch (platform) {
            case StreamPlatform::YouTube: return "YouTube";
            case StreamPlatform::Vimeo: return "Vimeo";
            case StreamPlatform::Rumble: return "Rumble";
            case StreamPlatform::RuTube: return "RuTube";
            case StreamPlatform::VK: return "VK";
            case StreamPlatform::OK: return "OK";
        }
        return "Stream";
    }

    std::string stream_platform_home(StreamPlatform platform) const {
        switch (platform) {
            case StreamPlatform::YouTube: return "https://www.youtube.com/";
            case StreamPlatform::Vimeo: return "https://vimeo.com/";
            case StreamPlatform::Rumble: return "https://rumble.com/";
            case StreamPlatform::RuTube: return "https://rutube.ru/";
            case StreamPlatform::VK: return "https://vk.com/video";
            case StreamPlatform::OK: return "https://ok.ru/video";
        }
        return {};
    }

    bool detect_stream_platform_from_url(const std::string& url, StreamPlatform& detected) const {
        const std::string lower = lower_copy(url);
        if (lower.find("youtube.com/") != std::string::npos ||
            lower.find("youtu.be/") != std::string::npos) {
            detected = StreamPlatform::YouTube; return true;
        }
        if (lower.find("vimeo.com/") != std::string::npos ||
            lower.find("player.vimeo.com/") != std::string::npos) {
            detected = StreamPlatform::Vimeo; return true;
        }
        if (lower.find("rumble.com/") != std::string::npos) {
            detected = StreamPlatform::Rumble; return true;
        }
        if (lower.find("rutube.ru/") != std::string::npos) {
            detected = StreamPlatform::RuTube; return true;
        }
        if (lower.find("vk.com/") != std::string::npos ||
            lower.find("vkvideo.ru/") != std::string::npos) {
            detected = StreamPlatform::VK; return true;
        }
        if (lower.find("ok.ru/") != std::string::npos) {
            detected = StreamPlatform::OK; return true;
        }
        return false;
    }

    void sync_stream_platform_from_url() {
        StreamPlatform detected = streamPlatform;
        if (detect_stream_platform_from_url(ytdlpUrl, detected)) streamPlatform = detected;
    }

    void select_stream_platform(StreamPlatform platform) {
        if (streamPlatform != platform) push_navigation_history();
        streamPlatform = platform;
        urlFocused = false;
        urlSelectAll = false;
        ytdlpStatus = std::string(stream_platform_name(platform)) + " selected. The URL box is shared across Stream.";
        redraw();
    }

    void open_stream_webpage() {
        if (!ytdlpUrl.empty()) sync_stream_platform_from_url();
        const std::string target = ytdlpUrl.empty() ? stream_platform_home(streamPlatform) : ytdlpUrl;
        if (target.empty()) return;
        launch_external_target(target);
        ytdlpStatus = ytdlpUrl.empty()
            ? std::string("Opened ") + stream_platform_name(streamPlatform) + " homepage in your default browser."
            : std::string("Opened the URL in your default browser.");
        redraw();
    }

    void direct_watch_stream() {
        if (ytdlpUrl.empty()) {
            ytdlpStatus = "Paste a video URL first.";
            redraw();
            return;
        }
        sync_stream_platform_from_url();
        ytdlpStatus = std::string("Direct Watch: opening ") + stream_platform_name(streamPlatform) +
                      " in Nougat Media Suite's native player...";
        redraw();
        start_ytdlp_play();
    }

    void draw_stream_screen(Drawable target) {
        const ViewPalette palette = stream_palette_for(streamPlatform);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Stream);
        const auto source_button = [&](const Rect& r, const char* label, StreamPlatform platform) {
            const bool selected = streamPlatform == platform;
            const bool hover = r.contains(pointerWindowX, pointerWindowY);
            const ViewPalette own = stream_palette_for(platform);
            draw_sheet_tab_surface(target, r, own, selected, hover);
            const Rect visual{r.x+2,r.y+1,std::max(1,r.w-4),std::max(1,r.h-4)};
            text(target, visual.x + std::max(6,(visual.w-text_width(label))/2),
                 visual.y + visual.h / 2 + 5, label, own.buttonText);
        };
        source_button(streamYoutubeTab,"YouTube",StreamPlatform::YouTube);
        source_button(streamVimeoTab,"Vimeo",StreamPlatform::Vimeo);
        source_button(streamRumbleTab,"Rumble",StreamPlatform::Rumble);
        source_button(streamRutubeTab,"RuTube",StreamPlatform::RuTube);
        source_button(streamVkTab,"VK",StreamPlatform::VK);
        source_button(streamOkTab,"OK",StreamPlatform::OK);

        const unsigned long focusBorder = urlFocused ? palette.accent : palette.border;
        draw_concept_field(target, ytdlpUrlRect, palette.field, focusBorder, urlFocused);
        int urlTextMax = std::max(24, ytdlpUrlRect.w - 18);
        std::string visibleUrl = ytdlpUrl.empty() ? std::string("") : tail_to_width(ytdlpUrl, urlTextMax);
        XRectangle urlClip{(short)(ytdlpUrlRect.x+5),(short)(ytdlpUrlRect.y+2),
                           (unsigned short)std::max(1,ytdlpUrlRect.w-10),(unsigned short)std::max(1,ytdlpUrlRect.h-4)};
        XSetClipRectangles(d, gc, 0, 0, &urlClip, 1, Unsorted);
        const unsigned long fieldInk = col(0x1717,0x1111,0x0b0b);
        if (visibleUrl.empty() && !urlFocused) {
            text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y+18,
                 "Paste URL Then Press Direct Watch / Vimeo / Rumble / RuTube / VK / OK", palette.muted);
        } else if (urlSelectAll && !visibleUrl.empty()) {
            int selectedW = std::min(text_width(visibleUrl)+4,std::max(1,ytdlpUrlRect.w-12));
            fill(target,{ytdlpUrlRect.x+6,ytdlpUrlRect.y+4,selectedW,ytdlpUrlRect.h-8},palette.selection);
            text(target,ytdlpUrlRect.x+8,ytdlpUrlRect.y+18,visibleUrl,palette.buttonText);
        } else {
            text(target,ytdlpUrlRect.x+8,ytdlpUrlRect.y+18,visibleUrl,fieldInk);
        }
        if (urlFocused && !urlSelectAll) {
            int cx=ytdlpUrlRect.x+8+text_width(visibleUrl);
            cx=std::min(cx,ytdlpUrlRect.x+ytdlpUrlRect.w-8);
            line(target,cx,ytdlpUrlRect.y+5,cx,ytdlpUrlRect.y+23,fieldInk);
        }
        apply_page_clip(ViewMode::Stream);

        draw_concept_field(target,ytdlpOutputRect,palette.field,palette.border,false);
        text(target,ytdlpOutputRect.x+8,ytdlpOutputRect.y+18,
             tail_to_width("Output folder: "+ytdlpOutputFolder,ytdlpOutputRect.w-16),fieldInk);
        button_on(target,ytdlpDownloadBtn,"Download");
        button_on(target,ytdlpDirectWatchBtn,"Direct Watch");
        button_on(target,ytdlpWebpageBtn,"Open Webpage");
        button_on(target,ytdlpClearBtn,"Clear Log");
        text(target,28,246,head_to_width("Status: "+ytdlpStatus,W-56),palette.text);

        Rect logBox={28,264,std::max(240,W-56),std::max(100,H-289)};
        // Match Discover's clean panel silhouette: the provider palette remains in
        // the sheet-style panel/bottom bevel, but no vertical accent strip climbs
        // the left edge. This applies uniformly to YouTube, Vimeo, Rumble, RuTube, VK and OK.
        draw_primary_panel(target, logBox, palette);
        text(target,logBox.x+14,logBox.y+20,std::string(stream_platform_name(streamPlatform))+" activity log",palette.text);
        int lineY=logBox.y+44;
        std::istringstream iss(ytdlpLog);
        std::string lineText;
        std::vector<std::string> lines;
        while(std::getline(iss,lineText)) lines.push_back(lineText);
        int maxLines=std::max(1,(logBox.h-52)/18);
        int first=std::max(0,(int)lines.size()-maxLines);
        for(int i=first;i<(int)lines.size() && lineY<logBox.y+logBox.h-8;++i) {
            text(target,logBox.x+14,lineY,head_to_width(lines[(size_t)i],logBox.w-24),palette.muted);
            lineY+=18;
        }
    }

    std::string format_bytes(std::int64_t bytes) {
        const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
        double value = bytes < 0 ? 0.0 : static_cast<double>(bytes);
        int unit = 0;
        while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
        char out[64];
        if (unit == 0) snprintf(out, sizeof(out), "%.0f %s", value, units[unit]);
        else snprintf(out, sizeof(out), "%.1f %s", value, units[unit]);
        return out;
    }
    void auto_select_single_video() {
        if (p2p.selected_file() >= 0) return;
        std::vector<P2PFileInfo> fs = p2p.files();
        int videoIndex = -1;
        int videoCount = 0;
        for (const P2PFileInfo& f : fs) if (f.video) { videoIndex = f.index; ++videoCount; }
        if (videoCount == 1) {
            std::string error;
            if (p2p.select_file(videoIndex, error)) p2pUiStatus = "Video selected automatically.";
        }
    }
    void draw_p2p_screen(Drawable target) {
        const bool embeddedInSearch = currentView == ViewMode::Nougat;
        const ViewPalette palette = palette_for(embeddedInSearch ? ViewMode::Nougat : ViewMode::P2P);
        const unsigned long dark = palette.text;
        const unsigned long fieldInk = col(0x0d0d,0x1b1b,0x2a2a);
        unsigned long border = p2pMagnetFocused ? col(0x7070,0xb0b0,0xdada) : palette.border;
        if (embeddedInSearch) draw_quilted_background(target, {0,86,W,H-86}, ViewMode::Nougat);
        else fill(target, {0,86,W,H-86}, palette.background);
        text(target, 28, 108, "P2P STREAMING", dark);
        text(target, 28, 130, "Open a magnet or P2P metadata file, choose a video, then Play.", dark);
        if (embeddedInSearch) draw_concept_field(target, p2pMagnetRect, palette.field, palette.border, p2pMagnetFocused);
        else { fill(target, p2pMagnetRect, palette.field); outline(target, p2pMagnetRect, border); }
        text(target, p2pMagnetRect.x+8, p2pMagnetRect.y-7, "Magnet link", dark);
        const int textMax = std::max(24, p2pMagnetRect.w-18);
        std::string visible = p2pMagnet.empty() ? "" : tail_to_width(p2pMagnet, textMax);
        XRectangle clip{(short)(p2pMagnetRect.x+5),(short)(p2pMagnetRect.y+2),(unsigned short)std::max(1,p2pMagnetRect.w-10),(unsigned short)std::max(1,p2pMagnetRect.h-4)};
        XSetClipRectangles(d, gc, 0, 0, &clip, 1, Unsorted);
        if (visible.empty() && !p2pMagnetFocused) text(target,p2pMagnetRect.x+8,p2pMagnetRect.y+18,"paste magnet here",col(0x5555,0x5555,0x5555));
        else if (p2pMagnetSelectAll && !visible.empty()) {
            int selectedW=std::min(text_width(visible)+4,std::max(1,p2pMagnetRect.w-12));
            fill(target,{p2pMagnetRect.x+6,p2pMagnetRect.y+4,selectedW,p2pMagnetRect.h-8},col(0x3333,0x6666,0xaaaa));
            text(target,p2pMagnetRect.x+8,p2pMagnetRect.y+18,visible,col(0xffff,0xffff,0xffff));
        } else text(target,p2pMagnetRect.x+8,p2pMagnetRect.y+18,visible,fieldInk);
        if (p2pMagnetFocused && !p2pMagnetSelectAll) {
            int cx=p2pMagnetRect.x+8+text_width(visible);
            cx=std::min(cx,p2pMagnetRect.x+p2pMagnetRect.w-8);
            line(target,cx,p2pMagnetRect.y+5,cx,p2pMagnetRect.y+23,fieldInk);
        }
        apply_page_clip(embeddedInSearch ? ViewMode::Nougat : ViewMode::P2P);
        if (embeddedInSearch) draw_concept_field(target,p2pOutputRect,palette.field,palette.border,false);
        else { fill(target,p2pOutputRect,palette.field); outline(target,p2pOutputRect,palette.border); }
        text(target,p2pOutputRect.x+8,p2pOutputRect.y+18,tail_to_width("Download folder: "+p2pOutputFolder,p2pOutputRect.w-16),fieldInk);
        button_on(target,p2pLoadMagnetBtn,"Load Magnet");
        button_on(target,p2pOpenTorrentBtn,"Open P2P File");
        button_on(target,p2pPlayBtn,"Watch Now");
        button_on(target,p2pStopResumeBtn,p2p.is_paused()?"Resume":"Pause");
        button_on(target,p2pRemoveBtn,"Remove");
        const P2PPlusSettings plus=p2p.plus_settings();
        button_on(target,p2pSpeedBtn,plus.download_limit_kib>0?"Speed Limited":"Speed Unlimited");
        button_on(target,p2pSeedRulesBtn,plus.seed_ratio_limit>0.0?"Seed Rule On":"Seed Rule Off");
        button_on(target,p2pQueueUpBtn,"Queue Up");
        button_on(target,p2pQueueDownBtn,"Queue Down");
        button_on(target,p2pReannounceBtn,"Reannounce");
        button_on(target,p2pRecheckBtn,"Recheck");
        button_on(target,p2pPriorityBtn,p2pPriorityPreset==0?"Priority: Off":(p2pPriorityPreset>=6?"Priority: High":"Priority: Normal"));

        p2p.enforce_seed_rules();
        auto_select_single_video();
        P2PStatus st=p2p.status();
        int y=310;
        text(target,28,y,"Status: "+p2pUiStatus,dark); y+=20;
        if (st.active) {
            int pct=std::max(0,std::min(100,(int)(st.progress*100.0f + 0.5f)));
            text(target,28,y,"Transfer: "+(st.name.empty()?std::string("fetching metadata"):st.name),dark); y+=20;
            text(target,28,y,"State: "+st.state+"   Progress: "+std::to_string(pct)+"%   Downloaded: "+format_bytes(st.downloaded),dark); y+=20;
            const Rect progressTrack{28,y-10,std::max(180,W-56),12};
            draw_sheet_track(target,progressTrack,palette.border,palette.field,palette.buttonLight);
            draw_sheet_track_fill(target,progressTrack,progressTrack.w*pct/100,palette.buttonDark,palette.accent,palette.buttonLight);
            y+=18;
            text(target,28,y,"Down: "+format_bytes(st.download_rate)+"/s   Up: "+format_bytes(st.upload_rate)+"/s   Connected peers: "+std::to_string(st.peers)+"   Remote seeds: "+std::to_string(st.seeds),dark); y+=20;
            std::ostringstream plusLine; plusLine << std::fixed << std::setprecision(2) << st.share_ratio;
            text(target,28,y,"P2P Plus: ratio "+plusLine.str()+"   trackers "+std::to_string(st.tracker_count)+"   queue "+std::to_string(st.queue_position)+
                 "   limits D/U "+(st.download_limit>0?format_bytes(st.download_limit)+"/s":"∞")+" / "+(st.upload_limit>0?format_bytes(st.upload_limit)+"/s":"∞"),palette.muted); y+=20;
            const std::vector<P2PTrackerInfo> trackerRows=p2p.trackers();
            if (!trackerRows.empty()) {
                std::string trackerText="Trackers: ";
                const std::size_t show=std::min<std::size_t>(2,trackerRows.size());
                for(std::size_t i=0;i<show;++i) { if(i) trackerText+=" | "; trackerText+=trackerRows[i].url; if(!trackerRows[i].message.empty()) trackerText+=" ("+trackerRows[i].message+")"; }
                text(target,28,y,head_to_width(trackerText,W-56),palette.muted); y+=20;
            }
            if (st.seeding) {
                const std::string seedState = st.paused ? "Paused" : (st.upload_rate>0 ? "Uploading" : "Available, idle");
                text(target,28,y,"You: Seed ✓   Seeding: "+seedState+"   Known peers: "+std::to_string(st.known_peers)+"   Known remote seeds: "+std::to_string(st.known_seeds),dark); y+=20;
                text(target,28,y,"Swarm availability: your complete local copy = 1.00   Announce: "+std::string(st.announcing_trackers?"tracker ":"")+(st.announcing_dht?"DHT ":"")+(st.announcing_lsd?"LAN":""),palette.muted); y+=20;
            } else {
                std::ostringstream availability;
                availability << std::fixed << std::setprecision(2) << std::max(0.0f,st.swarm_availability);
                text(target,28,y,"Availability: "+availability.str()+"   Known peers: "+std::to_string(st.known_peers)+"   Known seeds: "+std::to_string(st.known_seeds),palette.muted); y+=20;
            }
            if (st.selected_size > 0) {
                const int selectedPct = std::max(0,std::min(100,(int)(st.selected_progress*100.0f + 0.5f)));
                text(target,28,y,"Selected media: "+std::to_string(selectedPct)+"%   Start buffer: "+format_bytes((std::int64_t)st.selected_buffered_bytes),dark); y+=20;
            } else if (!st.metadata_ready) {
                text(target,28,y,"Fetching metadata and waiting for the file list...",palette.muted); y+=20;
            }
            if (!st.error.empty()) { text(target,28,y,"P2P: "+st.error,col(0x9900,0,0)); y+=20; }
        }
        p2pFileRows.clear();
        std::vector<P2PFileInfo> fs=p2p.files();
        Rect fileBox={28,y,std::max(240,W-56),std::max(80,H-y-24)};
        if (embeddedInSearch) draw_nougat_panel(target,fileBox);
        else { fill(target,fileBox,palette.panel); outline(target,fileBox,palette.border); }
        text(target,fileBox.x+8,fileBox.y+19,"P2P files",dark);
        int rowY=fileBox.y+30;
        const int selected=p2p.selected_file();
        for (const P2PFileInfo& f:fs) {
            if (rowY+24>fileBox.y+fileBox.h) break;
            Rect row={fileBox.x+6,rowY,fileBox.w-12,24};
            if (f.index==selected) fill(target,row,palette.selection);
            outline(target,row,palette.border);
            std::string label=(f.video?"[video] ":"[file] ")+f.path+"  ("+format_bytes((std::int64_t)f.size)+")";
            text(target,row.x+6,row.y+17,tail_to_width(label,row.w-12),dark);
            p2pFileRows.push_back(row);
            rowY+=26;
        }
        if (fs.empty()) text(target,fileBox.x+8,fileBox.y+46,st.active?"Waiting for P2P metadata...":"Load a magnet or P2P metadata file.",palette.muted);
    }

    std::string server_control_label() const {
        return mediaServer.status_label();
    }

    void start_server_task(int operation) {
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            if (serverState->busy) return;
        }
        if (serverWorker.joinable()) serverWorker.join();
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            serverState->busy = true;
            serverState->updated = false;
            serverState->progress = 0.0;
            serverState->progress_determinate = false;
            serverState->progress_label = "Working...";
            serverState->state = reddmedia::MediaServerState::Starting;
            if (operation == 1) serverState->status = "Starting integrated server...";
            else if (operation == 2) serverState->status = "Stopping owned integrated server...";
            else serverState->status = "Refreshing server status (library metadata unchanged)...";
        }
        redraw();
        const std::shared_ptr<ServerUiState> state = serverState;
        serverWorker = std::thread([this, state, operation]() {
            if (operation == 1) {
                mediaServer.start();
                for (int attempt = 0; attempt < 100; ++attempt) {
                    mediaServer.refresh();
                    {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        state->progress = 0.08 + static_cast<double>(attempt) * 0.0085;
                    }
                    if (mediaServer.state() == reddmedia::MediaServerState::Ready ||
                        mediaServer.state() == reddmedia::MediaServerState::Fault ||
                        mediaServer.state() == reddmedia::MediaServerState::RuntimeMissing) break;
                    usleep(100000);
                }
            } else if (operation == 2) {
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->progress = 0.35;
                }
                mediaServer.stop();
            } else {
                mediaServer.refresh();
            }
            std::lock_guard<std::mutex> lock(state->mutex);
            state->status = server_control_label();
            state->state = mediaServer.state();
            state->owned = mediaServer.owns_server();
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
            state->progress_determinate = false;
            state->progress_label.clear();
        });
    }

    void poll_server_worker() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            updated = serverState->updated;
            if (updated) serverState->updated = false;
        }
        if (!updated) return;
        if (serverWorker.joinable()) serverWorker.join();
        if (!fullscreen) redraw();
    }

    std::string library_cache_key(int operation,
                                  reddmedia::LibraryMediaType media_type,
                                  const reddmedia::LibraryNode& parent) const {
        if (operation == 5 && !parent.id.empty()) return "children:" + parent.id;
        if (operation != 0) {
            return media_type == reddmedia::LibraryMediaType::Television
                ? "roots:television" : "roots:movies";
        }
        return {};
    }

    void start_library_task(int operation,
                            const std::string& folder = {},
                            const reddmedia::LibraryNode& parent = {}) {
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            if (libraryState->busy) return;
        }
        if (operation == 1 || operation == 2 || operation == 3) homeNeedsRefresh.store(true);
        if (libraryWorker.joinable()) libraryWorker.join();

        const reddmedia::LibraryMediaType media_type = libraryMediaType;
        const bool type_chosen = libraryTypeChosen;
        const std::string cache_key = library_cache_key(operation, media_type, parent);
        std::vector<reddmedia::LibraryNode> cached_nodes;
        if (!cache_key.empty()) {
            std::string cache_error;
            libraryMetadataCache->load(cache_key, cached_nodes, cache_error);
        }
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            if (cached_nodes.empty() && (operation == 1 || operation == 2 || operation == 3)) {
                cached_nodes = libraryState->nodes;
            }
            if (!cached_nodes.empty() && (operation == 4 || operation == 5)) {
                libraryState->nodes = cached_nodes;
            }
            libraryState->busy = true;
            libraryState->updated = false;
            libraryState->progress = 0.0;
            libraryState->progress_determinate = false;
            libraryState->progress_label = "Scanning...";
            if (operation == 1) libraryState->status = "Linking folder and scanning Jellyfin library...";
            else if (operation == 2) libraryState->status = "Scanning Jellyfin library for changes...";
            else if (operation == 3) libraryState->status = "Unlinking folder and refreshing Jellyfin library...";
            else if (!cached_nodes.empty()) libraryState->status = "Cached metadata ready. Checking for library changes...";
            else libraryState->status = "Loading real library metadata...";
        }
        if (operation == 2) {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            posterState->failed.clear();
        }
        redraw();

        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        const std::shared_ptr<reddmedia::LibraryMetadataCache> metadata_cache = libraryMetadataCache;
        const std::shared_ptr<LibraryUiState> state = libraryState;
        libraryWorker = std::thread([client, engine, metadata_cache, state, operation, folder, parent,
                                     media_type, type_chosen, cache_key, cached_nodes]() mutable {
            const auto set_indeterminate = [state](const std::string& status, const std::string& label) {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (!status.empty()) state->status = status;
                state->progress = 0.0;
                state->progress_determinate = false;
                state->progress_label = label;
            };
            const auto set_measured = [state](std::size_t completed, std::size_t total,
                                              const std::string& status) {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (!status.empty()) state->status = status;
                state->progress_determinate = total > 0U;
                state->progress = total > 0U
                    ? std::max(0.0, std::min(1.0, static_cast<double>(completed) / static_cast<double>(total)))
                    : 0.0;
                state->progress_label = total > 0U
                    ? std::to_string(completed) + " / " + std::to_string(total)
                    : "Scanning...";
            };

            std::string error;
            bool ok = client->initialize(error);
            if (ok && operation == 1) {
                set_indeterminate("Linking folder and scanning Jellyfin library...", "Scanning...");
                ok = client->add_media_folder(folder, media_type, error);
            }
            if (ok && operation == 2) {
                set_indeterminate("Scanning Jellyfin library for changes...", "Scanning...");
                ok = client->refresh_library(error);
            }
            if (ok && operation == 3) {
                set_indeterminate("Unlinking folder and refreshing Jellyfin library...", "Scanning...");
                ok = client->unlink_media_folder(folder, media_type, error);
            }

            std::vector<reddmedia::MediaFolder> folders;
            if (ok) {
                set_indeterminate("Reading linked media folders...", "Reading...");
                ok = client->load_media_folders(folders, error);
            }

            std::vector<reddmedia::LibraryNode> nodes;
            if (ok && operation == 5) {
                set_indeterminate(cached_nodes.empty() ? "Loading library level..." :
                                  "Cached metadata ready. Checking this library level...", "Reading...");
                ok = client->load_library_children(parent, nodes, error);
            } else if (ok && type_chosen && operation != 0) {
                set_indeterminate(cached_nodes.empty() ? "Loading library titles..." :
                                  "Cached metadata ready. Checking for library changes...", "Reading...");
                ok = client->load_library_roots(media_type, nodes, error);
            }

            std::map<std::string, reddmedia::LibraryNode> cached_by_id;
            for (const auto& cached : cached_nodes) {
                if (!cached.id.empty()) cached_by_id[cached.id] = cached;
            }

            if (ok) {
                const std::size_t total = nodes.size();
                for (std::size_t index = 0; index < nodes.size(); ++index) {
                    reddmedia::LibraryNode& node = nodes[index];
                    const auto old = cached_by_id.find(node.id);
                    if (old != cached_by_id.end()) {
                        const reddmedia::LibraryNode& cached = old->second;
                        const bool same_provider_identity = cached.tmdb_id == node.tmdb_id &&
                            cached.series_tmdb_id == node.series_tmdb_id;
                        if (same_provider_identity) {
                            if (node.episode_title.empty()) node.episode_title = cached.episode_title;
                            if (node.overview.empty()) node.overview = cached.overview;
                            if (node.tmdb_poster_path.empty()) node.tmdb_poster_path = cached.tmdb_poster_path;
                        }
                    }

                    if (engine->external_credential_available()) {
                        std::string fallback_error;
                        if (node.kind == reddmedia::LibraryNodeKind::Episode &&
                            !node.series_tmdb_id.empty() && node.season_number >= 0 &&
                            node.episode_number > 0 &&
                            (node.episode_title.empty() || node.overview.empty())) {
                            std::string title;
                            std::string overview;
                            if (engine->load_tv_episode_details(node.series_tmdb_id,
                                                                node.season_number,
                                                                node.episode_number,
                                                                title, overview,
                                                                fallback_error)) {
                                if (node.episode_title.empty()) node.episode_title = title;
                                if (node.overview.empty()) node.overview = overview;
                            }
                        }
                        if (node.tmdb_poster_path.empty()) {
                            if (node.kind == reddmedia::LibraryNodeKind::Movie && !node.tmdb_id.empty()) {
                                engine->load_movie_poster_path(node.tmdb_id,
                                                              node.tmdb_poster_path,
                                                              fallback_error);
                            } else if (node.kind == reddmedia::LibraryNodeKind::Series && !node.tmdb_id.empty()) {
                                node.series_tmdb_id = node.tmdb_id;
                                engine->load_tv_poster_path(node.tmdb_id, 0,
                                                           node.tmdb_poster_path,
                                                           fallback_error);
                            } else if (!node.series_tmdb_id.empty()) {
                                engine->load_tv_poster_path(node.series_tmdb_id,
                                                           std::max(0, node.season_number),
                                                           node.tmdb_poster_path,
                                                           fallback_error);
                            }
                        }
                    }
                    set_measured(index + 1U, total, "Updating metadata...");
                }
                if (nodes.empty()) set_indeterminate("Library check complete.", "Ready");
            }

            if (ok && !cache_key.empty()) {
                std::string cache_error;
                metadata_cache->store(cache_key, nodes, cache_error);
            }

            std::lock_guard<std::mutex> lock(state->mutex);
            if (ok) {
                state->folders = std::move(folders);
                state->nodes = std::move(nodes);
                if (!type_chosen) {
                    state->status = "Choose Movies or TV.";
                } else if (state->nodes.empty()) {
                    state->status = operation == 5
                        ? "No real items were found inside this title."
                        : "No titles found. Link a folder or press Refresh after adding media.";
                } else {
                    state->status = std::to_string(state->nodes.size()) +
                        (state->nodes.size() == 1U ? " title ready." : " titles ready.");
                }
            } else {
                state->status = error;
            }
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
            state->progress_determinate = true;
            state->progress_label = "100%";
        });
    }

    void poll_library_worker() {
        bool updated = false;
        std::size_t count = 0;
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            updated = libraryState->updated;
            if (updated) {
                libraryState->updated = false;
                count = libraryState->nodes.size();
            }
        }
        if (!updated) return;
        if (libraryWorker.joinable()) libraryWorker.join();
        if (count == 0U) librarySelected = -1;
        else if (librarySelected < 0 || librarySelected >= static_cast<int>(count)) librarySelected = 0;
        libraryScroll = std::max(0, std::min(libraryScroll, std::max(0, static_cast<int>(count) - 1)));
        queue_library_posters();
        if (!fullscreen && currentView == ViewMode::Library) redraw();
    }

    bool has_library_folder(reddmedia::LibraryMediaType media_type) {
        std::lock_guard<std::mutex> lock(libraryState->mutex);
        for (const reddmedia::MediaFolder& folder : libraryState->folders) {
            if (folder.media_type == media_type) return true;
        }
        return false;
    }

    void select_library_type(reddmedia::LibraryMediaType media_type) {
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            if (libraryState->busy) return;
        }
        if (!libraryTypeChosen || libraryMediaType != media_type || !libraryParents.empty()) push_navigation_history();
        libraryMediaType = media_type;
        libraryTypeChosen = true;
        libraryParents.clear();
        librarySelected = -1;
        libraryScroll = 0;
        if (!has_library_folder(media_type)) {
            const std::string folder = choose_media_library_folder_dialog();
            if (!folder.empty()) start_library_task(1, folder);
            else {
                {
                    std::lock_guard<std::mutex> lock(libraryState->mutex);
                    libraryState->nodes.clear();
                    libraryState->status = std::string("No ") +
                        (media_type == reddmedia::LibraryMediaType::Movies ? "Movies" : "TV") +
                        " folder is linked. Choose the button again when you are ready to link one.";
                }
                redraw();
            }
            return;
        }
        start_library_task(4);
    }

    void add_library_folder() {
        if (!libraryTypeChosen) {
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                libraryState->status = "Choose Movies or TV before linking a folder.";
            }
            redraw();
            return;
        }
        const std::string folder = choose_media_library_folder_dialog();
        if (!folder.empty()) start_library_task(1, folder);
    }

    void unlink_library_folder() {
        if (!libraryTypeChosen) {
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                libraryState->status = "Choose Movies or TV before unlinking a folder.";
            }
            redraw();
            return;
        }
        const std::string folder = choose_media_library_folder_dialog();
        if (!folder.empty()) start_library_task(3, folder);
    }

    reddmedia::MediaDescriptor descriptor_for_node(const reddmedia::LibraryNode& node) {
        reddmedia::MediaDescriptor item;
        item.id = node.id;
        item.title = node.name;
        item.overview = node.overview;
        item.genres = node.genres;
        item.local_path = node.path;
        item.tmdb_id = node.tmdb_id;
        item.year = node.production_year;
        item.media_type = (node.kind == reddmedia::LibraryNodeKind::Series ||
                           node.kind == reddmedia::LibraryNodeKind::Season ||
                           node.kind == reddmedia::LibraryNodeKind::Episode)
            ? reddmedia::RecommendationMediaType::Television
            : reddmedia::RecommendationMediaType::Movie;
        return item;
    }

    std::string inferred_series_name_for_path(const std::string& path) const {
        const std::string folder = dirname_only(path);
        std::string name = basename_only(folder);
        const std::string lower = lower_copy(name);
        bool season_folder = lower.rfind("season", 0) == 0;
        if (!season_folder && !lower.empty() && lower[0] == 's') {
            season_folder = lower.size() > 1 && std::isdigit(static_cast<unsigned char>(lower[1]));
        }
        if (season_folder) {
            const std::string parent = dirname_only(folder);
            const std::string parent_name = basename_only(parent);
            if (!parent_name.empty()) name = parent_name;
        }
        return name.empty() ? "Series" : name;
    }

    reddmedia::LibraryNode inferred_episode_node_for_path(const std::string& path) const {
        reddmedia::LibraryNode node;
        int season = 0, episode = 0;
        if (!parse_episode_code(path, season, episode)) return node;
        node.kind = reddmedia::LibraryNodeKind::Episode;
        node.path = path;
        node.name = stem_only(path);
        node.episode_title = node.name;
        node.series_name = inferred_series_name_for_path(path);
        node.season_number = season;
        node.episode_number = episode;
        return node;
    }

    bool build_same_folder_episode_queue(const reddmedia::LibraryNode& selected,
                                         std::vector<reddmedia::LibraryNode>& queue,
                                         int& current_index) const {
        queue.clear();
        current_index = -1;
        if (selected.path.empty() || !exists_file(selected.path)) return false;
        int current_season = 0, current_episode = 0;
        const bool current_has_code = parse_episode_code(selected.path, current_season, current_episode);
        if (selected.kind != reddmedia::LibraryNodeKind::Episode && !current_has_code) return false;

        const std::string folder = dirname_only(selected.path);
        DIR* directory = opendir(folder.c_str());
        if (!directory) return false;
        std::vector<std::string> paths;
        while (dirent* entry = readdir(directory)) {
            const std::string name(entry->d_name);
            if (name.empty() || name == "." || name == "..") continue;
            const std::string candidate = folder + "/" + name;
            if (!exists_file(candidate) || !is_playable_video_path(candidate)) continue;
            int season = 0, episode = 0;
            if (current_has_code && !parse_episode_code(candidate, season, episode)) continue;
            paths.push_back(candidate);
        }
        closedir(directory);
        if (paths.empty()) return false;
        std::sort(paths.begin(), paths.end(), [](const std::string& a, const std::string& b) {
            int as = 0, ae = 0, bs = 0, be = 0;
            const bool ah = parse_episode_code(a, as, ae);
            const bool bh = parse_episode_code(b, bs, be);
            if (ah && bh) {
                if (as != bs) return as < bs;
                if (ae != be) return ae < be;
            } else if (ah != bh) {
                return ah;
            }
            return natural_filename_less(a, b);
        });

        const std::string inferred_series = !selected.series_name.empty()
            ? selected.series_name : inferred_series_name_for_path(selected.path);
        for (const std::string& path : paths) {
            reddmedia::LibraryNode node = inferred_episode_node_for_path(path);
            if (node.path.empty()) {
                node.kind = reddmedia::LibraryNodeKind::Episode;
                node.path = path;
                node.name = stem_only(path);
                node.episode_title = node.name;
                node.series_name = inferred_series;
            }
            if (node.series_name.empty()) node.series_name = inferred_series;
            if (path == selected.path) {
                reddmedia::LibraryNode merged = selected;
                if (merged.path.empty()) merged.path = path;
                if (merged.name.empty()) merged.name = node.name;
                if (merged.episode_title.empty()) merged.episode_title = node.episode_title;
                if (merged.series_name.empty()) merged.series_name = node.series_name;
                if (merged.season_number <= 0) merged.season_number = node.season_number;
                if (merged.episode_number <= 0) merged.episode_number = node.episode_number;
                merged.kind = reddmedia::LibraryNodeKind::Episode;
                node = std::move(merged);
            }
            if (path == selected.path) current_index = static_cast<int>(queue.size());
            queue.push_back(std::move(node));
        }
        return current_index >= 0;
    }

    bool resolve_catalog_series_for_episode_path(const std::string& path,
                                                  reddmedia::LibraryNode& series) {
        if (path.empty()) return false;
        std::string error;
        std::vector<reddmedia::LibraryNode> roots;
        if (!libraryClient->load_library_roots(reddmedia::LibraryMediaType::Television, roots, error)) return false;
        for (const auto& root : roots) {
            if (root.kind != reddmedia::LibraryNodeKind::Series) continue;
            if (!root.path.empty() && path.size() > root.path.size() &&
                path.compare(0, root.path.size(), root.path) == 0 && path[root.path.size()] == '/') {
                series = root;
                return true;
            }
            std::vector<reddmedia::LibraryNode> seasons;
            std::string child_error;
            if (!libraryClient->load_library_children(root, seasons, child_error)) continue;
            for (const auto& season : seasons) {
                if (season.kind != reddmedia::LibraryNodeKind::Season) continue;
                std::vector<reddmedia::LibraryNode> episodes;
                child_error.clear();
                if (!libraryClient->load_library_children(season, episodes, child_error)) continue;
                for (const auto& episode : episodes) {
                    if (episode.kind == reddmedia::LibraryNodeKind::Episode && episode.path == path) {
                        series = root;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool prepare_tv_autoplay(const reddmedia::LibraryNode& selected) {
        tvAutoplayQueue.clear();
        tvAutoplayIndex = -1;
        tvAutoplayRetryIndex = -1;
        tvAutoplayRetryAttempts = 0;
        tvAutoplayRetryAtMs = 0;
        lastLocalPlaybackPositionMs = 0;
        lastLocalPlaybackLengthMs = 0;
        tvAutoplayArmed = false;
        activeLibraryItemValid = false;
        playbackEndHandled = false;
        reddmedia::LibraryNode episode = selected;
        int parsed_season = 0, parsed_episode = 0;
        if (episode.kind != reddmedia::LibraryNodeKind::Episode &&
            parse_episode_code(episode.path, parsed_season, parsed_episode)) {
            const reddmedia::LibraryNode inferred = inferred_episode_node_for_path(episode.path);
            if (episode.name.empty()) episode.name = inferred.name;
            if (episode.episode_title.empty()) episode.episode_title = inferred.episode_title;
            if (episode.series_name.empty()) episode.series_name = inferred.series_name;
            episode.season_number = parsed_season;
            episode.episode_number = parsed_episode;
            episode.kind = reddmedia::LibraryNodeKind::Episode;
        }
        if (episode.kind != reddmedia::LibraryNodeKind::Episode) return false;

        // Player-level reliability: the current file's directory is authoritative
        // enough to resolve the immediate next local episode regardless of whether
        // playback started from Home, Library, Open File, resume, or another route.
        std::vector<reddmedia::LibraryNode> folder_queue;
        int folder_index = -1;
        const bool have_folder_queue = build_same_folder_episode_queue(episode, folder_queue, folder_index);
        if (have_folder_queue && folder_index + 1 < static_cast<int>(folder_queue.size())) {
            tvAutoplayQueue = std::move(folder_queue);
            tvAutoplayIndex = folder_index;
            activeLibraryItem = tvAutoplayQueue[static_cast<std::size_t>(tvAutoplayIndex)];
            activeLibraryItemValid = true;
            tvAutoplayArmed = true;
            return true;
        }

        reddmedia::LibraryNode series;
        bool have_series = false;
        for (const auto& parent : libraryParents) {
            if (parent.kind == reddmedia::LibraryNodeKind::Series) { series = parent; have_series = true; break; }
        }
        // An episode can be opened from a route that no longer has the Series
        // object in the visible navigation stack. Jellyfin still gives the
        // episode its SeriesId, so reconstruct the parent and build the full
        // season-spanning queue instead of silently degrading to one page.
        if (!have_series && !episode.series_id.empty()) {
            series.id = episode.series_id;
            series.name = episode.series_name;
            series.kind = reddmedia::LibraryNodeKind::Series;
            series.tmdb_id = episode.series_tmdb_id;
            series.series_tmdb_id = episode.series_tmdb_id;
            have_series = true;
        }
        std::string error;
        if (have_series && libraryClient->initialize(error)) {
            std::vector<reddmedia::LibraryNode> seasons;
            if (libraryClient->load_library_children(series, seasons, error)) {
                std::sort(seasons.begin(), seasons.end(), [](const auto& a, const auto& b) {
                    if (a.season_number != b.season_number) return a.season_number < b.season_number;
                    return a.name < b.name;
                });
                for (const auto& season : seasons) {
                    if (season.kind != reddmedia::LibraryNodeKind::Season) continue;
                    std::vector<reddmedia::LibraryNode> episodes;
                    std::string child_error;
                    if (!libraryClient->load_library_children(season, episodes, child_error)) continue;
                    std::sort(episodes.begin(), episodes.end(), [](const auto& a, const auto& b) {
                        if (a.season_number != b.season_number) return a.season_number < b.season_number;
                        if (a.episode_number != b.episode_number) return a.episode_number < b.episode_number;
                        return a.name < b.name;
                    });
                    for (const auto& episode : episodes) {
                        if (episode.kind == reddmedia::LibraryNodeKind::Episode && exists_file(episode.path)) {
                            tvAutoplayQueue.push_back(episode);
                        }
                    }
                }
            }
        }
        if (tvAutoplayQueue.empty()) {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            for (const auto& episode : libraryState->nodes) {
                if (episode.kind == reddmedia::LibraryNodeKind::Episode && exists_file(episode.path)) {
                    tvAutoplayQueue.push_back(episode);
                }
            }
        }
        for (std::size_t i=0; i<tvAutoplayQueue.size(); ++i) {
            if ((!episode.id.empty() && tvAutoplayQueue[i].id == episode.id) ||
                (!episode.path.empty() && tvAutoplayQueue[i].path == episode.path)) {
                tvAutoplayIndex = static_cast<int>(i);
                break;
            }
        }
        if (tvAutoplayIndex < 0 && have_folder_queue) {
            tvAutoplayQueue = std::move(folder_queue);
            tvAutoplayIndex = folder_index;
        }
        if (tvAutoplayIndex < 0) return false;
        activeLibraryItem = episode;
        activeLibraryItemValid = true;
        tvAutoplayArmed = true;
        return true;
    }

    int up_next_seconds_remaining() const {
        if (!upNextVisible || !upNextHasEpisode || upNextDeadlineMs <= 0) return -1;
        const long long remaining = std::max<long long>(0, upNextDeadlineMs - now_ms());
        return static_cast<int>((remaining + 999) / 1000);
    }

    void clear_up_next_overlay() {
        upNextVisible = false;
        upNextHasEpisode = false;
        upNextTargetIndex = -1;
        upNextDeadlineMs = 0;
        upNextLastDisplayedSeconds = -1;
        upNextMessage.clear();
    }

    void show_up_next_overlay(bool draw_now=true) {
        clear_up_next_overlay();
        if (!tvAutoplayArmed || tvAutoplayIndex < 0) {
            upNextVisible = true;
            upNextMessage = "No next episode was resolved for this playback.";
            if (draw_now) draw_video_message();
            return;
        }
        const int next = tvAutoplayIndex + 1;
        if (next >= static_cast<int>(tvAutoplayQueue.size())) {
            upNextVisible = true;
            upNextMessage = "No next episode found. You reached the end of the available series queue.";
            if (draw_now) draw_video_message();
            return;
        }
        const reddmedia::LibraryNode candidate = tvAutoplayQueue[static_cast<std::size_t>(next)];
        if (!exists_file(candidate.path)) {
            upNextVisible = true;
            upNextMessage = "The next episode was identified, but its media file is unavailable.";
            if (draw_now) draw_video_message();
            return;
        }
        upNextVisible = true;
        upNextHasEpisode = true;
        upNextTargetIndex = next;
        upNextEpisode = candidate;
        upNextDeadlineMs = now_ms() + 10000;
        upNextLastDisplayedSeconds = 10;
        upNextMessage = "Playing automatically in 10 seconds.";
        if (draw_now) draw_video_message();
    }

    void play_up_next_now() {
        if (!upNextVisible || !upNextHasEpisode || upNextTargetIndex < 0) return;
        const int target = upNextTargetIndex;
        clear_up_next_overlay();
        if (!start_tv_autoplay_index(target)) {
            upNextVisible = true;
            upNextMessage = "Nougat could not start the next episode yet. Automatic retry is active.";
            draw_video_message();
        }
    }

    void replay_active_episode() {
        if (!activeLibraryItemValid || activeLibraryItem.path.empty() || !exists_file(activeLibraryItem.path)) return;
        clear_up_next_overlay();
        playbackEndHandled = false;
        const reddmedia::LibraryNode replay = activeLibraryItem;
        if (open_media(replay.path, 0)) {
            activeLibraryItem = replay;
            activeLibraryItemValid = true;
            std::string history_error;
            recommendationEngine->record_started(descriptor_for_node(replay), history_error);
        }
    }

    void back_to_series_from_up_next() {
        reddmedia::LibraryNode episode = activeLibraryItemValid ? activeLibraryItem : upNextEpisode;
        clear_up_next_overlay();
        tvAutoplayArmed = false;
        tvAutoplayRetryIndex = -1;
        tvAutoplayRetryAttempts = 0;
        tvAutoplayRetryAtMs = 0;
        reddmedia::LibraryNode series;
        if (!episode.series_id.empty()) {
            series.id = episode.series_id;
            series.name = episode.series_name.empty() ? "Series" : episode.series_name;
            series.kind = reddmedia::LibraryNodeKind::Series;
            series.tmdb_id = episode.series_tmdb_id;
            series.series_tmdb_id = episode.series_tmdb_id;
        } else if (!resolve_catalog_series_for_episode_path(episode.path, series)) {
            libraryMediaType = reddmedia::LibraryMediaType::Television;
            libraryTypeChosen = true;
            libraryParents.clear();
            librarySelected = -1;
            libraryScroll = 0;
            switch_view(ViewMode::Library);
            start_library_task(4);
            return;
        }
        libraryMediaType = reddmedia::LibraryMediaType::Television;
        libraryTypeChosen = true;
        libraryParents.clear();
        libraryParents.push_back(series);
        librarySelected = -1;
        libraryScroll = 0;
        switch_view(ViewMode::Library);
        start_library_task(5, {}, series);
    }

    void poll_up_next_overlay() {
        if (!upNextVisible || !upNextHasEpisode || upNextDeadlineMs <= 0) return;
        const int seconds = up_next_seconds_remaining();
        if (seconds != upNextLastDisplayedSeconds) {
            upNextLastDisplayedSeconds = seconds;
            upNextMessage = "Playing automatically in " + std::to_string(seconds) +
                (seconds == 1 ? " second." : " seconds.");
            if (currentView == ViewMode::VideoPlayer) draw_video_message();
        }
        if (now_ms() >= upNextDeadlineMs) play_up_next_now();
    }

    void mark_active_episode_completed() {
        if (!activeLibraryItemValid) return;
        std::string error;
        recommendationEngine->record_completed(descriptor_for_node(activeLibraryItem), error);
    }

    bool start_tv_autoplay_index(int index) {
        if (!tvAutoplayArmed || index < 0 || index >= static_cast<int>(tvAutoplayQueue.size())) return false;
        const reddmedia::LibraryNode candidate = tvAutoplayQueue[static_cast<std::size_t>(index)];
        clear_up_next_overlay();
        if (!exists_file(candidate.path) || !open_media(candidate.path, 0)) {
            tvAutoplayRetryIndex = index;
            ++tvAutoplayRetryAttempts;
            tvAutoplayRetryAtMs = now_ms() + 750;
            return false;
        }
        tvAutoplayIndex = index;
        tvAutoplayRetryIndex = -1;
        tvAutoplayRetryAttempts = 0;
        tvAutoplayRetryAtMs = 0;
        activeLibraryItem = candidate;
        activeLibraryItemValid = true;
        std::string history_error;
        recommendationEngine->record_started(descriptor_for_node(activeLibraryItem), history_error);
        return true;
    }

    bool play_next_tv_episode() {
        if (!tvAutoplayArmed || tvAutoplayIndex < 0) return false;
        const int next = tvAutoplayIndex + 1;
        if (next >= static_cast<int>(tvAutoplayQueue.size())) {
            tvAutoplayArmed = false;
            tvAutoplayRetryIndex = -1;
            return false;
        }
        return start_tv_autoplay_index(next);
    }

    void poll_tv_autoplay_retry() {
        if (!tvAutoplayArmed || tvAutoplayRetryIndex < 0 || now_ms() < tvAutoplayRetryAtMs) return;
        const int retry_index = tvAutoplayRetryIndex;
        if (tvAutoplayRetryAttempts >= 3) {
            tvAutoplayArmed = false;
            tvAutoplayRetryIndex = -1;
            upNextVisible = true;
            upNextHasEpisode = false;
            upNextMessage = "Nougat could not start the resolved next episode after three attempts.";
            if (currentView == ViewMode::VideoPlayer) draw_video_message();
            return;
        }
        start_tv_autoplay_index(retry_index);
    }

    void poll_natural_playback_end() {
        if (!mp || paused || playbackEndHandled || currentMediaIsNetwork || currentMediaIsP2P || currentMediaIsYtDlpStream) return;
        const int state = api.get_state ? api.get_state(mp) : -1;
        const long long length = playback_length_ms();
        const long long position = playback_time_ms();
        if (length > 0) lastLocalPlaybackLengthMs = length;
        if (position >= 0) lastLocalPlaybackPositionMs = position;

        bool ended = state == 6; // libVLC Ended
        if (!ended && length > 0 && position >= std::max<long long>(0, length - 500)) ended = true;
        // Some local files briefly report Stopped at natural EOF. Manual Stop
        // cancels tvAutoplayArmed before calling libVLC stop, so this fallback
        // cannot accidentally advance after an owner-requested Stop.
        if (!ended && state == 5 && tvAutoplayArmed && lastLocalPlaybackLengthMs > 0 &&
            lastLocalPlaybackPositionMs >= std::max<long long>(0, lastLocalPlaybackLengthMs - 1500)) {
            ended = true;
        }
        if (!ended) return;
        playbackEndHandled = true;
        mark_active_episode_completed();
        mark_current_resume_completed();
        show_up_next_overlay();
        if (!fullscreen) redraw();
    }

    void open_library_node(const reddmedia::LibraryNode& selected) {
        if (selected.kind == reddmedia::LibraryNodeKind::MovieCollection ||
            selected.kind == reddmedia::LibraryNodeKind::Series ||
            selected.kind == reddmedia::LibraryNodeKind::Season) {
            push_navigation_history();
            libraryParents.push_back(selected);
            librarySelected = -1;
            libraryScroll = 0;
            start_library_task(5, {}, selected);
            return;
        }
        if (!exists_file(selected.path)) {
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                libraryState->status = "That media file is unavailable. Press Refresh.";
            }
            redraw();
            return;
        }
        if (selected.kind == reddmedia::LibraryNodeKind::Episode) prepare_tv_autoplay(selected);
        else cancel_tv_autoplay();
        activeLibraryItem = selected;
        activeLibraryItemValid = true;
        std::string history_error;
        recommendationEngine->record_started(descriptor_for_node(selected), history_error);
        request_local_playback(selected.path, &selected, true);
    }

    void open_selected_library_item() {
        reddmedia::LibraryNode selected;
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            if (librarySelected < 0 || librarySelected >= static_cast<int>(libraryState->nodes.size())) return;
            selected = libraryState->nodes[static_cast<std::size_t>(librarySelected)];
        }
        open_library_node(selected);
    }

    void library_back() {
        if (libraryParents.empty()) return;
        push_navigation_history();
        libraryParents.pop_back();
        librarySelected = -1;
        libraryScroll = 0;
        if (libraryParents.empty()) start_library_task(4);
        else start_library_task(5, {}, libraryParents.back());
    }

    std::string library_poster_key(const reddmedia::LibraryNode& node) const {
        // v0.0.28 prefers an exact-ID TMDb poster when the catalog already knows
        // the provider ID.  Jellyfin Primary remains the local fallback.
        if (!node.tmdb_poster_path.empty()) return "tmdb:" + node.tmdb_poster_path;
        if (!node.poster_item_id.empty()) {
            return "jellyfin:" + node.poster_item_id + ":" + node.poster_image_tag;
        }
        return "";
    }

    std::string home_artwork_key(const reddmedia::LibraryNode& node) const {
        // v0.0.28 Home rests on poster art, never a backdrop/still by preference.
        // Exact TMDb IDs are used when available; Jellyfin Primary is the local fallback.
        if (!node.tmdb_poster_path.empty()) return "tmdb:" + node.tmdb_poster_path;
        const std::string item_id = !node.poster_item_id.empty() ? node.poster_item_id : node.id;
        const std::string tag = !node.poster_image_tag.empty() ? node.poster_image_tag : node.primary_image_tag;
        if (!item_id.empty() && !tag.empty()) return "primary:" + item_id + ":" + tag;
        return {};
    }

    static bool poster_quality_ok(const reddmedia::LibraryPoster& poster) {
        if (poster.width < 240 || poster.height < 320 || poster.rgb.empty()) return false;
        const double aspect = static_cast<double>(poster.width) / static_cast<double>(poster.height);
        return aspect >= 0.52 && aspect <= 0.82;
    }
    static bool home_artwork_quality_ok(const reddmedia::LibraryPoster& image) {
        if (image.width < 240 || image.height < 135 || image.rgb.empty()) return false;
        const double aspect = static_cast<double>(image.width) / static_cast<double>(image.height);
        // Accept ordinary portrait posters and 16:9-ish episode stills/backdrops.
        return (aspect >= 0.50 && aspect <= 0.90) || (aspect >= 1.20 && aspect <= 2.20);
    }

    static int home_grid_columns_for_width(int width) {
        const int available = std::max(1, width - 56);
        // At the owner's half-screen width (~650px), three cards must fit.
        if (width >= 600) return std::max(3, available / 184);
        if (width >= 430) return 2;
        return 1;
    }

    static std::string joined_genres(const std::vector<std::string>& genres, std::size_t limit=2U) {
        std::string result;
        for (std::size_t i=0; i<genres.size() && i<limit; ++i) {
            if (!result.empty()) result += "  •  ";
            result += genres[i];
        }
        return result;
    }

    std::string home_card_subtitle(const reddmedia::LibraryNode& node) const {
        std::string result;
        if (node.production_year > 0) result = std::to_string(node.production_year);
        const std::string genres = joined_genres(node.genres);
        if (!genres.empty()) {
            if (!result.empty()) result += "  •  ";
            result += genres;
        }
        if (result.empty() && node.kind == reddmedia::LibraryNodeKind::Episode) {
            char season_episode[32];
            std::snprintf(season_episode, sizeof(season_episode), "S%02dE%02d", node.season_number, node.episode_number);
            result = season_episode;
        }
        return result;
    }

    std::string home_continue_subtitle(const ResumeRecord& record) const {
        std::string result;
        if (!record.series_name.empty()) {
            char season_episode[32];
            std::snprintf(season_episode, sizeof(season_episode), "S%02dE%02d", record.season_number, record.episode_number);
            result = season_episode;
            if (!record.episode_title.empty()) result += "  •  " + record.episode_title;
        } else if (record.production_year > 0) {
            result = std::to_string(record.production_year);
        }
        if (record.duration_ms > 0) {
            if (!result.empty()) result += "  •  ";
            result += format_time(record.position_ms) + " / " + format_time(record.duration_ms);
        }
        return result;
    }

    static bool home_node_is_playable_file(const reddmedia::LibraryNode& node) {
        return !node.path.empty() && exists_file(node.path);
    }

    static std::string normalized_genre(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    void start_home_task() {
        bool already_loaded = false;
        {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            if (homeState->busy) return;
            already_loaded = homeState->loaded;
        }
        // Returning to Home is instant.  A reload only starts when Home has never
        // loaded, watch/library data changed, or the owner explicitly refreshes it.
        if (already_loaded && !homeNeedsRefresh.exchange(false)) return;
        if (!already_loaded) homeNeedsRefresh.store(false);
        if (homeWorker.joinable()) homeWorker.join();
        const std::vector<ResumeRecord> continue_records = resumeStore.unfinished();
        {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            homeState->busy = true;
            homeState->updated = false;
            homeState->progress = 0.02;
            homeState->continue_watching = continue_records;
            if (!homeState->loaded) {
                homeState->status = "Loading your local Home feed...";
                homeState->sections.clear();
                homeState->artwork.clear();
                homeState->continue_artwork_nodes.clear();
                homeState->artwork_failed.clear();
            }
        }
        const std::shared_ptr<HomeUiState> state = homeState;
        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        homeWorker = std::thread([state, client, engine, continue_records]() {
            const auto fail_refresh = [state](const std::string& message) {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->busy = false;
                state->updated = true;
                state->progress = 1.0;
                if (!state->loaded) state->status = message;
            };

            std::string error;
            if (!client->initialize(error)) {
                fail_refresh("LOCAL is waiting for the Nougat media server: " + error);
                return;
            }
            std::vector<reddmedia::LibraryNode> nodes;
            if (!client->load_all_recommendation_items(nodes, error)) {
                fail_refresh("Could not load LOCAL media: " + error);
                return;
            }
            nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const auto& node) {
                return node.kind != reddmedia::LibraryNodeKind::Movie &&
                       node.kind != reddmedia::LibraryNodeKind::Series;
            }), nodes.end());

            // Exact TMDb provider IDs from Jellyfin may provide a cleaner/high-res
            // poster.  This is not title guessing: the existing catalog ID is used.
            if (engine->external_credential_available()) {
                for (auto& node : nodes) {
                    std::string poster_error;
                    if (node.kind == reddmedia::LibraryNodeKind::Movie && !node.tmdb_id.empty()) {
                        engine->load_movie_poster_path(node.tmdb_id, node.tmdb_poster_path, poster_error);
                    } else if (node.kind == reddmedia::LibraryNodeKind::Series && !node.tmdb_id.empty()) {
                        node.series_tmdb_id = node.tmdb_id;
                        engine->load_tv_poster_path(node.tmdb_id, 0, node.tmdb_poster_path, poster_error);
                    }
                }
            }

            std::map<std::string, reddmedia::LibraryNode> series_by_id;
            std::map<std::string, reddmedia::LibraryNode> item_by_id;
            for (const auto& node : nodes) {
                item_by_id[node.id] = node;
                if (node.kind == reddmedia::LibraryNodeKind::Series) series_by_id[node.id] = node;
            }

            // Resolve one real episode path for series cards so a series can provide
            // a silent hover preview without changing the card's poster identity.
            for (auto& node : nodes) {
                if (node.kind != reddmedia::LibraryNodeKind::Series || home_node_is_playable_file(node)) continue;
                std::vector<reddmedia::LibraryNode> seasons;
                std::string child_error;
                if (!client->load_library_children(node, seasons, child_error)) continue;
                std::sort(seasons.begin(), seasons.end(), [](const auto& a, const auto& b) {
                    if (a.season_number != b.season_number) return a.season_number < b.season_number;
                    return a.name < b.name;
                });
                bool found = false;
                for (const auto& season : seasons) {
                    if (season.kind != reddmedia::LibraryNodeKind::Season) continue;
                    std::vector<reddmedia::LibraryNode> episodes;
                    if (!client->load_library_children(season, episodes, child_error)) continue;
                    std::sort(episodes.begin(), episodes.end(), [](const auto& a, const auto& b) {
                        if (a.season_number != b.season_number) return a.season_number < b.season_number;
                        if (a.episode_number != b.episode_number) return a.episode_number < b.episode_number;
                        return a.name < b.name;
                    });
                    for (const auto& episode : episodes) {
                        if (episode.kind == reddmedia::LibraryNodeKind::Episode && home_node_is_playable_file(episode)) {
                            node.path = episode.path;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }

            // v0.0.29 Continue Watching artwork hierarchy:
            // episode Primary still -> matching season poster -> series poster -> Nougat fallback.
            // This prevents black TV cards while still giving an episode its own image when Jellyfin has one.
            std::map<std::string, reddmedia::LibraryNode> continue_artwork_nodes;
            const auto has_local_art = [](const reddmedia::LibraryNode& node) {
                return !node.tmdb_poster_path.empty() ||
                       !node.poster_image_tag.empty() || !node.primary_image_tag.empty();
            };
            const auto art_from_record = [](const ResumeRecord& record) {
                reddmedia::LibraryNode art;
                art.id = record.item_id;
                art.path = record.path;
                art.name = record.title;
                art.series_name = record.series_name;
                art.episode_title = record.episode_title;
                art.tmdb_id = record.tmdb_id;
                art.series_id = record.series_id;
                art.series_tmdb_id = record.series_tmdb_id;
                art.primary_image_tag = record.primary_image_tag;
                art.poster_item_id = record.item_id;
                art.poster_image_tag = record.primary_image_tag;
                art.season_number = record.season_number;
                art.episode_number = record.episode_number;
                art.production_year = record.production_year;
                art.kind = static_cast<reddmedia::LibraryNodeKind>(record.kind);
                return art;
            };
            for (const auto& record : continue_records) {
                reddmedia::LibraryNode art;
                bool have_art = false;
                if (record.series_name.empty()) {
                    const auto exact_item = item_by_id.find(record.item_id);
                    if (exact_item != item_by_id.end() && has_local_art(exact_item->second)) {
                        art = exact_item->second;
                        have_art = true;
                    }
                }
                if (!record.series_name.empty() && !record.item_id.empty() && !record.primary_image_tag.empty()) {
                    art = art_from_record(record);
                    art.kind = reddmedia::LibraryNodeKind::Episode;
                    art.poster_item_id = record.item_id;
                    art.poster_image_tag = record.primary_image_tag;
                    art.primary_image_tag = record.primary_image_tag;
                    have_art = true;
                }
                // Resolve the owning series even for resume records created from
                // Open File / Home paths that do not carry Jellyfin IDs. Exact path
                // containment is preferred; a case-insensitive series-name match is
                // only a fallback. This keeps TV cards from becoming black simply
                // because the episode was not launched from the Library hierarchy.
                reddmedia::LibraryNode resolved_series;
                bool have_series = false;
                if (!record.series_id.empty()) {
                    const auto series_it = series_by_id.find(record.series_id);
                    if (series_it != series_by_id.end()) {
                        resolved_series = series_it->second;
                        have_series = true;
                    }
                }
                if (!have_series && !record.path.empty()) {
                    for (const auto& pair : series_by_id) {
                        const auto& candidate = pair.second;
                        if (!candidate.path.empty() && record.path.size() > candidate.path.size() &&
                            record.path.compare(0, candidate.path.size(), candidate.path) == 0 &&
                            record.path[candidate.path.size()] == '/') {
                            resolved_series = candidate;
                            have_series = true;
                            break;
                        }
                    }
                }
                if (!have_series && !record.series_name.empty()) {
                    const std::string wanted = lower_copy(record.series_name);
                    for (const auto& pair : series_by_id) {
                        if (lower_copy(pair.second.name) == wanted) {
                            resolved_series = pair.second;
                            have_series = true;
                            break;
                        }
                    }
                }
                if (have_series) {
                    const reddmedia::LibraryNode series = resolved_series;
                    std::vector<reddmedia::LibraryNode> seasons;
                    std::string season_error;
                    if (client->load_library_children(series, seasons, season_error)) {
                        for (auto season : seasons) {
                            if (season.kind != reddmedia::LibraryNodeKind::Season ||
                                (record.season_number > 0 && season.season_number != record.season_number)) continue;

                            // Exact episode Primary/still art is first choice when
                            // Jellyfin can map the resumed path back to the episode.
                            std::vector<reddmedia::LibraryNode> episodes;
                            std::string episode_error;
                            if (!have_art && client->load_library_children(season, episodes, episode_error)) {
                                for (const auto& episode : episodes) {
                                    if (episode.kind != reddmedia::LibraryNodeKind::Episode) continue;
                                    const bool same_path = !record.path.empty() && episode.path == record.path;
                                    const bool same_id = !record.item_id.empty() && episode.id == record.item_id;
                                    if ((same_path || same_id) && has_local_art(episode)) {
                                        art = episode;
                                        have_art = true;
                                        break;
                                    }
                                }
                            }
                            if (engine->external_credential_available() && !series.tmdb_id.empty()) {
                                std::string tmdb_error;
                                engine->load_tv_poster_path(series.tmdb_id, season.season_number,
                                                            season.tmdb_poster_path, tmdb_error);
                            }
                            // At minimum a TV episode gets its matching season poster
                            // when an episode still is unavailable; series art is last.
                            if (!have_art && has_local_art(season)) { art = std::move(season); have_art = true; }
                            if (record.season_number > 0) break;
                        }
                    }
                    if (!have_art && has_local_art(series)) { art = series; have_art = true; }
                }
                if (!have_art) {
                    art = art_from_record(record);
                    art.poster_item_id = record.item_id;
                    art.poster_image_tag = record.primary_image_tag;
                    art.primary_image_tag = record.primary_image_tag;
                    art.series_tmdb_id = record.series_tmdb_id;
                    if (engine->external_credential_available()) {
                        std::string tmdb_error;
                        if (!record.series_tmdb_id.empty() && record.season_number > 0) {
                            engine->load_tv_poster_path(record.series_tmdb_id, record.season_number,
                                                        art.tmdb_poster_path, tmdb_error);
                        } else if (!record.tmdb_id.empty()) {
                            engine->load_movie_poster_path(record.tmdb_id, art.tmdb_poster_path, tmdb_error);
                        }
                    }
                }
                continue_artwork_nodes[record.path] = std::move(art);
            }

            std::map<std::string, double> genre_weights;
            auto learn_history = [&genre_weights](const std::vector<reddmedia::ViewingRecord>& history) {
                const long long now = static_cast<long long>(std::time(nullptr));
                for (const auto& record : history) {
                    double weight = 1.0 + std::min(5, record.play_count) * 0.65 + (record.completed ? 1.4 : 0.35);
                    const long long age = std::max<long long>(0, now - record.last_watched);
                    if (age < 7LL * 86400LL) weight += 1.25;
                    else if (age < 30LL * 86400LL) weight += 0.6;
                    for (const auto& genre : record.item.genres) genre_weights[normalized_genre(genre)] += weight;
                }
            };
            std::vector<reddmedia::ViewingRecord> movie_history;
            std::vector<reddmedia::ViewingRecord> tv_history;
            std::string history_error;
            if (engine->recent_history(reddmedia::RecommendationMediaType::Movie, movie_history, history_error, 200)) {
                learn_history(movie_history);
            }
            history_error.clear();
            if (engine->recent_history(reddmedia::RecommendationMediaType::Television, tv_history, history_error, 200)) {
                learn_history(tv_history);
            }

            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_real_distribution<double> noise(0.0, 1.0);
            std::vector<std::pair<double, reddmedia::LibraryNode>> ranked;
            ranked.reserve(nodes.size());
            for (const auto& node : nodes) {
                double score = noise(rng) * 2.0;
                for (const auto& genre : node.genres) {
                    const auto found = genre_weights.find(normalized_genre(genre));
                    if (found != genre_weights.end()) score += found->second;
                }
                ranked.emplace_back(score, node);
            }
            std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
                return a.first > b.first;
            });

            std::vector<HomeSection> sections;
            HomeSection recommended;
            recommended.title = genre_weights.empty() ? "Explore" : "Recommended For You";
            for (std::size_t i=0; i<ranked.size() && i<18U; ++i) recommended.items.push_back(ranked[i].second);
            if (!recommended.items.empty()) sections.push_back(std::move(recommended));

            const std::vector<std::string> genre_order = {
                "Action", "Adventure", "Comedy", "Drama", "Crime", "Thriller", "Horror",
                "Science Fiction", "Fantasy", "Mystery", "Documentary", "Western", "Animation",
                "Family", "History", "War", "Music", "Romance"
            };
            std::set<std::string> used_genres;
            auto append_genre = [&](const std::string& display_genre) {
                const std::string wanted = normalized_genre(display_genre);
                if (!used_genres.insert(wanted).second) return;
                HomeSection section;
                section.title = display_genre;
                for (const auto& pair : ranked) {
                    const auto& node = pair.second;
                    bool match = false;
                    for (const auto& genre : node.genres) {
                        if (normalized_genre(genre) == wanted) { match = true; break; }
                    }
                    if (match) section.items.push_back(node);
                    if (section.items.size() >= 15U) break;
                }
                if (section.items.size() >= 2U) sections.push_back(std::move(section));
            };
            for (const auto& genre : genre_order) append_genre(genre);
            for (const auto& node : nodes) {
                for (const auto& genre : node.genres) append_genre(genre);
            }

            // Load portrait poster artwork at a display-useful resolution.  Reject
            // tiny or landscape sources instead of stretching them into mush.
            std::map<std::string, reddmedia::LibraryPoster> artwork;
            std::set<std::string> failed;
            std::vector<reddmedia::LibraryNode> artwork_nodes;
            artwork_nodes.reserve(continue_artwork_nodes.size() + nodes.size());
            for (const auto& entry : continue_artwork_nodes) artwork_nodes.push_back(entry.second);
            for (const auto& node : nodes) artwork_nodes.push_back(node);
            std::set<std::string> seen;
            std::size_t done = 0;
            for (const auto& node : artwork_nodes) {
                const std::string item_id = !node.poster_item_id.empty() ? node.poster_item_id : node.id;
                const std::string tag = !node.poster_image_tag.empty() ? node.poster_image_tag : node.primary_image_tag;
                std::string key;
                if (!node.tmdb_poster_path.empty()) key = "tmdb:" + node.tmdb_poster_path;
                else if (!item_id.empty() && !tag.empty()) key = "primary:" + item_id + ":" + tag;
                if (key.empty() || !seen.insert(key).second) continue;

                std::string bytes;
                std::string image_error;
                reddmedia::LibraryPoster poster;
                bool good = false;
                if (!node.tmdb_poster_path.empty()) {
                    if (engine->load_external_poster_bmp(node.tmdb_poster_path, 480, 720, bytes, image_error) &&
                        reddmedia::decode_library_poster_bmp(bytes, poster, image_error) && home_artwork_quality_ok(poster)) {
                        good = true;
                    }
                }
                if (!good && !item_id.empty() && !tag.empty()) {
                    bytes.clear(); image_error.clear(); poster = reddmedia::LibraryPoster{};
                    if (client->load_primary_image_bmp(item_id, tag, 480, 720, bytes, image_error) &&
                        reddmedia::decode_library_poster_bmp(bytes, poster, image_error) && home_artwork_quality_ok(poster)) {
                        good = true;
                    }
                }
                if (good) artwork[key] = std::move(poster);
                else failed.insert(key);

                ++done;
                if ((done % 3U) == 0U) {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->progress = std::min(0.95, 0.25 + static_cast<double>(done) /
                        static_cast<double>(std::max<std::size_t>(1U, artwork_nodes.size())) * 0.7);
                }
            }

            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->continue_watching = continue_records;
                state->continue_artwork_nodes = std::move(continue_artwork_nodes);
                state->sections = std::move(sections);
                state->artwork = std::move(artwork);
                state->artwork_failed = std::move(failed);
                state->status = nodes.empty() ? "LOCAL has no movies or TV yet." : "LOCAL ready.";
                state->progress = 1.0;
                state->busy = false;
                state->updated = true;
                state->loaded = true;
            }
        });
    }

    void poll_home_worker() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            updated = homeState->updated;
            if (updated) homeState->updated = false;
        }
        if (!updated) return;
        if (homeWorker.joinable()) homeWorker.join();
        if (!fullscreen && currentView == ViewMode::Home) redraw();
    }

    void draw_cover_pixels(Drawable target, const Rect& area, const reddmedia::LibraryPoster& poster) {
        if (area.w <= 0 || area.h <= 0 || poster.width <= 0 || poster.height <= 0 || poster.rgb.empty()) return;
        const double source_aspect = static_cast<double>(poster.width) / static_cast<double>(poster.height);
        const double target_aspect = static_cast<double>(area.w) / static_cast<double>(area.h);
        int crop_x = 0;
        int crop_y = 0;
        int crop_w = poster.width;
        int crop_h = poster.height;
        if (source_aspect > target_aspect) {
            crop_w = std::max(1, static_cast<int>(poster.height * target_aspect));
            crop_x = std::max(0, (poster.width - crop_w) / 2);
        } else if (source_aspect < target_aspect) {
            crop_h = std::max(1, static_cast<int>(poster.width / target_aspect));
            crop_y = std::max(0, (poster.height - crop_h) / 2);
        }
        const int depth = DefaultDepth(d, screen);
        const int bytes_per_pixel = 4;
        char* image_data = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(area.w) * static_cast<std::size_t>(area.h),
            static_cast<std::size_t>(bytes_per_pixel)));
        if (!image_data) return;
        for (int y=0; y<area.h; ++y) {
            const int source_y = crop_y + std::min(crop_h - 1, y * crop_h / std::max(1, area.h));
            for (int x=0; x<area.w; ++x) {
                const int source_x = crop_x + std::min(crop_w - 1, x * crop_w / std::max(1, area.w));
                const std::size_t source = (static_cast<std::size_t>(source_y) * static_cast<std::size_t>(poster.width) +
                                            static_cast<std::size_t>(source_x)) * 3U;
                if (source + 2U >= poster.rgb.size()) continue;
                const unsigned long pixel =
                    component_to_visual_mask(poster.rgb[source], DefaultVisual(d, screen)->red_mask) |
                    component_to_visual_mask(poster.rgb[source + 1U], DefaultVisual(d, screen)->green_mask) |
                    component_to_visual_mask(poster.rgb[source + 2U], DefaultVisual(d, screen)->blue_mask);
                std::memcpy(image_data +
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(area.w) + static_cast<std::size_t>(x)) * static_cast<std::size_t>(bytes_per_pixel),
                    &pixel, static_cast<std::size_t>(bytes_per_pixel));
            }
        }
        XImage* image = XCreateImage(d, DefaultVisual(d, screen), depth, ZPixmap, 0,
                                     image_data, area.w, area.h, 32, 0);
        if (!image) { std::free(image_data); return; }
        XPutImage(d, target, gc, image, 0, 0, area.x, area.y, area.w, area.h);
        XDestroyImage(image);
    }

    void fill_top_round(Drawable target, const Rect& raw, int radius, unsigned long c) {
        if (raw.w <= 0 || raw.h <= 0) return;
        Rect r = raw;
        radius = std::max(0, std::min(radius, std::min(r.w / 2, r.h)));
        if (radius <= 0) { fill(target, r, c); return; }
        XSetForeground(d, gc, c);
        XFillRectangle(d, target, gc, r.x, r.y + radius,
                       static_cast<unsigned>(r.w), static_cast<unsigned>(std::max(0, r.h - radius)));
        XFillRectangle(d, target, gc, r.x + radius, r.y,
                       static_cast<unsigned>(std::max(0, r.w - radius * 2)), static_cast<unsigned>(radius));
        const int diameter = radius * 2;
        XFillArc(d, target, gc, r.x, r.y, diameter, diameter, 90 * 64, 90 * 64);
        XFillArc(d, target, gc, r.x + r.w - diameter, r.y, diameter, diameter, 0, 90 * 64);
    }

    int rounded_top_inset_for_row(int row, int radius) const {
        if (radius <= 0 || row >= radius) return 0;
        const double rr = static_cast<double>(radius);
        const double dy = rr - (static_cast<double>(row) + 0.5);
        const double inside = std::max(0.0, rr * rr - dy * dy);
        return std::max(0, static_cast<int>(std::ceil(rr - std::sqrt(inside))));
    }

    void copy_pixmap_top_rounded(Pixmap source, Drawable target, const Rect& area, int radius) {
        for (int y = 0; y < area.h; ++y) {
            const int inset = std::min(area.w / 2, rounded_top_inset_for_row(y, radius));
            const int width = area.w - inset * 2;
            if (width <= 0) continue;
            XCopyArea(d, source, target, gc, inset, y, static_cast<unsigned>(width), 1,
                      area.x + inset, area.y + y);
        }
    }

    void draw_cover_pixels_top_rounded(Drawable target, const Rect& area,
                                       const reddmedia::LibraryPoster& poster, int radius) {
        if (area.w <= 0 || area.h <= 0) return;
        Pixmap tmp = XCreatePixmap(d, win, static_cast<unsigned>(area.w),
                                   static_cast<unsigned>(area.h), DefaultDepth(d, screen));
        if (!tmp) return;
        GC windowGc = gc;
        GC imageGc = XCreateGC(d, tmp, 0, nullptr);
        if (!imageGc) { XFreePixmap(d, tmp); return; }
        gc = imageGc;
        draw_cover_pixels(tmp, {0, 0, area.w, area.h}, poster);
        gc = windowGc;
        XFreeGC(d, imageGc);
        copy_pixmap_top_rounded(tmp, target, area, radius);
        XFreePixmap(d, tmp);
    }

    void draw_contain_pixels_top_rounded(Drawable target, const Rect& area,
                                         const reddmedia::LibraryPoster& poster, int radius,
                                         unsigned long background) {
        if (area.w <= 0 || area.h <= 0 || poster.width <= 0 || poster.height <= 0) return;
        Pixmap tmp = XCreatePixmap(d, win, static_cast<unsigned>(area.w),
                                   static_cast<unsigned>(area.h), DefaultDepth(d, screen));
        if (!tmp) return;
        GC windowGc = gc;
        GC imageGc = XCreateGC(d, tmp, 0, nullptr);
        if (!imageGc) { XFreePixmap(d, tmp); return; }
        gc = imageGc;
        fill(tmp, {0, 0, area.w, area.h}, background);
        const double source_aspect = static_cast<double>(poster.width) / static_cast<double>(poster.height);
        const double target_aspect = static_cast<double>(area.w) / static_cast<double>(area.h);
        Rect dest{0,0,area.w,area.h};
        if (source_aspect < target_aspect) {
            dest.w = std::max(1, static_cast<int>(area.h * source_aspect));
            dest.x = (area.w - dest.w) / 2;
        } else if (source_aspect > target_aspect) {
            dest.h = std::max(1, static_cast<int>(area.w / source_aspect));
            dest.y = (area.h - dest.h) / 2;
        }
        draw_poster_pixels(tmp, dest, poster);
        gc = windowGc;
        XFreeGC(d, imageGc);
        copy_pixmap_top_rounded(tmp, target, area, radius);
        XFreePixmap(d, tmp);
    }

    void draw_home_artwork(Drawable target, const Rect& area, const reddmedia::LibraryNode& node) {
        const int radius = 10;
        const unsigned long backdrop = rgb8(26, 20, 29);
        fill_top_round(target, area, radius, backdrop);
        reddmedia::LibraryPoster poster;
        bool available = false;
        const std::string key = home_artwork_key(node);
        if (!key.empty()) {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            const auto found = homeState->artwork.find(key);
            if (found != homeState->artwork.end()) { poster = found->second; available = true; }
        }
        if (available) {
            // v0.0.29 keeps poster/still identity but fills the artwork region.
            // Preserve aspect ratio and crop only the excess instead of shrinking
            // a portrait movie poster into a postage stamp inside a wide card.
            draw_cover_pixels_top_rounded(target, area, poster, radius);
        } else {
            const std::string label = node.name.empty() ? "NO POSTER" : head_to_width(node.name, area.w - 20);
            metadata_text(target, area.x + 10, area.y + area.h / 2, label, rgb8(226, 214, 226));
        }
    }

    int home_card_artwork_height(bool continue_card, int card_width) const {
        // v0.0.37 owner correction: Continue Watching is a shelf behavior, not
        // a smaller card family. Every Home card uses the same physical poster
        // geometry; Continue Watching keeps only its progress/resume semantics.
        (void)continue_card;
        return std::max(168, card_width * 3 / 2);
    }

    int home_card_height(bool continue_card, int card_width) const {
        return home_card_artwork_height(continue_card, card_width) + 50;
    }

    void draw_home_card(Drawable target, const Rect& card, const reddmedia::LibraryNode& node,
                        const std::string& subtitle, bool continue_card, long long resume_ms=0, long long duration_ms=0) {
        const ViewPalette palette = palette_for(ViewMode::Home);
        const unsigned long cardText = rgb8(55, 36, 49);
        const unsigned long cardMuted = rgb8(101, 76, 87);
        const Rect image{card.x, card.y, card.w, std::max(70, card.h - 50)};
        fill_round(target, {card.x, card.y + 2, card.w, card.h}, 10, palette.buttonDark);
        fill_round(target, card, 10, rgb8(246, 238, 225));
        draw_home_artwork(target, image, node);

        // Replace poster with the live silent hover frame.  The preview is clipped
        // through the same rounded top mask so square pixels never protrude.
        if (!node.path.empty() && node.path == homeHoveredPath) {
            reddmedia::LibraryPoster frame;
            bool has_frame = false;
            {
                std::lock_guard<std::mutex> lock(homePreviewState->mutex);
                has_frame = homePreviewState->has_frame && homePreviewState->path == node.path;
                if (has_frame) frame = homePreviewState->frame;
            }
            if (has_frame) draw_cover_pixels_top_rounded(target, image, frame, 10);
        }
        if (continue_card && duration_ms > 0) {
            const double fraction = std::max(0.0, std::min(1.0, static_cast<double>(resume_ms) / static_cast<double>(duration_ms)));
            fill(target, {image.x, image.y + image.h - 5, image.w, 5}, rgb8(109, 89, 75));
            fill(target, {image.x, image.y + image.h - 5, std::max(1, static_cast<int>(image.w * fraction)), 5}, rgb8(194, 122, 48));
        }
        metadata_text(target, card.x + 5, image.y + image.h + 18,
                      head_to_width(node.name, card.w - 10), cardText);
        if (!subtitle.empty()) {
            metadata_text(target, card.x + 5, image.y + image.h + 38,
                          head_to_width(subtitle, card.w - 10), cardMuted);
        }
        outline_round(target, card, 10, palette.buttonDark);
    }

    int home_max_page_scroll() const {
        const Rect frame=page_content_frame(ViewMode::Home);
        const int viewportBottom=frame.y+frame.h-8;
        return std::max(0, homeContentHeight - viewportBottom);
    }

    void draw_home_scrollbar_component(Drawable target, const Rect& track, const Rect& thumb,
                                       const ViewPalette& palette) {
        if (track.w <= 0 || track.h <= 0 || thumb.w <= 0 || thumb.h <= 0) return;
        const int trackRadius = std::max(4, std::min(track.w, track.h) / 2);
        fill_round(target, {track.x, track.y + 2, track.w, track.h}, trackRadius, palette.buttonDark);
        fill_round(target, track, trackRadius, palette.field);
        outline_round(target, track, trackRadius, palette.border);
        Rect inset{track.x + 2, track.y + 2, std::max(1, track.w - 4), std::max(1, track.h - 4)};
        outline_round(target, inset, std::max(2, trackRadius - 2), palette.buttonLight);

        const int thumbRadius = std::max(4, std::min(thumb.w, thumb.h) / 2);
        fill_round(target, {thumb.x, thumb.y + 2, thumb.w, thumb.h}, thumbRadius, palette.buttonDark);
        fill_round(target, thumb, thumbRadius, palette.button);
        outline_round(target, thumb, thumbRadius, palette.buttonDark);
        Rect thumbInset{thumb.x + 2, thumb.y + 2, std::max(1, thumb.w - 4), std::max(1, thumb.h - 4)};
        outline_round(target, thumbInset, std::max(2, thumbRadius - 2), palette.buttonLight);
    }

    void update_home_vertical_scroll_from_pointer(int pointerY, bool centerThumb) {
        const int maxScroll = home_max_page_scroll();
        const int span = std::max(0, homeVerticalScrollTrack.h - homeVerticalScrollThumb.h);
        if (maxScroll <= 0 || span <= 0) { homePageScroll = 0; return; }
        int thumbTop = pointerY - (centerThumb ? homeVerticalScrollThumb.h / 2 : homeVerticalScrollDragOffset);
        thumbTop = std::max(homeVerticalScrollTrack.y, std::min(homeVerticalScrollTrack.y + span, thumbTop));
        homePageScroll = static_cast<int>((static_cast<long long>(thumbTop - homeVerticalScrollTrack.y) * maxScroll + span / 2) / span);
    }

    void update_home_continue_scroll_from_pointer(int pointerX, bool centerThumb) {
        const int span = std::max(0, homeContinueScrollTrack.w - homeContinueScrollThumb.w);
        if (homeContinueMaxScrollX <= 0 || span <= 0) { homeContinueScrollX = 0; return; }
        int thumbLeft = pointerX - (centerThumb ? homeContinueScrollThumb.w / 2 : homeContinueScrollDragOffset);
        thumbLeft = std::max(homeContinueScrollTrack.x, std::min(homeContinueScrollTrack.x + span, thumbLeft));
        homeContinueScrollX = static_cast<int>((static_cast<long long>(thumbLeft - homeContinueScrollTrack.x) * homeContinueMaxScrollX + span / 2) / span);
    }

    bool handle_home_scrollbar_press(int x, int y) {
        if (homeVerticalScrollThumb.contains(x, y)) {
            homeVerticalScrollDragging = true;
            homeVerticalScrollDragOffset = y - homeVerticalScrollThumb.y;
            return true;
        }
        if (homeVerticalScrollTrack.contains(x, y)) {
            update_home_vertical_scroll_from_pointer(y, true);
            redraw();
            return true;
        }
        if (homeContinueScrollThumb.contains(x, y)) {
            homeContinueScrollDragging = true;
            homeContinueScrollDragOffset = x - homeContinueScrollThumb.x;
            return true;
        }
        if (homeContinueScrollTrack.contains(x, y)) {
            update_home_continue_scroll_from_pointer(x, true);
            redraw();
            return true;
        }
        return false;
    }

    bool handle_home_scrollbar_motion(int x, int y) {
        bool changed = false;
        if (homeVerticalScrollDragging) {
            const int before = homePageScroll;
            update_home_vertical_scroll_from_pointer(y, false);
            changed = changed || before != homePageScroll;
        }
        if (homeContinueScrollDragging) {
            const int before = homeContinueScrollX;
            update_home_continue_scroll_from_pointer(x, false);
            changed = changed || before != homeContinueScrollX;
        }
        if (changed) redraw();
        return homeVerticalScrollDragging || homeContinueScrollDragging;
    }

    void draw_home_screen(Drawable target) {
        homeCardHitboxes.clear();
        const ViewPalette palette = palette_for(ViewMode::Home);
        const Rect frame = page_content_frame(ViewMode::Home);
        draw_quilted_background(target, frame, ViewMode::Home);
        const int viewport_top = frame.y + 8;
        const int viewport_bottom = frame.y + frame.h - 8;
        const int viewport_left = frame.x + 10;
        const int viewport_right = frame.x + frame.w - 22; // reserve vertical scrollbar
        // The outer Home frame is the authoritative rendering viewport.
        XRectangle homeClip{static_cast<short>(viewport_left), static_cast<short>(viewport_top),
                            static_cast<unsigned short>(std::max(1, viewport_right - viewport_left)),
                            static_cast<unsigned short>(std::max(1, viewport_bottom - viewport_top))};
        XSetClipRectangles(d, gc, 0, 0, &homeClip, 1, Unsorted);
        const int content_y0 = viewport_top - homePageScroll;
        int y = content_y0;
        const int gap = 12;
        const int grid_width = std::max(1, viewport_right - viewport_left - 16);
        const int columns = home_grid_columns_for_width(grid_width + 56);
        const int card_w = std::max(145, std::min(240,
            (grid_width - (columns - 1) * gap) / std::max(1, columns)));
        const int local_card_h = home_card_height(false, card_w);

        std::vector<ResumeRecord> continues;
        std::vector<HomeSection> sections;
        std::map<std::string, reddmedia::LibraryNode> continue_artwork_nodes;
        std::string status;
        bool busy = false;
        {
            std::lock_guard<std::mutex> lock(homeState->mutex);
            continues = homeState->continue_watching;
            sections = homeState->sections;
            continue_artwork_nodes = homeState->continue_artwork_nodes;
            status = homeState->status;
            busy = homeState->busy;
        }

        if (!continues.empty()) {
            section_text(target, viewport_left + 8, y + 20, "CONTINUE WATCHING", palette.text);
            y += 34;
            // Every Continue Watching item uses one fixed card geometry. Source
            // media type/artwork can never make one card taller or shorter than its peers.
            const int continue_max_h = home_card_height(true, card_w);
            homeContinueArea = {viewport_left + 8, y, std::max(1, viewport_right - viewport_left - 20), continue_max_h};
            // Continue Watching has its own hard horizontal clip. A card that
            // crosses either shelf edge disappears immediately and can never
            // render underneath a scrollbar or against the raw window edge.
            XRectangle continueClip{static_cast<short>(homeContinueArea.x), static_cast<short>(std::max(viewport_top, homeContinueArea.y)),
                                    static_cast<unsigned short>(std::max(1, homeContinueArea.w)),
                                    static_cast<unsigned short>(std::max(1, std::min(viewport_bottom, homeContinueArea.y + homeContinueArea.h) - std::max(viewport_top, homeContinueArea.y)))};
            XSetClipRectangles(d, gc, 0, 0, &continueClip, 1, Unsorted);
            const int total_w = static_cast<int>(continues.size()) * (card_w + gap) - gap;
            const int max_scroll = std::max(0, total_w - homeContinueArea.w);
            homeContinueMaxScrollX = max_scroll;
            homeContinueScrollX = std::max(0, std::min(homeContinueScrollX, max_scroll));
            int x = homeContinueArea.x - homeContinueScrollX;
            for (const auto& record : continues) {
                reddmedia::LibraryNode node = node_from_resume_record(record);
                Rect card{x, y, card_w, continue_max_h};
                if (card.x + card.w > homeContinueArea.x && card.x < homeContinueArea.x + homeContinueArea.w &&
                    card.y + card.h > viewport_top && card.y < viewport_bottom) {
                    if (!record.series_name.empty()) node.name = record.series_name;
                    else if (node.name.empty()) node.name = record.title;

                    // Playback identity remains the episode/movie file, while the
                    // resting artwork comes from the resolved season/movie poster.
                    const auto art_it = continue_artwork_nodes.find(record.path);
                    if (art_it != continue_artwork_nodes.end()) {
                        const auto& art = art_it->second;
                        node.poster_item_id = art.poster_item_id;
                        node.poster_image_tag = art.poster_image_tag;
                        node.primary_image_tag = art.primary_image_tag;
                        node.tmdb_poster_path = art.tmdb_poster_path;
                    }
                    draw_home_card(target, card, node, home_continue_subtitle(record), true,
                                   record.position_ms, record.duration_ms);
                    Rect visibleCard{std::max(card.x,homeContinueArea.x),std::max(card.y,viewport_top),
                                     std::max(0,std::min(card.x+card.w,homeContinueArea.x+homeContinueArea.w)-std::max(card.x,homeContinueArea.x)),
                                     std::max(0,std::min(card.y+card.h,viewport_bottom)-std::max(card.y,viewport_top))};
                    if (visibleCard.w>0 && visibleCard.h>0) homeCardHitboxes.push_back({visibleCard, node, true, record.position_ms});
                }
                x += card_w + gap;
            }
            XSetClipRectangles(d, gc, 0, 0, &homeClip, 1, Unsorted);
            // Dedicated sheet-style horizontal scrollbar for Continue Watching.
            // It lives directly below the shelf; mouse-wheel shelf scrolling is retained.
            const int scrollY = y + continue_max_h + 8;
            homeContinueScrollTrack = {homeContinueArea.x, scrollY, homeContinueArea.w, 14};
            if (homeContinueMaxScrollX > 0) {
                const int totalSpan = homeContinueArea.w + homeContinueMaxScrollX;
                const int thumbW = std::max(42, std::min(homeContinueScrollTrack.w,
                    homeContinueScrollTrack.w * homeContinueArea.w / std::max(1, totalSpan)));
                const int thumbTravel = std::max(0, homeContinueScrollTrack.w - thumbW);
                const int thumbX = homeContinueScrollTrack.x +
                    (homeContinueMaxScrollX > 0 ? thumbTravel * homeContinueScrollX / homeContinueMaxScrollX : 0);
                homeContinueScrollThumb = {thumbX, homeContinueScrollTrack.y, thumbW, homeContinueScrollTrack.h};
            } else {
                homeContinueScrollThumb = homeContinueScrollTrack;
            }
            y += continue_max_h + 54;
        } else {
            homeContinueArea = {0,0,0,0};
            homeContinueMaxScrollX = 0;
            homeContinueScrollTrack = {0,0,0,0};
            homeContinueScrollThumb = {0,0,0,0};
        }

        section_text(target, viewport_left + 8, y + 20, "LOCAL", palette.text);
        y += 38;
        if (sections.empty()) {
            metadata_text(target, viewport_left + 8, y + 22,
                          busy ? "Loading your movies and TV..." : status, palette.text);
            y += 48;
        }
        for (const auto& section : sections) {
            section_text(target, viewport_left + 8, y + 20, section.title, palette.text);
            y += 34;
            int column = 0;
            for (const auto& node : section.items) {
                const int x = viewport_left + 8 + column * (card_w + gap);
                Rect card{x, y, card_w, local_card_h};
                if (card.y + card.h > viewport_top && card.y < viewport_bottom) {
                    draw_home_card(target, card, node, home_card_subtitle(node), false);
                    homeCardHitboxes.push_back({card, node, false, 0});
                }
                ++column;
                if (column >= columns) { column = 0; y += local_card_h + gap; }
            }
            if (column != 0) y += local_card_h + gap;
            y += 20;
        }
        homeContentHeight = std::max(H, y + homePageScroll + 20);
        const int max_page_scroll = home_max_page_scroll();
        homePageScroll = std::max(0, std::min(homePageScroll, max_page_scroll));

        // Scrollbars remain controls inside the Home frame and above card content.
        apply_page_clip(ViewMode::Home);
        const int verticalTrackH = std::max(40, viewport_bottom - viewport_top - 8);
        homeVerticalScrollTrack = {frame.x + frame.w - 17, viewport_top + 4, 11, verticalTrackH};
        if (max_page_scroll > 0) {
            const int visibleH = std::max(1, viewport_bottom - viewport_top);
            const int totalH = visibleH + max_page_scroll;
            const int thumbH = std::max(38, std::min(homeVerticalScrollTrack.h,
                homeVerticalScrollTrack.h * visibleH / std::max(1, totalH)));
            const int thumbTravel = std::max(0, homeVerticalScrollTrack.h - thumbH);
            const int thumbY = homeVerticalScrollTrack.y + thumbTravel * homePageScroll / max_page_scroll;
            homeVerticalScrollThumb = {homeVerticalScrollTrack.x, thumbY, homeVerticalScrollTrack.w, thumbH};
        } else {
            homeVerticalScrollThumb = homeVerticalScrollTrack;
        }
        draw_home_scrollbar_component(target, homeVerticalScrollTrack, homeVerticalScrollThumb, palette);

        if (homeContinueScrollTrack.w > 0 && homeContinueScrollTrack.y >= viewport_top &&
            homeContinueScrollTrack.y + homeContinueScrollTrack.h <= viewport_bottom) {
            draw_home_scrollbar_component(target, homeContinueScrollTrack, homeContinueScrollThumb, palette);
        }
    }

    void set_home_hover(const std::string& path, long long resume_ms) {
        if (path == homeHoveredPath) return;
        homeHoveredPath = path;
        homeHoverStartedMs = now_ms();
        homePreviewCursorMs = resume_ms > 0 ? resume_ms : 60000;
        homePreviewNextFrameMs = 0;
        {
            std::lock_guard<std::mutex> lock(homePreviewState->mutex);
            ++homePreviewState->generation;
            homePreviewState->path = path;
            homePreviewState->has_frame = false;
            homePreviewState->updated = false;
        }
        if (!fullscreen && currentView == ViewMode::Home) redraw();
    }

    void update_home_hover_from_pointer(int x, int y) {
        if (currentView != ViewMode::Home) return;
        for (const auto& hitbox : homeCardHitboxes) {
            if (hitbox.card.contains(x, y)) {
                if (home_node_is_playable_file(hitbox.node)) set_home_hover(hitbox.node.path, hitbox.resume_ms);
                else set_home_hover({}, 0);
                return;
            }
        }
        set_home_hover({}, 0);
    }

    void poll_home_preview() {
        if (currentView != ViewMode::Home || homeHoveredPath.empty()) return;
        const long long now = now_ms();
        if (now - homeHoverStartedMs < 600 || now < homePreviewNextFrameMs) return;
        std::string path = homeHoveredPath;
        int generation = 0;
        {
            std::lock_guard<std::mutex> lock(homePreviewState->mutex);
            if (homePreviewState->busy) return;
            homePreviewState->busy = true;
            generation = homePreviewState->generation;
        }
        const long long target = std::max<long long>(0, homePreviewCursorMs);
        homePreviewCursorMs += 1000;
        homePreviewNextFrameMs = now + 750;
        const std::shared_ptr<FramePreviewState> state = homePreviewState;
        std::thread([state, path, target, generation]() {
            std::string bytes;
            reddmedia::LibraryPoster frame;
            std::string error;
            const bool ok = extract_video_frame_bmp(path, target, 384, 216, bytes) &&
                            reddmedia::decode_library_poster_bmp(bytes, frame, error);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (generation == state->generation && state->path == path && ok) {
                state->frame = std::move(frame);
                state->has_frame = true;
                state->updated = true;
            }
            state->busy = false;
        }).detach();
    }

    void poll_home_preview_update() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(homePreviewState->mutex);
            updated = homePreviewState->updated;
            if (updated) homePreviewState->updated = false;
        }
        if (updated && !fullscreen && currentView == ViewMode::Home) redraw();
    }

    void open_home_card(const HomeCardHitbox& hitbox) {
        if (hitbox.continue_watching) {
            reddmedia::LibraryNode node = hitbox.node;
            activeLibraryItem = node;
            activeLibraryItemValid = true;
            if (node.kind == reddmedia::LibraryNodeKind::Episode) prepare_tv_autoplay(node);
            std::string history_error;
            recommendationEngine->record_started(descriptor_for_node(node), history_error);
            switch_view(ViewMode::VideoPlayer);
            open_media(node.path, hitbox.resume_ms);
            return;
        }
        if (hitbox.node.kind == reddmedia::LibraryNodeKind::Series) {
            libraryMediaType = reddmedia::LibraryMediaType::Television;
            libraryTypeChosen = true;
            libraryParents.clear();
            switch_view(ViewMode::Library);
            open_library_node(hitbox.node);
            return;
        }
        if (hitbox.node.kind == reddmedia::LibraryNodeKind::Movie && home_node_is_playable_file(hitbox.node)) {
            std::string history_error;
            recommendationEngine->record_started(descriptor_for_node(hitbox.node), history_error);
            request_local_playback(hitbox.node.path, &hitbox.node, true);
        }
    }

    void handle_home_click(int x, int y) {
        for (const auto& hitbox : homeCardHitboxes) {
            if (hitbox.card.contains(x, y)) { open_home_card(hitbox); return; }
        }
    }

    void queue_library_posters() {
        {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            if (posterState->busy) {
                posterQueued = true;
                return;
            }
        }
        if (posterWorker.joinable()) posterWorker.join();
        std::vector<reddmedia::LibraryNode> nodes;
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            nodes = libraryState->nodes;
        }
        {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            posterState->busy = true;
            posterState->updated = false;
            posterState->progress = 0.0;
            posterState->progress_determinate = false;
            posterState->progress_label = "Preparing artwork...";
        }
        const std::shared_ptr<PosterUiState> posters = posterState;
        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        posterWorker = std::thread([posters, client, engine, nodes]() {
            std::size_t total = 0;
            for (const auto& node : nodes) {
                if (!node.tmdb_poster_path.empty() || !node.poster_item_id.empty()) ++total;
            }
            {
                std::lock_guard<std::mutex> lock(posters->mutex);
                posters->progress_determinate = total > 0U;
                posters->progress_label = total > 0U ? "0 / " + std::to_string(total) : "Artwork ready";
            }
            std::size_t completed = 0;
            for (const auto& node : nodes) {
                std::string key;
                if (!node.tmdb_poster_path.empty()) key = "tmdb:" + node.tmdb_poster_path;
                else if (!node.poster_item_id.empty()) {
                    key = "jellyfin:" + node.poster_item_id + ":" + node.poster_image_tag;
                }
                if (key.empty()) continue;
                bool needed = true;
                {
                    std::lock_guard<std::mutex> lock(posters->mutex);
                    needed = posters->cache.count(key) == 0U && posters->failed.count(key) == 0U;
                }
                if (needed) {
                    std::string bytes;
                    std::string error;
                    reddmedia::LibraryPoster poster;
                    bool loaded = false;

                    // Exact-ID TMDb poster first.  If it cannot be loaded, fall
                    // back to Jellyfin Primary.  Both are requested at a useful
                    // portrait size; tiny/landscape results are rejected.
                    if (!node.tmdb_poster_path.empty()) {
                        if (engine->load_external_poster_bmp(node.tmdb_poster_path,
                                                             480, 720, bytes, error) &&
                            reddmedia::decode_library_poster_bmp(bytes, poster, error) &&
                            poster_quality_ok(poster)) {
                            loaded = true;
                        }
                    }
                    if (!loaded && !node.poster_item_id.empty()) {
                        bytes.clear(); error.clear(); poster = reddmedia::LibraryPoster{};
                        if (client->load_primary_image_bmp(node.poster_item_id,
                                                           node.poster_image_tag,
                                                           480, 720, bytes, error) &&
                            reddmedia::decode_library_poster_bmp(bytes, poster, error) &&
                            poster_quality_ok(poster)) {
                            loaded = true;
                        }
                    }
                    std::lock_guard<std::mutex> lock(posters->mutex);
                    if (loaded) posters->cache[key] = std::move(poster);
                    else posters->failed.insert(key);
                }
                ++completed;
                std::lock_guard<std::mutex> lock(posters->mutex);
                posters->progress = total == 0U ? 1.0 :
                    static_cast<double>(completed) / static_cast<double>(total);
                posters->progress_determinate = total > 0U;
                posters->progress_label = total > 0U
                    ? std::to_string(completed) + " / " + std::to_string(total)
                    : "Artwork ready";
            }
            std::lock_guard<std::mutex> lock(posters->mutex);
            posters->busy = false;
            posters->updated = true;
            posters->progress = 1.0;
            posters->progress_determinate = true;
            posters->progress_label = "100%";
        });
    }

    void poll_poster_worker() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            updated = posterState->updated;
            if (updated) posterState->updated = false;
        }
        if (!updated) return;
        if (posterWorker.joinable()) posterWorker.join();
        const bool queued = posterQueued;
        posterQueued = false;
        if (queued) queue_library_posters();
        if (!fullscreen && currentView == ViewMode::Library) redraw();
    }

    void draw_poster_pixels(Drawable target,
                            const Rect& area,
                            const reddmedia::LibraryPoster& poster) {
        const int depth = DefaultDepth(d, screen);
        const int bytes_per_pixel = 4;
        char* image_data = static_cast<char*>(std::calloc(
            static_cast<std::size_t>(area.w) * static_cast<std::size_t>(area.h),
            static_cast<std::size_t>(bytes_per_pixel)));
        if (!image_data) return;
        for (int y = 0; y < area.h; ++y) {
            const int source_y = y * poster.height / area.h;
            for (int x = 0; x < area.w; ++x) {
                const int source_x = x * poster.width / area.w;
                const std::size_t source =
                    (static_cast<std::size_t>(source_y) * static_cast<std::size_t>(poster.width) +
                     static_cast<std::size_t>(source_x)) * 3U;
                const unsigned long pixel =
                    component_to_visual_mask(poster.rgb[source], DefaultVisual(d, screen)->red_mask) |
                    component_to_visual_mask(poster.rgb[source + 1U], DefaultVisual(d, screen)->green_mask) |
                    component_to_visual_mask(poster.rgb[source + 2U], DefaultVisual(d, screen)->blue_mask);
                std::memcpy(image_data +
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(area.w) +
                     static_cast<std::size_t>(x)) * static_cast<std::size_t>(bytes_per_pixel),
                    &pixel, static_cast<std::size_t>(bytes_per_pixel));
            }
        }
        XImage* image = XCreateImage(d, DefaultVisual(d, screen), depth, ZPixmap, 0,
                                     image_data, area.w, area.h, 32, 0);
        if (!image) {
            std::free(image_data);
            return;
        }
        XPutImage(d, target, gc, image, 0, 0, area.x, area.y, area.w, area.h);
        XDestroyImage(image);
    }

    void draw_contain_poster_pixels(Drawable target,
                                    const Rect& area,
                                    const reddmedia::LibraryPoster& poster,
                                    unsigned long background) {
        if (area.w <= 0 || area.h <= 0 || poster.width <= 0 || poster.height <= 0 || poster.rgb.empty()) return;
        fill(target, area, background);
        const double source_aspect = static_cast<double>(poster.width) / static_cast<double>(poster.height);
        const double target_aspect = static_cast<double>(area.w) / static_cast<double>(area.h);
        Rect dest = area;
        if (source_aspect < target_aspect) {
            dest.w = std::max(1, static_cast<int>(area.h * source_aspect));
            dest.x = area.x + (area.w - dest.w) / 2;
        } else if (source_aspect > target_aspect) {
            dest.h = std::max(1, static_cast<int>(area.w / source_aspect));
            dest.y = area.y + (area.h - dest.h) / 2;
        }
        draw_poster_pixels(target, dest, poster);
    }

    void draw_library_poster(Drawable target,
                             const Rect& area,
                             const reddmedia::LibraryNode& node) {
        const unsigned long background = rgb8(20, 24, 18);
        fill(target, area, background);
        const std::string key = library_poster_key(node);
        if (key.empty()) {
            metadata_text(target, area.x + 12, area.y + area.h / 2, "NO POSTER",
                          rgb8(188, 202, 178));
            return;
        }
        reddmedia::LibraryPoster poster;
        bool available = false;
        bool loading = false;
        {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            const auto found = posterState->cache.find(key);
            if (found != posterState->cache.end()) {
                poster = found->second;
                available = true;
            }
            loading = posterState->busy;
        }
        if (available) draw_contain_poster_pixels(target, area, poster, background);
        else metadata_text(target, area.x + 12, area.y + area.h / 2,
                           loading ? "LOADING..." : "NO POSTER",
                           rgb8(188, 202, 178));
    }

    std::string library_view_modes_file() const {
        return config_dir() + "/library_view_modes.cfg";
    }

    LibraryDisplayMode current_library_display_mode() const {
        return libraryMediaType == reddmedia::LibraryMediaType::Television ? libraryTvView : libraryMovieView;
    }

    void save_library_view_modes() {
        ensure_config_dir();
        std::ofstream out(library_view_modes_file(), std::ios::trunc);
        if (!out) return;
        out << "movies=" << (libraryMovieView == LibraryDisplayMode::List ? "list" : "grid") << "\n";
        out << "tv=" << (libraryTvView == LibraryDisplayMode::List ? "list" : "grid") << "\n";
    }

    void load_library_view_modes() {
        std::ifstream in(library_view_modes_file());
        std::string line;
        while (std::getline(in,line)) {
            if (line == "movies=list") libraryMovieView = LibraryDisplayMode::List;
            else if (line == "movies=grid") libraryMovieView = LibraryDisplayMode::Grid;
            else if (line == "tv=list") libraryTvView = LibraryDisplayMode::List;
            else if (line == "tv=grid") libraryTvView = LibraryDisplayMode::Grid;
        }
    }

    void set_library_display_mode(LibraryDisplayMode mode) {
        if (libraryMediaType == reddmedia::LibraryMediaType::Television) libraryTvView = mode;
        else libraryMovieView = mode;
        libraryScroll = 0;
        librarySelected = -1;
        save_library_view_modes();
        redraw();
    }

    LibraryGridMetrics library_grid_metrics() const {
        LibraryGridMetrics metrics;
        const int inner_width = std::max(1, libraryListBox.w - 12);
        const int inner_height = std::max(1, libraryListBox.h - 12);
        metrics.gap = 8;
        if (current_library_display_mode() == LibraryDisplayMode::List) {
            metrics.columns = 1;
            metrics.rows = std::max(1, inner_height / 34);
            metrics.tileWidth = inner_width;
            metrics.tileHeight = 32;
            metrics.posterHeight = 0;
            metrics.visibleItems = metrics.rows;
            return metrics;
        }

        // Keep DVD/poster geometry at 2:3, but size cards from both width and
        // available height.  v0.0.29 could stretch each card wide enough that
        // only one row fit even in a tall/fullscreen Library viewport.
        int target_width = 150;
        if (inner_height >= 430) {
            const int two_row_tile_height = std::max(218, (inner_height - metrics.gap) / 2);
            const int two_row_poster_height = std::max(168, two_row_tile_height - 50);
            target_width = std::max(112, std::min(150, two_row_poster_height * 2 / 3));
        }
        metrics.tileWidth = target_width;
        metrics.columns = std::max(1, (inner_width + metrics.gap) /
                                      (metrics.tileWidth + metrics.gap));
        metrics.posterHeight = std::max(168, metrics.tileWidth * 3 / 2);
        metrics.tileHeight = metrics.posterHeight + 50;
        metrics.rows = std::max(1, (inner_height + metrics.gap) /
                                   (metrics.tileHeight + metrics.gap));
        metrics.visibleItems = metrics.columns * metrics.rows;
        return metrics;
    }

    std::string library_display_title(const reddmedia::LibraryNode& item) const {
        if (item.kind != reddmedia::LibraryNodeKind::Episode) {
            std::string title = item.name;
            if (item.production_year > 0) {
                title += " (" + std::to_string(item.production_year) + ")";
            }
            return title;
        }
        std::ostringstream title;
        if (item.season_number > 0 && item.episode_number > 0) {
            title << 'S' << std::setfill('0') << std::setw(2) << item.season_number
                  << 'E' << std::setw(2) << item.episode_number;
        } else if (item.episode_number > 0) {
            title << "Episode " << item.episode_number;
        } else title << "Episode number unavailable";
        title << " - " << (item.episode_title.empty() ? "Title unavailable" : item.episode_title);
        return title.str();
    }

    void draw_library_view_button(Drawable target, const Rect& r, LibraryDisplayMode mode, bool active) {
        const ViewPalette palette = palette_for(ViewMode::Library);
        const bool hover = r.contains(pointerWindowX, pointerWindowY);
        const SheetControlState state = active ? SheetControlState::Pressed :
            (hover ? SheetControlState::Hover : SheetControlState::Normal);
        draw_sheet_button_surface(target, r, palette, state);
        const unsigned long iconInk = sheet_button_ink(palette, state);
        if (mode == LibraryDisplayMode::List) {
            const int x1 = r.x + 8;
            const int x2 = r.x + r.w - 8;
            for (int row = 0; row < 3; ++row) {
                const int y = r.y + 8 + row * 5;
                line(target, x1, y, x2, y, iconInk);
            }
        } else {
            const int size = 5;
            const int gap = 3;
            const int total = size * 2 + gap;
            const int sx = r.x + (r.w - total) / 2;
            const int sy = r.y + (r.h - total) / 2;
            for (int row = 0; row < 2; ++row) {
                for (int column = 0; column < 2; ++column) {
                    Rect square{sx + column * (size + gap), sy + row * (size + gap), size, size};
                    fill_round(target, square, 1, iconInk);
                }
            }
        }
    }

    bool library_node_matches_search(const reddmedia::LibraryNode& node) const {
        if (librarySearchQuery.empty()) return true;
        const std::string needle = lower_copy(librarySearchQuery);
        const auto contains = [&needle](const std::string& value) {
            return lower_copy(value).find(needle) != std::string::npos;
        };
        if (contains(node.name) || contains(node.series_name) || contains(node.episode_title)) return true;
        if (node.production_year > 0 && std::to_string(node.production_year).find(needle) != std::string::npos) return true;
        return false;
    }

    std::vector<int> library_visible_indices() const {
        std::vector<int> indices;
        std::lock_guard<std::mutex> lock(libraryState->mutex);
        indices.reserve(libraryState->nodes.size());
        for (std::size_t i=0; i<libraryState->nodes.size(); ++i) {
            if (library_node_matches_search(libraryState->nodes[i])) indices.push_back(static_cast<int>(i));
        }
        return indices;
    }

    int library_max_scroll() const {
        const std::size_t count = library_visible_indices().size();
        const LibraryGridMetrics grid = library_grid_metrics();
        return std::max(0, static_cast<int>(count) - grid.visibleItems);
    }

    void update_library_vertical_scroll_from_pointer(int pointerY, bool centerThumb) {
        const int maxScroll = library_max_scroll();
        if (maxScroll <= 0) { libraryScroll = 0; return; }
        const int span = std::max(1, libraryVerticalScrollTrack.h - libraryVerticalScrollThumb.h);
        int thumbTop = pointerY - (centerThumb ? libraryVerticalScrollThumb.h / 2 : libraryVerticalScrollDragOffset);
        thumbTop = std::max(libraryVerticalScrollTrack.y, std::min(libraryVerticalScrollTrack.y + span, thumbTop));
        libraryScroll = static_cast<int>((static_cast<long long>(thumbTop - libraryVerticalScrollTrack.y) * maxScroll + span / 2) / span);
    }

    bool handle_library_scrollbar_press(int x, int y) {
        if (!libraryVerticalScrollTrack.contains(x,y)) return false;
        if (libraryVerticalScrollThumb.contains(x,y)) {
            libraryVerticalScrollDragging = true;
            libraryVerticalScrollDragOffset = y - libraryVerticalScrollThumb.y;
        } else {
            update_library_vertical_scroll_from_pointer(y, true);
            redraw();
        }
        return true;
    }

    bool handle_library_scrollbar_motion(int y) {
        if (!libraryVerticalScrollDragging) return false;
        const int before = libraryScroll;
        update_library_vertical_scroll_from_pointer(y, false);
        if (before != libraryScroll) redraw();
        return true;
    }

    void draw_library_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Library);
        draw_quilted_background(target,{0,32,W,H-32},ViewMode::Library);

        // The Library tool row never wraps. Clip the scrolling tools before
        // the fixed far-right List/Grid controls so offscreen buttons cannot
        // paint over or steal space from those view toggles.
        const int libraryToolClipRight = std::max(libraryMoviesBtn.x + 1, libraryListViewBtn.x - 8);
        XRectangle libraryToolClip{static_cast<short>(page_content_frame(ViewMode::Library).x + 16),
                                   static_cast<short>(kPageControlY),
                                   static_cast<unsigned short>(std::max(1, libraryToolClipRight - (page_content_frame(ViewMode::Library).x + 16))),
                                   static_cast<unsigned short>(kCompactButtonH)};
        XSetClipRectangles(d, gc, 0, 0, &libraryToolClip, 1, Unsorted);
        button_on(target,libraryMoviesBtn,"Movies");
        button_on(target,libraryTvBtn,"TV");
        button_on(target,libraryAddFolderBtn,"Link Folder");
        button_on(target,libraryUnlinkFolderBtn,"Unlink Folder");
        button_on(target,libraryRefreshBtn,"Refresh Library");
        button_on(target,libraryBackBtn,"Back");
        apply_page_clip(ViewMode::Library);
        draw_library_view_button(target,libraryListViewBtn,LibraryDisplayMode::List,current_library_display_mode()==LibraryDisplayMode::List);
        draw_library_view_button(target,libraryGridBtn,LibraryDisplayMode::Grid,current_library_display_mode()==LibraryDisplayMode::Grid);

        // Exact sheet INPUT FIELD treatment on its own row below the green buttons.
        draw_concept_field(target, librarySearchRect, palette.field, palette.border, librarySearchFocused);
        XRectangle searchClip{static_cast<short>(librarySearchRect.x+6), static_cast<short>(librarySearchRect.y+3),
                              static_cast<unsigned short>(std::max(1,librarySearchRect.w-12)),
                              static_cast<unsigned short>(std::max(1,librarySearchRect.h-6))};
        XSetClipRectangles(d, gc, 0, 0, &searchClip, 1, Unsorted);
        const int searchBaseline = librarySearchRect.y + librarySearchRect.h/2 + 5;
        if (librarySearchQuery.empty()) {
            // Keep the requested placeholder plainly legible inside the exact-sheet field.
            text(target, librarySearchRect.x+10, searchBaseline, "Search", palette.text);
        } else {
            const std::string visibleSearch = tail_to_width(librarySearchQuery, librarySearchRect.w-22);
            if (librarySearchFocused && librarySearchSelectAll) {
                const int selectedW = std::min(text_width(visibleSearch)+4, std::max(1,librarySearchRect.w-16));
                fill(target,{librarySearchRect.x+7,librarySearchRect.y+5,selectedW,librarySearchRect.h-10},palette.selection);
            }
            text(target, librarySearchRect.x+10, searchBaseline, visibleSearch, palette.text);
            if (librarySearchFocused && !librarySearchSelectAll) {
                int caretX = librarySearchRect.x+10+text_width(visibleSearch);
                caretX = std::min(caretX,librarySearchRect.x+librarySearchRect.w-9);
                line(target,caretX,librarySearchRect.y+6,caretX,librarySearchRect.y+librarySearchRect.h-7,palette.text);
            }
        }
        apply_page_clip(ViewMode::Library);
        button_on(target, librarySearchBtn, "Search");

        std::vector<reddmedia::LibraryNode> nodes;
        std::string status;
        bool busy=false;
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            nodes=libraryState->nodes; status=libraryState->status; busy=libraryState->busy;
        }
        std::vector<int> visibleIndices;
        visibleIndices.reserve(nodes.size());
        for (std::size_t i=0; i<nodes.size(); ++i) {
            if (library_node_matches_search(nodes[i])) visibleIndices.push_back(static_cast<int>(i));
        }

        const Rect libraryFrame = page_content_frame(ViewMode::Library);
        std::string statusLine;
        if (!libraryParents.empty()) statusLine = libraryParents.back().name + "  |  ";
        statusLine += "Status: " + std::string(busy?"Working - ":"") + status;
        text(target,libraryFrame.x+16,libraryListBox.y-12,
             head_to_width(statusLine,libraryFrame.w-42),palette.text);
        draw_primary_panel(target, libraryListBox, palette);
        libraryRows.clear();
        libraryRowNodeIndices.clear();
        const LibraryGridMetrics grid=library_grid_metrics();
        const int max_scroll=std::max(0,static_cast<int>(visibleIndices.size())-grid.visibleItems);
        libraryScroll=std::max(0,std::min(libraryScroll,max_scroll));

        if (current_library_display_mode()==LibraryDisplayMode::List) {
            const int rowHeight=32;
            const int visible=std::max(1,(libraryListBox.h-12)/rowHeight);
            for(int visibleIndex=0;visibleIndex<visible;++visibleIndex) {
                const int filteredIndex=libraryScroll+visibleIndex;
                if(filteredIndex>=static_cast<int>(visibleIndices.size())) break;
                const int nodeIndex=visibleIndices[static_cast<std::size_t>(filteredIndex)];
                const auto& item=nodes[static_cast<std::size_t>(nodeIndex)];
                Rect row={libraryListBox.x+6,libraryListBox.y+6+visibleIndex*rowHeight,libraryListBox.w-12,rowHeight-2};
                if(nodeIndex==librarySelected) fill(target,row,palette.selection);
                outline(target,row,palette.border);
                const bool container=item.kind==reddmedia::LibraryNodeKind::MovieCollection || item.kind==reddmedia::LibraryNodeKind::Series || item.kind==reddmedia::LibraryNodeKind::Season;
                const std::string action=container?"Open":"Play";
                std::string details;
                if(item.production_year>0) details+=std::to_string(item.production_year);
                if(item.child_count>0) {
                    if(!details.empty()) details+="  |  ";
                    details+=std::to_string(item.child_count)+(item.kind==reddmedia::LibraryNodeKind::Series?" seasons/items":" items");
                }
                if(!item.technical_details.empty()) {
                    if(!details.empty()) details+="  |  ";
                    details+=item.technical_details;
                }
                const int actionW=60;
                const int detailsW=std::min(340,std::max(120,row.w/3));
                text(target,row.x+8,row.y+20,head_to_width(library_display_title(item),row.w-detailsW-actionW-30),palette.text);
                text(target,row.x+row.w-detailsW-actionW-12,row.y+20,head_to_width(details,detailsW),palette.muted);
                text(target,row.x+row.w-actionW,row.y+20,action,col(0x9f9f,0xd0d0,0xa7a7));
                libraryRows.push_back(row);
                libraryRowNodeIndices.push_back(nodeIndex);
            }
        } else {
            for(int visible_index=0;visible_index<grid.visibleItems;++visible_index) {
                const int filtered_index=libraryScroll+visible_index;
                if(filtered_index>=static_cast<int>(visibleIndices.size())) break;
                const int node_index=visibleIndices[static_cast<std::size_t>(filtered_index)];
                const auto& item=nodes[static_cast<std::size_t>(node_index)];
                const int column=visible_index%grid.columns;
                const int row_number=visible_index/grid.columns;
                Rect row={libraryListBox.x+6+column*(grid.tileWidth+grid.gap),
                          libraryListBox.y+6+row_number*(grid.tileHeight+grid.gap),grid.tileWidth,grid.tileHeight};
                if(node_index==librarySelected) fill(target,row,palette.selection);
                outline(target,row,palette.border);
                draw_library_poster(target,{row.x+4,row.y+4,row.w-8,grid.posterHeight},item);
                const int title_y=row.y+grid.posterHeight+18;
                text(target,row.x+6,title_y,head_to_width(library_display_title(item),row.w-12),palette.text);
                if(!item.technical_details.empty()) text(target,row.x+6,title_y+14,head_to_width(item.technical_details,row.w-12),palette.muted);
                const bool container=item.kind==reddmedia::LibraryNodeKind::MovieCollection || item.kind==reddmedia::LibraryNodeKind::Series || item.kind==reddmedia::LibraryNodeKind::Season;
                text(target,row.x+6,row.y+row.h-5,container?"Open":"Play",col(0x9f9f,0xd0d0,0xa7a7));
                libraryRows.push_back(row);
                libraryRowNodeIndices.push_back(node_index);
            }
        }
        if(visibleIndices.empty() && !busy) {
            const std::string emptyMessage = !librarySearchQuery.empty()
                ? "No titles match Search."
                : (libraryTypeChosen?"No real titles to show.":"Choose Movies or TV.");
            text(target,libraryListBox.x+12,libraryListBox.y+28,emptyMessage,palette.muted);
        }

        // Dedicated Library page scrollbar. It is inside the outer frame, never
        // on the raw window edge and never underneath poster/list content.
        if (max_scroll > 0) {
            const int thumbH = std::max(38, std::min(libraryVerticalScrollTrack.h,
                libraryVerticalScrollTrack.h * grid.visibleItems / std::max(1, static_cast<int>(visibleIndices.size()))));
            const int travel = std::max(0, libraryVerticalScrollTrack.h - thumbH);
            const int thumbY = libraryVerticalScrollTrack.y +
                (max_scroll > 0 ? travel * libraryScroll / max_scroll : 0);
            libraryVerticalScrollThumb = {libraryVerticalScrollTrack.x, thumbY, libraryVerticalScrollTrack.w, thumbH};
        } else {
            libraryVerticalScrollThumb = libraryVerticalScrollTrack;
        }
        draw_home_scrollbar_component(target, libraryVerticalScrollTrack, libraryVerticalScrollThumb, palette);
    }

    void refresh_live_tv_tuners(bool announce=true) {
        std::string status;
        liveTvTuners = tunerBackend.detect(status);
        liveTvChannels = tunerBackend.load_channels();
        if (liveTvTuners.empty()) liveTvSelectedTuner = -1;
        else if (liveTvSelectedTuner < 0 || liveTvSelectedTuner >= static_cast<int>(liveTvTuners.size())) liveTvSelectedTuner = 0;
        liveTvStatus = status;
        if (announce && liveTvTuners.empty())
            liveTvStatus += " Connect the WinTV-HVR-955Q, then press Detect Tuners again.";
    }

    void start_live_tv_scan() {
        if (liveTvTunerUse != LiveTvTunerUse::Idle) {
            liveTvStatus = liveTvTunerUse == LiveTvTunerUse::Watching
                ? "Stop Live TV playback before scanning channels."
                : "The tuner is already busy.";
            return;
        }
        if (liveTvSelectedTuner < 0 || liveTvSelectedTuner >= static_cast<int>(liveTvTuners.size())) {
            liveTvStatus = "Detect and select a tuner before scanning channels.";
            return;
        }
        if (liveTvScanWorker.joinable()) {
            bool busy = false;
            { std::lock_guard<std::mutex> lock(liveTvScanState->mutex); busy = liveTvScanState->busy; }
            if (busy) {
                liveTvStatus = "Channel scan is already running.";
                return;
            }
            liveTvScanWorker.join();
        }

        const reddmedia::TunerDevice tuner = liveTvTuners[static_cast<std::size_t>(liveTvSelectedTuner)];
        const reddmedia::NougatTunerBackend backend = tunerBackend;
        const auto state = liveTvScanState;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy = true;
            state->updated = true;
            state->cancel = false;
            state->finished = false;
            state->success = false;
            state->physical_channel = 0;
            state->frequency_hz = 0;
            state->completed = 0;
            state->total = 35;
            state->locked = false;
            state->signal_percent = -1;
            state->quality_percent = -1;
            state->channels_found = 0;
            state->channels.clear();
            state->status = "Starting native ATSC channel scan...";
        }
        liveTvStatus = "Starting native ATSC channel scan...";
        liveTvTunerUse = LiveTvTunerUse::Scanning;
        liveTvScanWorker = std::thread([state, tuner, backend]() mutable {
            std::vector<reddmedia::LiveTvChannel> channels;
            std::string status;
            const bool ok = backend.scan_channels(tuner, channels, status,
                [state](const reddmedia::ChannelScanProgress& progress) {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->physical_channel = progress.physical_channel;
                    state->frequency_hz = progress.frequency_hz;
                    state->completed = progress.completed;
                    state->total = progress.total;
                    state->locked = progress.locked;
                    state->signal_percent = progress.signal_percent;
                    state->quality_percent = progress.quality_percent;
                    state->channels_found = progress.channels_found;
                    state->status = progress.message;
                    state->updated = true;
                    return !state->cancel;
                });
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy = false;
            state->finished = true;
            state->success = ok;
            state->status = status.empty() ? (ok ? "Channel scan complete." : "Channel scan failed.") : status;
            state->channels = std::move(channels);
            state->channels_found = static_cast<int>(state->channels.size());
            state->updated = true;
        });
    }

    void poll_live_tv_scan() {
        bool updated = false;
        bool busy = false;
        bool finished = false;
        bool success = false;
        std::string status;
        std::vector<reddmedia::LiveTvChannel> channels;
        {
            std::lock_guard<std::mutex> lock(liveTvScanState->mutex);
            updated = liveTvScanState->updated;
            if (updated) liveTvScanState->updated = false;
            busy = liveTvScanState->busy;
            finished = liveTvScanState->finished;
            success = liveTvScanState->success;
            status = liveTvScanState->status;
            channels = liveTvScanState->channels;
        }
        if (!updated) return;
        liveTvStatus = status;
        if (!busy && finished) {
            liveTvTunerUse = LiveTvTunerUse::Idle;
            if (success) { liveTvChannels = std::move(channels); liveTvSelectedChannel = liveTvChannels.empty() ? -1 : 0; }
        }
        if (!fullscreen && currentView == ViewMode::LiveTV) redraw();
    }

    void start_live_tv_guide_refresh() {
        if (liveTvTunerUse != LiveTvTunerUse::Idle) {
            liveTvStatus = liveTvTunerUse == LiveTvTunerUse::Watching
                ? "Stop Live TV playback before refreshing the broadcast guide."
                : "The tuner is already busy.";
            return;
        }
        if (liveTvSelectedTuner < 0 || liveTvSelectedTuner >= static_cast<int>(liveTvTuners.size())) {
            liveTvStatus = "Detect and select a tuner before refreshing the guide.";
            return;
        }
        if (liveTvChannels.empty()) {
            liveTvStatus = "Scan channels before refreshing the broadcast guide.";
            return;
        }
        if (liveTvGuideWorker.joinable()) {
            bool busy=false;
            { std::lock_guard<std::mutex> lock(liveTvGuideState->mutex); busy=liveTvGuideState->busy; }
            if (busy) { liveTvStatus="Broadcast guide refresh is already running."; return; }
            liveTvGuideWorker.join();
        }
        const reddmedia::TunerDevice tuner=liveTvTuners[static_cast<std::size_t>(liveTvSelectedTuner)];
        const reddmedia::NougatTunerBackend backend=tunerBackend;
        const auto state=liveTvGuideState;
        std::vector<reddmedia::LiveTvChannel> channels=liveTvChannels;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=true; state->updated=true; state->cancel=false; state->finished=false; state->success=false;
            state->completed=0; state->total=0; state->programs_found=0;
            state->channels=channels; state->programs.clear(); state->status="Refreshing ATSC broadcast guide...";
        }
        liveTvTunerUse=LiveTvTunerUse::GuideRefreshing;
        liveTvStatus="Refreshing ATSC broadcast guide...";
        liveTvGuideWorker=std::thread([state,tuner,backend,channels]() mutable {
            std::vector<reddmedia::LiveTvProgram> programs;
            std::string status;
            const bool ok=backend.refresh_guide(tuner,channels,programs,status,[state](const reddmedia::ChannelScanProgress& progress){
                std::lock_guard<std::mutex> lock(state->mutex);
                state->completed=progress.completed; state->total=progress.total;
                state->programs_found=progress.channels_found; state->status=progress.message; state->updated=true;
                return !state->cancel;
            });
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=false; state->finished=true; state->success=ok;
            state->status=status.empty() ? (ok ? "Broadcast guide refreshed." : "Broadcast guide refresh failed.") : status;
            state->channels=std::move(channels); state->programs=std::move(programs);
            state->programs_found=static_cast<int>(state->programs.size()); state->updated=true;
        });
    }

    void poll_live_tv_guide() {
        bool updated=false,busy=false,finished=false,success=false;
        std::string status;
        std::vector<reddmedia::LiveTvChannel> channels;
        std::vector<reddmedia::LiveTvProgram> programs;
        {
            std::lock_guard<std::mutex> lock(liveTvGuideState->mutex);
            updated=liveTvGuideState->updated; if (updated) liveTvGuideState->updated=false;
            busy=liveTvGuideState->busy; finished=liveTvGuideState->finished; success=liveTvGuideState->success;
            status=liveTvGuideState->status; channels=liveTvGuideState->channels; programs=liveTvGuideState->programs;
        }
        if (!updated) return;
        liveTvStatus=status;
        if (!busy && finished) {
            liveTvTunerUse=LiveTvTunerUse::Idle;
            if (success) { liveTvChannels=std::move(channels); liveTvPrograms=std::move(programs); }
        }
        if (!fullscreen && currentView==ViewMode::LiveTV) redraw();
    }

    std::string live_tv_clock(long long unixTime) const {
        std::time_t stamp=static_cast<std::time_t>(unixTime);
        std::tm local{};
        localtime_r(&stamp,&local);
        char buffer[24]{};
        std::strftime(buffer,sizeof(buffer),"%I:%M %p",&local);
        std::string out(buffer);
        if (!out.empty() && out[0]=='0') out.erase(out.begin());
        return out;
    }

    void watch_live_tv_channel(int index) {
        if (index < 0 || index >= static_cast<int>(liveTvChannels.size())) { liveTvStatus="Select a channel to watch."; redraw(); return; }
        if (liveTvSelectedTuner < 0 || liveTvSelectedTuner >= static_cast<int>(liveTvTuners.size())) { liveTvStatus="Detect and select a tuner first."; redraw(); return; }
        if (liveTvTunerUse == LiveTvTunerUse::Scanning || liveTvTunerUse == LiveTvTunerUse::GuideRefreshing) { liveTvStatus="The tuner is busy. Wait for the current tuner job to finish."; redraw(); return; }
        if (currentMediaIsLiveTv) cleanup_player();
        const auto& channel=liveTvChannels[static_cast<std::size_t>(index)];
        std::string mrl,status; std::vector<std::string> options;
        if (!tunerBackend.live_playback_input(liveTvTuners[static_cast<std::size_t>(liveTvSelectedTuner)],channel,mrl,options,status)) { liveTvStatus=status; redraw(); return; }
        liveTvStatus=status;
        const std::string label=channel.id+" "+channel.name;
        if (!open_live_tv_location(mrl,options,index,label)) {
            liveTvTunerUse=LiveTvTunerUse::Idle;
            liveTvStatus="VLC could not open the ATSC tuner stream for "+label+".";
            switch_view(ViewMode::LiveTV); redraw(); return;
        }
    }

    void draw_live_tv_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::LiveTV);
        const Rect frame = page_content_frame(ViewMode::LiveTV);
        draw_quilted_background(target, frame, ViewMode::LiveTV);
        liveTvChannelHitboxes.clear();

        bool scanBusy=false, guideBusy=false;
        int scanPhysical=0,scanCompleted=0,scanTotal=35,scanSignal=-1,scanQuality=-1,scanFound=0;
        bool scanLocked=false;
        {
            std::lock_guard<std::mutex> lock(liveTvScanState->mutex);
            scanBusy=liveTvScanState->busy; scanPhysical=liveTvScanState->physical_channel;
            scanCompleted=liveTvScanState->completed; scanTotal=liveTvScanState->total;
            scanSignal=liveTvScanState->signal_percent; scanQuality=liveTvScanState->quality_percent;
            scanFound=liveTvScanState->channels_found; scanLocked=liveTvScanState->locked;
        }
        {
            std::lock_guard<std::mutex> lock(liveTvGuideState->mutex);
            guideBusy=liveTvGuideState->busy;
        }

        button_on(target,liveTvDetectBtn,"Detect Tuner");
        button_on(target,liveTvRefreshBtn,"Refresh Tuner");
        button_on(target,liveTvScanBtn,scanBusy ? "Scanning..." : "Scan Channels");
        button_on(target,liveTvWatchBtn,"Watch Live");
        button_on(target,liveTvChannelsBtn,"Channels");
        button_on(target,liveTvGuideBtn,"Guide");
        button_on(target,liveTvGuideRefreshBtn,guideBusy ? "Guide..." : "Refresh Guide");
        button_on(target,liveTvNowBtn,"Now");
        button_on(target,liveTvRecordBtn,"Record");

        draw_primary_panel(target, liveTvListBox, palette);
        section_text(target, liveTvListBox.x+12, liveTvListBox.y+24,
                     liveTvGuideMode ? "LIVE TV GUIDE" : "LIVE TV", palette.text);
        text(target,liveTvListBox.x+12,liveTvListBox.y+48,
             head_to_width("Status: "+liveTvStatus,liveTvListBox.w-24),palette.text);

        if (liveTvGuideMode) {
            const int top=liveTvListBox.y+66;
            const int left=liveTvListBox.x+8;
            const int channelW=132;
            const int headerH=34;
            const int rowH=46;
            const int gridX=left+channelW;
            const int gridW=std::max(120,liveTvListBox.w-channelW-16);
            const int slotCount=std::max(3,std::min(8,gridW/112));
            const int slotW=std::max(1,gridW/slotCount);
            const long long now=static_cast<long long>(std::time(nullptr));
            const long long base=(now/1800LL)*1800LL + static_cast<long long>(liveTvGuideTimeOffsetSlots)*1800LL;
            const long long windowEnd=base+static_cast<long long>(slotCount)*1800LL;

            fill(target,{left,top,channelW,headerH},palette.button);
            outline(target,{left,top,channelW,headerH},palette.border);
            text(target,left+8,top+22,"CHANNEL",palette.buttonText);
            for (int slot=0;slot<slotCount;++slot) {
                Rect h{gridX+slot*slotW,top,slotW,headerH};
                fill(target,h,palette.button); outline(target,h,palette.border);
                text(target,h.x+6,h.y+22,head_to_width(live_tv_clock(base+slot*1800LL),h.w-10),palette.buttonText);
            }

            const int visibleRows=std::max(1,(liveTvListBox.y+liveTvListBox.h-(top+headerH)-8)/rowH);
            const int maxScroll=std::max(0,static_cast<int>(liveTvChannels.size())-visibleRows);
            liveTvGuideChannelScroll=std::max(0,std::min(liveTvGuideChannelScroll,maxScroll));
            for (int row=0;row<visibleRows;++row) {
                const int index=liveTvGuideChannelScroll+row;
                if (index>=static_cast<int>(liveTvChannels.size())) break;
                const auto& channel=liveTvChannels[static_cast<std::size_t>(index)];
                const int ry=top+headerH+row*rowH;
                Rect channelRect{left,ry,channelW,rowH};
                fill(target,channelRect,index==liveTvSelectedChannel?palette.selection:palette.panel);
                outline(target,channelRect,palette.border);
                text(target,channelRect.x+7,channelRect.y+18,channel.id+"  "+channel.name,palette.text);
                text(target,channelRect.x+7,channelRect.y+36,"RF "+std::to_string(channel.physical_channel),palette.muted);
                liveTvChannelHitboxes.push_back({channelRect,index});

                Rect rowBg{gridX,ry,gridW,rowH};
                fill(target,rowBg,palette.background); outline(target,rowBg,palette.border);
                bool drew=false;
                for (std::size_t pi=0;pi<liveTvPrograms.size();++pi) {
                    const auto& program=liveTvPrograms[pi];
                    if (program.channel_id!=channel.id || program.duration_seconds<=0) continue;
                    const long long eventEnd=program.start_unix+program.duration_seconds;
                    if (eventEnd<=base || program.start_unix>=windowEnd) continue;
                    const long long visibleStart=std::max(base,program.start_unix);
                    const long long visibleEnd=std::min(windowEnd,eventEnd);
                    const int px=gridX+static_cast<int>((visibleStart-base)*gridW/(windowEnd-base));
                    const int pr=gridX+static_cast<int>((visibleEnd-base)*gridW/(windowEnd-base));
                    Rect block{px+1,ry+2,std::max(34,pr-px-2),rowH-4};
                    const bool current=program.start_unix<=now && now<eventEnd;
                    fill(target,block,current?palette.selection:palette.button);
                    outline(target,block,palette.border);
                    text(target,block.x+6,block.y+17,head_to_width(program.title,block.w-12),current?palette.text:palette.buttonText);
                    if (block.w>92) text(target,block.x+6,block.y+34,
                        head_to_width(live_tv_clock(program.start_unix)+"-"+live_tv_clock(eventEnd),block.w-12),
                        current?palette.muted:palette.buttonText);
                    drew=true;
                }
                if (!drew) text(target,gridX+8,ry+27,"No broadcast guide data cached",palette.muted);
            }
            if (base<=now && now<windowEnd) {
                const int nx=gridX+static_cast<int>((now-base)*gridW/(windowEnd-base));
                line(target,nx,top,nx,std::min(liveTvListBox.y+liveTvListBox.h-8,top+headerH+visibleRows*rowH),rgb8(244,197,72));
            }
            if (liveTvPrograms.empty()) {
                text(target,left+8,liveTvListBox.y+liveTvListBox.h-12,
                     "Press Refresh Guide to collect ATSC PSIP program listings.",palette.muted);
            }
            return;
        }

        int y=liveTvListBox.y+76;
        if (scanBusy) {
            std::ostringstream progress;
            progress << "Scan: " << scanCompleted << "/" << scanTotal;
            if (scanPhysical>0) progress << " | RF " << scanPhysical;
            progress << " | " << (scanLocked?"LOCK":"searching") << " | found " << scanFound;
            text(target,liveTvListBox.x+12,y,progress.str(),palette.text); y+=20;
            if (scanSignal>=0) { text(target,liveTvListBox.x+12,y,"Signal strength: "+std::to_string(scanSignal)+"%",palette.muted); y+=20; }
            if (scanQuality>=0) { text(target,liveTvListBox.x+12,y,"Signal quality: "+std::to_string(scanQuality)+"%",palette.muted); y+=20; }
        }
        if (liveTvTuners.empty()) {
            text(target,liveTvListBox.x+12,y,"No tuner detected yet. Press Detect Tuner.",palette.muted); y+=36;
        } else {
            // One owner-visible row per independently usable DVB frontend. Raw
            // VBI/video nodes stay implementation details instead of fake tuners.
            const auto& tuner=liveTvTuners[static_cast<std::size_t>(std::max(0,liveTvSelectedTuner))];
            Rect row{liveTvListBox.x+8,y-16,liveTvListBox.w-16,34};
            fill(target,row,palette.selection); outline(target,row,palette.border);
            const std::string lead=tuner.hauppauge?"[Hauppauge] ":"[Tuner] ";
            text(target,row.x+8,row.y+21,head_to_width(lead+tuner.name+"  ATSC 1.0  "+tuner.status,row.w-16),palette.text);
            y+=46;
        }
        y=std::max(y,liveTvListBox.y+142);
        text(target,liveTvListBox.x+12,y,"Channels stored: "+std::to_string(liveTvChannels.size()),palette.text); y+=24;
        const int visibleRows=std::max(1,(liveTvListBox.y+liveTvListBox.h-y-10)/26);
        const int maxScroll=std::max(0,static_cast<int>(liveTvChannels.size())-visibleRows);
        liveTvGuideChannelScroll=std::max(0,std::min(liveTvGuideChannelScroll,maxScroll));
        for (int row=0;row<visibleRows;++row) {
            const int index=liveTvGuideChannelScroll+row;
            if (index>=static_cast<int>(liveTvChannels.size())) break;
            const auto& channel=liveTvChannels[static_cast<std::size_t>(index)];
            Rect hit{liveTvListBox.x+10,y-18,liveTvListBox.w-20,24};
            if (index==liveTvSelectedChannel) fill(target,hit,palette.selection);
            outline(target,hit,palette.border);
            text(target,hit.x+8,hit.y+17,head_to_width(channel.id+"  "+channel.name+"  "+channel.service,hit.w-16),palette.text);
            liveTvChannelHitboxes.push_back({hit,index});
            y+=26;
        }
    }

    void handle_live_tv_click(int x,int y, Time eventTime) {
        if (liveTvDetectBtn.contains(x,y) || liveTvRefreshBtn.contains(x,y)) { refresh_live_tv_tuners(true); redraw(); return; }
        if (liveTvScanBtn.contains(x,y)) { start_live_tv_scan(); redraw(); return; }
        if (liveTvChannelsBtn.contains(x,y)) { liveTvGuideMode=false; redraw(); return; }
        if (liveTvGuideBtn.contains(x,y)) { liveTvGuideMode=true; liveTvPrograms=tunerBackend.load_guide(); redraw(); return; }
        if (liveTvGuideRefreshBtn.contains(x,y)) { liveTvGuideMode=true; start_live_tv_guide_refresh(); redraw(); return; }
        if (liveTvNowBtn.contains(x,y)) { liveTvGuideTimeOffsetSlots=0; liveTvGuideMode=true; redraw(); return; }
        if (liveTvWatchBtn.contains(x,y)) { watch_live_tv_channel(liveTvSelectedChannel); return; }
        if (liveTvRecordBtn.contains(x,y)) { liveTvStatus="Recording is reserved for the next DVR stage; Watch Live and Guide are active first."; redraw(); return; }

        for (const auto& hit:liveTvChannelHitboxes) {
            if (!hit.rect.contains(x,y)) continue;
            const bool doubleClick=hit.channel_index==liveTvLastClickChannel && liveTvLastClickTime!=0 &&
                eventTime>=liveTvLastClickTime && eventTime-liveTvLastClickTime<=450;
            liveTvSelectedChannel=hit.channel_index;
            liveTvLastClickChannel=hit.channel_index; liveTvLastClickTime=eventTime;
            if (doubleClick) { liveTvLastClickTime=0; watch_live_tv_channel(hit.channel_index); return; }
            liveTvStatus="Selected "+liveTvChannels[static_cast<std::size_t>(hit.channel_index)].id+" "+
                liveTvChannels[static_cast<std::size_t>(hit.channel_index)].name+". Double-click or press Watch Live.";
            redraw(); return;
        }
    }

    void start_discover_task(reddmedia::RecommendationSource source,
                             reddmedia::RecommendationMediaType media_type) {
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) return;
        }
        if (!discoverTargetSelected || discoverSource != source || discoverMediaType != media_type) push_navigation_history();
        discoverSource = source;
        discoverMediaType = media_type;
        discoverTargetSelected = true;
        if (source == reddmedia::RecommendationSource::LiveTV) {
            liveTvChannels = tunerBackend.load_channels();
            discoverDetailsScroll = 0;
            discoverServiceSettings = false;
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->busy = false;
            discoverState->updated = true;
            discoverState->hasResult = false;
            discoverState->hasPoster = false;
            discoverState->hasAvailability = false;
            discoverState->availabilityStatus.clear();
            discoverState->progress = 1.0;
            if (liveTvChannels.empty()) {
                discoverState->status = std::string(discoverMode == reddmedia::RecommendationMode::Usual ? "Usual" : "Random") +
                    " + Live TV selected. No Live TV channels are stored yet; complete channel scanning in Live TV first.";
            } else {
                discoverState->status = std::string(discoverMode == reddmedia::RecommendationMode::Usual ? "Usual" : "Random") +
                    " + Live TV selected. Channels are stored; program-guide recommendations will activate when EPG data is connected.";
            }
            redraw();
            return;
        }
        if (source == reddmedia::RecommendationSource::External &&
            !recommendationEngine->external_credential_available()) {
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->status =
                    "TMDb recommendations need a validated TMDb credential. Use Save / Replace.";
                discoverState->hasResult = false;
                discoverState->hasPoster = false;
                discoverState->hasAvailability = false;
            }
            redraw();
            return;
        }
        if (discoverWorker.joinable()) discoverWorker.join();
        discoverDetailsScroll = 0;
        discoverServiceSettings = false;
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->busy = true;
            discoverState->updated = false;
            discoverState->hasResult = false;
            discoverState->hasPoster = false;
            discoverState->hasAvailability = false;
            discoverState->availabilityStatus.clear();
            discoverState->progress = 0.05;
            discoverState->status = "Finding one real recommendation...";
        }
        redraw();
        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        const std::shared_ptr<DiscoverUiState> state = discoverState;
        const reddmedia::RecommendationMode mode = discoverMode;
        const std::string watch_region = watchPreferences.region();
        discoverWorker = std::thread([client, engine, state, source, media_type, mode,
                                      watch_region]() {
            const auto set_progress = [state](double progress) {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = progress;
            };
            std::string error;
            std::vector<reddmedia::LibraryNode> nodes;
            bool ok = client->load_all_recommendation_items(nodes, error);
            set_progress(0.28);
            std::vector<reddmedia::MediaDescriptor> local_items;
            if (ok) {
                for (const reddmedia::LibraryNode& node : nodes) {
                    reddmedia::MediaDescriptor item;
                    item.id = node.id;
                    item.title = node.name;
                    item.overview = node.overview;
                    item.genres = node.genres;
                    item.local_path = node.path;
                    item.tmdb_id = node.tmdb_id;
                    item.poster_path = node.primary_image_tag;
                    item.year = node.production_year;
                    item.media_type = (node.kind == reddmedia::LibraryNodeKind::Series ||
                                       node.kind == reddmedia::LibraryNodeKind::Season ||
                                       node.kind == reddmedia::LibraryNodeKind::Episode)
                        ? reddmedia::RecommendationMediaType::Television
                        : reddmedia::RecommendationMediaType::Movie;
                    local_items.push_back(std::move(item));
                }
            }
            reddmedia::RecommendationResult result;
            if (ok) {
                reddmedia::RecommendationRequest request;
                request.source = source;
                request.media_type = media_type;
                request.mode = mode;
                ok = engine->recommend(request, local_items, result, error);
            }
            set_progress(0.78);
            reddmedia::LibraryPoster poster;
            bool has_poster = false;
            if (ok && !result.item.poster_path.empty()) {
                std::string bytes;
                std::string poster_error;
                const bool loaded = source == reddmedia::RecommendationSource::External
                    ? engine->load_external_poster_bmp(
                        result.item.poster_path, 180, 260, bytes, poster_error)
                    : client->load_primary_image_bmp(
                        result.item.id, result.item.poster_path, 180, 260, bytes, poster_error);
                has_poster = loaded &&
                    reddmedia::decode_library_poster_bmp(bytes, poster, poster_error);
            }
            set_progress(0.88);
            reddmedia::WatchAvailability availability;
            bool has_availability = false;
            std::string availability_status;
            if (ok && source == reddmedia::RecommendationSource::External &&
                !result.item.tmdb_id.empty()) {
                std::string availability_error;
                has_availability = engine->load_watch_availability(
                    media_type, result.item.tmdb_id, watch_region,
                    availability, availability_error);
                if (!has_availability) {
                    availability_status = "Watch availability failed: " + availability_error;
                }
            }
            set_progress(0.95);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (ok) {
                state->result = std::move(result);
                state->poster = std::move(poster);
                state->hasPoster = has_poster;
                state->availability = std::move(availability);
                state->hasAvailability = has_availability;
                state->availabilityStatus = std::move(availability_status);
                state->hasResult = true;
                state->status = "One recommendation ready.";
            } else {
                state->hasResult = false;
                state->status = error;
            }
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
        });
    }

    void start_tmdb_credential_task(int operation, const std::string& credential = {}) {
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) return;
        }
        if (discoverWorker.joinable()) discoverWorker.join();
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->busy = true;
            discoverState->updated = false;
            discoverState->progress = 0.05;
            if (operation == 1) discoverState->status = "Testing the saved TMDb credential...";
            else if (operation == 2) discoverState->status =
                "Validating the replacement before saving it...";
            else discoverState->status = "Clearing the saved TMDb credential...";
        }
        redraw();
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        const std::shared_ptr<DiscoverUiState> state = discoverState;
        discoverWorker = std::thread([engine, state, operation, credential]() {
            std::string error;
            bool ok = false;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = operation == 3 ? 0.50 : 0.25;
            }
            if (operation == 1) ok = engine->test_external_credential(error);
            else if (operation == 2) ok = engine->save_external_credential(credential, error);
            else ok = engine->clear_external_credential(error);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (ok) {
                if (operation == 1) state->status = "TMDb credential test passed.";
                else if (operation == 2) state->status =
                    "Validated and saved: " + engine->external_credential_label() + ".";
                else {
                    state->status = "Saved TMDb credential cleared.";
                    state->hasResult = false;
                    state->hasPoster = false;
                    state->hasAvailability = false;
                    state->providerCatalog.clear();
                    state->providerCatalogLoaded = false;
                }
            } else state->status = error;
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
        });
    }

    void start_provider_catalog_task() {
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) return;
            if (discoverState->providerCatalogLoaded) {
                discoverServiceSettings = true;
                discoverServicesScroll = 0;
                redraw();
                return;
            }
        }
        if (!recommendationEngine->external_credential_available()) {
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->status = "My Services needs a validated TMDb credential.";
            }
            redraw();
            return;
        }
        if (discoverWorker.joinable()) discoverWorker.join();
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->busy = true;
            discoverState->updated = false;
            discoverState->progress = 0.10;
            discoverState->status = "Loading United States watch services...";
        }
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        const std::shared_ptr<DiscoverUiState> state = discoverState;
        const std::string region = watchPreferences.region();
        discoverWorker = std::thread([engine, state, region]() {
            std::vector<reddmedia::WatchProvider> providers;
            std::string error;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = 0.45;
            }
            const bool ok = engine->load_watch_provider_catalog(region, providers, error);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (ok) {
                state->providerCatalog = std::move(providers);
                state->providerCatalogLoaded = true;
                state->status = "My Services ready.";
            } else state->status = error;
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
        });
    }

    void poll_discover_worker() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            updated = discoverState->updated;
            if (updated) discoverState->updated = false;
        }
        if (!updated) return;
        if (discoverWorker.joinable()) discoverWorker.join();
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->providerCatalogLoaded &&
                discoverState->status == "My Services ready.") {
                discoverServiceSettings = true;
            }
        }
        if (!fullscreen && (currentView == ViewMode::Discover || currentView == ViewMode::Debug)) {
            redraw();
        }
    }

    std::string current_view_name() const {
        switch (currentView) {
        case ViewMode::Home: return "Home";
        case ViewMode::VideoPlayer: return "Video Player";
        case ViewMode::Library: return "Library";
        case ViewMode::Discover: return "Discover";
        case ViewMode::LiveTV: return "Live TV";
        case ViewMode::Nougat: return "Search";
        case ViewMode::Stream: return "Stream";
        case ViewMode::Studio: return "Studio";
        case ViewMode::P2P: return "P2P";
        case ViewMode::Debug: return "System";
        }
        return "Unknown";
    }

    static std::string playback_state_name(int state) {
        switch (state) {
        case 0: return "Nothing special";
        case 1: return "Opening";
        case 2: return "Buffering";
        case 3: return "Playing";
        case 4: return "Paused";
        case 5: return "Stopped";
        case 6: return "Ended";
        case 7: return "Error";
        default: return "Unknown";
        }
    }

    reddmedia::DiagnosticInput diagnostic_input() {
        reddmedia::DiagnosticInput input;
        input.app_version = "Nougat Media Suite v0.0.37";
        input.executable_path = resolved_executable_path();
        input.project_root = exe_dir();
        input.current_view = current_view_name();
        input.runtime_path = mediaServer.runtime_path();
        input.data_path = mediaServer.data_path();
        input.config_path = mediaServer.config_path();
        input.cache_path = mediaServer.cache_path();
        input.log_path = mediaServer.log_path();
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            input.server_state = serverState->state;
            input.server_owned = serverState->owned;
            input.server_api_ready = input.server_state == reddmedia::MediaServerState::Ready;
            input.server_busy = serverState->busy;
            if (serverState->busy) input.active_operation = serverState->status;
        }
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            input.library_folders = libraryState->folders;
            input.library_nodes = libraryState->nodes;
            input.library_status = libraryState->status;
            if (libraryState->busy) input.active_operation = libraryState->status;
        }
        {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            input.poster_failures = posterState->failed.size();
            if (posterState->busy) input.active_operation = "Loading library artwork.";
        }
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            input.tmdb_status = discoverState->status;
            if (discoverState->busy) input.active_operation = discoverState->status;
        }
        input.vlc_probe_attempted = true;
        input.vlc_loaded = api.handle != nullptr;
        input.vlc_version = api.get_version ? api.get_version() : (api.handle ? "Loaded; version symbol unavailable" : "Unavailable");
        input.vlc_error = vlcErr;
        input.playback_active = mp != nullptr && hasMedia;
        input.playback_path = currentPath.empty() ? (activeLibraryItemValid ? activeLibraryItem.path : std::string()) : currentPath;
        input.playback_state = (mp && api.get_state) ? playback_state_name(api.get_state(mp)) : (mp ? "Player active; state unavailable" : "Idle");
        input.playback_position_ms = mp && api.get_time ? api.get_time(mp) : 0;
        input.playback_length_ms = mp && api.get_length ? api.get_length(mp) : 0;
        input.volume_percent = volumePercent;
        input.tv_autoplay_armed = tvAutoplayArmed;
        input.up_next_visible = upNextVisible;
        input.up_next_seconds = up_next_seconds_remaining();
        input.up_next_title = upNextVisible ? library_display_title(upNextEpisode) : "None";
        input.search_data_dir = nougat.data_directory();
        input.search_node_running = nougat.node_running();
        input.search_node_port = nougat.node_port();
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            input.search_node_id = nougatState->node_id;
            input.search_peer_count = nougatState->peers.size();
            input.search_probe_error = nougatState->status;
        }
        const P2PStatus p2p_status = p2p.status();
        input.p2p_version = p2p.libtorrent_version();
        input.p2p_active = p2p_status.active;
        input.p2p_metadata_ready = p2p_status.metadata_ready;
        input.p2p_seeding = p2p_status.seeding;
        input.p2p_paused = p2p_status.paused;
        input.p2p_progress = p2p_status.progress;
        input.p2p_downloaded = p2p_status.downloaded;
        input.p2p_total = p2p_status.total;
        input.p2p_download_rate = p2p_status.download_rate;
        input.p2p_upload_rate = p2p_status.upload_rate;
        input.p2p_peers = p2p_status.peers;
        input.p2p_seeds = p2p_status.seeds;
        input.p2p_name = p2p_status.name;
        input.p2p_state = p2p_status.state;
        input.p2p_error = p2p_status.error;
        input.p2p_save_path = p2p_status.save_path;
        input.p2p_selected_progress = p2p_status.selected_progress;
        input.p2p_selected_buffered_bytes = p2p_status.selected_buffered_bytes;
        input.p2p_stream_running = p2pStream.running();
        input.tmdb_configured = recommendationEngine->external_credential_available();
        input.ai_model_path = exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf";
        input.ai_runtime_path = exe_dir() + "/components/ai/runtime";
        if (exists_file(input.ai_model_path)) {
            const std::string hash_line = run_command_capture("sha256sum " + shell_quote(input.ai_model_path) + " 2>/dev/null");
            const std::size_t space = hash_line.find(' ');
            if (space != std::string::npos) input.ai_model_sha256 = hash_line.substr(0, space);
        }
        input.stream_engine_path = ytdlp_engine_path();
        if (exists_file(input.stream_engine_path)) {
            input.stream_engine_version = run_command_capture(shell_quote(input.stream_engine_path) + " --version 2>/dev/null");
        }
        input.stream_provider = stream_platform_name(streamPlatform);
        input.stream_status = ytdlpStatus;
        input.stream_process_running = ytdlpPid > 0 || ytdlpStream.running();
        return input;
    }

    void start_debug_task() {
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            if (debugState->busy) return;
        }
        if (debugWorker.joinable()) debugWorker.join();
        reddmedia::DiagnosticInput input = diagnostic_input();
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            debugState->busy = true;
            debugState->updated = false;
            debugState->progress = 0.10;
            debugState->status = "Running evidence-based health checks...";
        }
        redraw();
        const std::shared_ptr<DebugUiState> state = debugState;
        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const reddmedia::DiagnosticEngine engine = diagnosticEngine;
        debugWorker = std::thread([state, client, engine, input]() mutable {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = 0.35;
            }
            std::string library_error;
            if (client && client->initialize(library_error)) {
                std::vector<reddmedia::MediaFolder> folders;
                std::vector<reddmedia::LibraryNode> nodes;
                if (client->load_media_folders(folders, library_error) &&
                    client->load_all_recommendation_items(nodes, library_error)) {
                    input.library_folders = std::move(folders);
                    input.library_nodes = std::move(nodes);
                    input.library_full_scan = true;
                } else input.library_scan_error = library_error;
            } else input.library_scan_error = library_error;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = 0.65;
            }
            reddmedia::DiagnosticReport report = engine.evaluate(input);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->input = input;
            state->report = std::move(report);
            state->hasReport = true;
            state->busy = false;
            state->updated = true;
            state->progress = 1.0;
            state->status = "Checks complete. Only evidence-backed findings are shown.";
        });
    }

    void poll_debug_worker() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            updated = debugState->updated;
            if (updated) debugState->updated = false;
        }
        if (!updated) return;
        if (debugWorker.joinable()) debugWorker.join();
        debugScroll = 0;
        if (!fullscreen && currentView == ViewMode::Debug) redraw();
    }

    void launch_external_target(const std::string& target) {
        if (target.empty()) return;
        const pid_t child = fork();
        if (child != 0) return;
        const int null_descriptor = open("/dev/null", O_RDWR);
        if (null_descriptor >= 0) {
            dup2(null_descriptor, STDIN_FILENO);
            dup2(null_descriptor, STDOUT_FILENO);
            dup2(null_descriptor, STDERR_FILENO);
        }
        execlp("xdg-open", "xdg-open", target.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    void open_watch_options() {
        std::string link;
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (!discoverState->hasAvailability) return;
            link = discoverState->availability.link;
        }
        if (link.rfind("https://www.themoviedb.org/", 0U) != 0U) {
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->availabilityStatus =
                    "TMDb did not provide a safe official watch-options link.";
            }
            redraw();
            return;
        }
        launch_external_target(link);
    }

    void open_debug_logs() {
        launch_external_target(mediaServer.log_path());
    }

    void copy_debug_report() {
        reddmedia::DiagnosticReport report;
        reddmedia::DiagnosticInput input;
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            if (!debugState->hasReport) return;
            report = debugState->report;
            input = debugState->input;
            debugState->status = "Diagnostic report copied with credentials redacted.";
        }
        own_clipboard_text(reddmedia::DiagnosticEngine::report_text(report, input));
        redraw();
    }

    std::string diagnostic_export_path(const std::string& suffix) const {
        const std::time_t now = std::time(nullptr);
        std::tm local {};
        localtime_r(&now, &local);
        char stamp[32];
        std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local);
        return home_dir() + "/Downloads/Nougat_Media_Suite_Diagnostic_" + stamp + suffix;
    }

    void export_debug_report(int mode) {
        reddmedia::DiagnosticReport report;
        reddmedia::DiagnosticInput input;
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            if (!debugState->hasReport) {
                debugState->status = "Run Checks before exporting diagnostics.";
                redraw();
                return;
            }
            report = debugState->report;
            input = debugState->input;
        }
        const std::string suffix = mode == 1 ? ".txt" : (mode == 2 ? ".json" : "_Support_Bundle.tar.gz");
        const std::string path = diagnostic_export_path(suffix);
        std::string error;
        bool ok = false;
        if (mode == 1) ok = reddmedia::DiagnosticEngine::write_text_report(report, input, path, error);
        else if (mode == 2) ok = reddmedia::DiagnosticEngine::write_json_report(report, input, path, error);
        else ok = reddmedia::DiagnosticEngine::write_support_bundle(report, input, path, error);
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            debugState->status = ok ? ("Diagnostic export saved: " + path) : ("Diagnostic export failed: " + error);
        }
        redraw();
    }

    unsigned long nougat_cocoa() { return col(0x1717,0x0d0d,0x0808); }
    unsigned long nougat_chocolate() { return col(0x4a4a,0x1f1f,0x0b0b); }
    unsigned long nougat_tan() { return col(0xd2d2,0xa5a5,0x6d6d); }
    unsigned long nougat_caramel() { return col(0xc9c9,0x8282,0x2c2c); }
    unsigned long nougat_cream() { return col(0xf2f2,0xe6e6,0xc9c9); }
    unsigned long nougat_light() { return col(0xdfdf,0xc6c6,0xa5a5); }
    unsigned long nougat_dark() { return col(0x1a1a,0x1212,0x0e0e); }

    void nougat_button(Drawable target, const Rect& r, const std::string& label, bool selected=false) {
        const ViewPalette palette = palette_for(ViewMode::Nougat);
        const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);
        const SheetControlState state = selected ? SheetControlState::Pressed :
            (hover ? SheetControlState::Hover : SheetControlState::Normal);
        draw_sheet_button_surface(target, r, palette, state);
        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        const int labelX = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int labelY = visual.y + visual.h / 2 + 5;
        text(target, labelX, labelY, head_to_width(label, visual.w - 10), sheet_button_ink(palette, state));
    }

    void nougat_tab_button(Drawable target, const Rect& r, const std::string& label, bool active) {
        const ViewPalette palette = palette_for(ViewMode::Nougat);
        const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);
        draw_sheet_tab_surface(target, r, palette, active, hover);
        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        const int labelX = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int labelY = visual.y + visual.h / 2 + 5;
        text(target, labelX, labelY, head_to_width(label, visual.w - 10), palette.buttonText);
    }

    void draw_nougat_panel(Drawable target, const Rect& r) {
        draw_sheet_panel_surface(target, r, palette_for(ViewMode::Nougat));
    }

    void draw_nougat_input(Drawable target, const Rect& r, const std::string& value, NougatInputFocus field) {
        const ViewPalette palette = palette_for(ViewMode::Nougat);
        draw_concept_field(target, r, palette.field, palette.border, field == nougatInputFocus);
        const std::string shown = tail_to_width(value, std::max(20, (int)r.w - 16));
        if (nougatInputFocus == field && nougatInputSelectAll && !value.empty()) {
            fill_round(target, {r.x+5, r.y+5, std::min((int)r.w-10, text_width(shown)+6), (int)r.h-10}, 4, palette.selection);
        }
        text(target, r.x+8, r.y+20, shown, palette.text);
        if (nougatInputFocus == field && !nougatInputSelectAll) {
            int cx = r.x + 8 + text_width(shown);
            cx = std::min(cx, r.x + r.w - 8);
            line(target, cx, r.y + 5, cx, r.y + r.h - 5, palette.text);
        }
    }

    std::string& focused_nougat_text() {
        if (nougatInputFocus == NougatInputFocus::CrawlSeed) return nougatCrawlSeed;
        if (nougatInputFocus == NougatInputFocus::Peer) return nougatPeerEntry;
        return nougatSearchQuery;
    }

    void focus_nougat_input(NougatInputFocus focus) {
        nougatInputFocus = focus;
        nougatInputSelectAll = false;
        nougatOutputFocused = false;
        XSetInputFocus(d, win, RevertToParent, CurrentTime);
        redraw();
    }

    void start_nougat_search() {
        if (nougatSearchQuery.empty()) return;
        if (nougatSearchWorker.joinable()) {
            bool busy = false; { std::lock_guard<std::mutex> lock(nougatState->mutex); busy = nougatState->search_busy; }
            if (busy) return;
            nougatSearchWorker.join();
        }
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->search_busy = true;
            nougatState->status = "Searching Nougat...";
            nougatState->updated = true;
        }
        const std::string query = nougatSearchQuery;
        const bool raw = nougatRaw;
        const bool include_peers = nougatSearchPeers;
        const int offset = nougatSearchOffset;
        const auto state = nougatState;
        nougatSearchWorker = std::thread([this, state, query, raw, include_peers, offset]() {
            reddmedia::NougatSearchResponse response = nougat.search(query, raw, include_peers, 100, offset);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->search = response;
            state->search_busy = false;
            if (!response.error.empty()) state->status = "Search error: " + response.error;
            else {
                std::ostringstream status;
                status << (raw ? "RAW" : "RANKED") << ": " << response.total << " matching record(s) reported";
                if (!response.results.empty()) status << " | showing " << (offset + 1) << "-" << (offset + (int)response.results.size());
                state->status = status.str();
            }
            state->updated = true;
        });
    }

    void append_nougat_crawl_log(const std::string& value) {
        std::lock_guard<std::mutex> lock(nougatState->mutex);
        nougatState->crawl_log.push_back(value);
        if (nougatState->crawl_log.size() > 5000U) nougatState->crawl_log.erase(nougatState->crawl_log.begin(), nougatState->crawl_log.begin()+1000);
        nougatState->updated = true;
    }

    void start_nougat_crawl() {
        if (nougatCrawlWorker.joinable()) {
            bool busy = false; { std::lock_guard<std::mutex> lock(nougatState->mutex); busy = nougatState->crawl_busy; }
            if (busy) return;
            nougatCrawlWorker.join();
        }
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->crawl_log.clear();
            nougatState->crawl_busy = true;
            nougatState->status = "Crawler running...";
            nougatState->updated = true;
        }
        nougatCrawlScroll = 0;
        nougatOutputSelectionStart = nougatOutputSelectionEnd = -1;
        const std::string seed = nougatCrawlSeed;
        const int max_pages = nougatMaxPages;
        const bool same_domain = nougatSameDomain;
        const auto state = nougatState;
        nougatCrawlWorker = std::thread([this, state, seed, max_pages, same_domain]() {
            std::string summary, error;
            const bool ok = nougat.crawl(seed, max_pages, same_domain,
                [this](const std::string& line) { append_nougat_crawl_log(line); }, summary, error);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->crawl_busy = false;
            state->status = ok ? summary : (error.empty() ? "Crawler failed." : "Crawler failed: " + error);
            state->updated = true;
        });
    }

    void refresh_nougat_peers() {
        std::string error;
        std::vector<std::string> values = nougat.peers(error);
        std::lock_guard<std::mutex> lock(nougatState->mutex);
        if (!error.empty()) nougatState->status = error;
        else nougatState->peers = std::move(values);
        nougatState->updated = true;
    }

    void add_nougat_peer() {
        std::string error;
        if (nougat.add_peer(nougatPeerEntry, error)) {
            nougatPeerEntry.clear();
            refresh_nougat_peers();
        } else {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->status = error;
            nougatState->updated = true;
        }
    }

    void remove_selected_nougat_peer() {
        std::string peer;
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            if (nougatPeerSelected < 0 || nougatPeerSelected >= (int)nougatState->peers.size()) return;
            peer = nougatState->peers[(size_t)nougatPeerSelected];
        }
        std::string error;
        if (nougat.remove_peer(peer, error)) {
            nougatPeerSelected = -1;
            refresh_nougat_peers();
        } else {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->status = error;
            nougatState->updated = true;
        }
    }

    void toggle_nougat_node() {
        if (nougat.node_running()) {
            nougat.stop_node();
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->status = "Nougat node stopped.";
            nougatState->updated = true;
            return;
        }
        std::string error;
        if (nougat.start_node(48731, error)) {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->status = "Nougat node listening on port 48731.";
            nougatState->updated = true;
        } else {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            nougatState->status = error;
            nougatState->updated = true;
        }
    }

    int nougat_log_line_at(int y) const {
        if (!nougatCrawlLogBox.contains(nougatCrawlLogBox.x+2, y)) return -1;
        const int local = (y - nougatCrawlLogBox.y - 8) / 18;
        if (local < 0) return -1;
        return nougatCrawlScroll + local;
    }

    void select_all_nougat_output() {
        std::size_t count = 0;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); count = nougatState->crawl_log.size(); }
        if (count == 0U) return;
        nougatOutputFocused = true;
        nougatOutputSelectionStart = 0;
        nougatOutputSelectionEnd = static_cast<int>(count) - 1;
        redraw();
    }

    std::string selected_nougat_output_text() {
        std::lock_guard<std::mutex> lock(nougatState->mutex);
        if (nougatOutputSelectionStart < 0 || nougatOutputSelectionEnd < 0 || nougatState->crawl_log.empty()) return {};
        int a = std::min(nougatOutputSelectionStart, nougatOutputSelectionEnd);
        int b = std::max(nougatOutputSelectionStart, nougatOutputSelectionEnd);
        a = std::max(0, a); b = std::min((int)nougatState->crawl_log.size()-1, b);
        std::ostringstream out;
        for (int i=a; i<=b; ++i) { if (i>a) out << '\n'; out << nougatState->crawl_log[(size_t)i]; }
        return out.str();
    }

    void copy_nougat_output_selection() {
        const std::string value = selected_nougat_output_text();
        if (value.empty()) return;
        own_clipboard_text(value);
        std::lock_guard<std::mutex> lock(nougatState->mutex);
        nougatState->status = "Crawler output copied.";
        nougatState->updated = true;
    }

    void show_nougat_output_context_menu(int x, int y) {
        const bool has_selection = !selected_nougat_output_text().empty();
        std::vector<MenuItem> items;
        items.push_back({"Copy", has_selection ? MenuAction::NougatCopySelection : MenuAction::NoAction, 0});
        items.push_back({"Select All", MenuAction::NougatSelectAll, 0});
        show_menu(win, x, y, items);
    }

    void handle_nougat_motion(const XMotionEvent& event) {
        if (currentView != ViewMode::Nougat || nougatPanel != NougatPanel::Crawler || !nougatOutputSelecting || event.window != win) return;
        int line_index = nougat_log_line_at(event.y);
        std::size_t count = 0;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); count = nougatState->crawl_log.size(); }
        if (line_index >= 0 && line_index < (int)count && line_index != nougatOutputSelectionEnd) {
            nougatOutputSelectionEnd = line_index;
            redraw();
        }
    }

    void paste_into_nougat_input() {
        if (nougatInputFocus == NougatInputFocus::NoFocus) return;
        std::string clip = clipboard_value();
        clip.erase(std::remove(clip.begin(), clip.end(), '\r'), clip.end());
        while (!clip.empty() && (clip.back()=='\n' || clip.back()=='\t')) clip.pop_back();
        if (clip.empty()) return;
        std::string& target = focused_nougat_text();
        if (nougatInputSelectAll) target.clear();
        target += clip;
        nougatInputSelectAll = false;
        redraw();
    }

    void handle_nougat_key(XKeyEvent& event, KeySym ks) {
        if (nougatOutputFocused && nougatPanel == NougatPanel::Crawler) {
            if ((event.state & ControlMask) && (ks == XK_a || ks == XK_A)) { select_all_nougat_output(); return; }
            if ((event.state & ControlMask) && (ks == XK_c || ks == XK_C)) { copy_nougat_output_selection(); return; }
            if (ks == XK_Escape) { nougatOutputFocused=false; nougatOutputSelectionStart=nougatOutputSelectionEnd=-1; redraw(); return; }
        }
        if (nougatInputFocus == NougatInputFocus::NoFocus) return;
        if (ks == XK_Escape) { nougatInputFocus=NougatInputFocus::NoFocus; nougatInputSelectAll=false; redraw(); return; }
        if ((event.state & ControlMask) && (ks == XK_a || ks == XK_A)) { nougatInputSelectAll=!focused_nougat_text().empty(); redraw(); return; }
        if ((event.state & ControlMask) && (ks == XK_v || ks == XK_V)) { paste_into_nougat_input(); return; }
        if ((event.state & ShiftMask) && ks == XK_Insert) { paste_into_nougat_input(); return; }
        if ((event.state & ControlMask) && (ks == XK_c || ks == XK_C) && nougatInputSelectAll) { own_clipboard_text(focused_nougat_text()); return; }
        if (ks == XK_BackSpace) {
            std::string& target = focused_nougat_text();
            if (nougatInputSelectAll) { target.clear(); nougatInputSelectAll=false; }
            else if (!target.empty()) target.pop_back();
            redraw(); return;
        }
        if (ks == XK_Return || ks == XK_KP_Enter) {
            if (nougatInputFocus == NougatInputFocus::Search) { nougatSearchOffset=0; start_nougat_search(); }
            else if (nougatInputFocus == NougatInputFocus::CrawlSeed) start_nougat_crawl();
            else if (nougatInputFocus == NougatInputFocus::Peer) add_nougat_peer();
            return;
        }
        char buf[64]; KeySym outks=0; int n=XLookupString(&event,buf,sizeof(buf)-1,&outks,nullptr);
        if (n>0) {
            std::string& target = focused_nougat_text();
            if (nougatInputSelectAll) { target.clear(); nougatInputSelectAll=false; }
            buf[n]=0; target.append(buf, static_cast<std::size_t>(n)); redraw();
        }
    }

    void handle_p2p_click(int x, int y) {
        if (p2pMagnetRect.contains(x,y)) {
            p2pMagnetFocused=true; p2pMagnetSelectAll=false;
            XSetInputFocus(d,win,RevertToParent,CurrentTime);
            p2pUiStatus="Magnet field ready. Ctrl+A selects all. Right-click opens Cut / Copy / Paste.";
            redraw(); return;
        }
        if (p2pOutputRect.contains(x,y)) {
            p2pMagnetFocused=false; p2pMagnetSelectAll=false;
            std::string folder=choose_p2p_folder_dialog();
            if (!folder.empty()) { p2pOutputFolder=folder; p2pUiStatus="Download folder set."; }
            redraw(); return;
        }
        if (p2pLoadMagnetBtn.contains(x,y)) { start_p2p_magnet(); return; }
        if (p2pOpenTorrentBtn.contains(x,y)) { open_p2p_torrent(); return; }
        if (p2pPlayBtn.contains(x,y)) { play_selected_p2p(); return; }
        if (p2pStopResumeBtn.contains(x,y)) { toggle_p2p_transfer(); return; }
        if (p2pRemoveBtn.contains(x,y)) { remove_p2p_transfer(); return; }
        if (p2pSpeedBtn.contains(x,y)) {
            p2pSpeedPreset=(p2pSpeedPreset+1)%4;
            int down=0,up=0;
            if (p2pSpeedPreset==1) { down=5120; up=1024; }
            else if (p2pSpeedPreset==2) { down=20480; up=5120; }
            else if (p2pSpeedPreset==3) { down=51200; up=10240; }
            std::string error;
            if (p2p.set_speed_limits(down,up,error)) p2pUiStatus=down==0?"P2P Plus speed limits disabled.":"P2P Plus speed limits updated.";
            else p2pUiStatus=error;
            redraw(); return;
        }
        if (p2pSeedRulesBtn.contains(x,y)) {
            p2pSeedPreset=(p2pSeedPreset+1)%4;
            double ratio=0.0; int minutes=0;
            if (p2pSeedPreset==1) ratio=1.0;
            else if (p2pSeedPreset==2) { ratio=2.0; minutes=120; }
            else if (p2pSeedPreset==3) minutes=240;
            std::string error;
            if (p2p.set_seed_rules(ratio,minutes,error)) p2pUiStatus=p2pSeedPreset==0?"P2P Plus seeding rule disabled.":"P2P Plus seeding rule updated.";
            else p2pUiStatus=error;
            redraw(); return;
        }
        if (p2pQueueUpBtn.contains(x,y)) { std::string error; if(p2p.queue_up(error)) p2pUiStatus="Moved transfer up in queue."; else p2pUiStatus=error; redraw(); return; }
        if (p2pQueueDownBtn.contains(x,y)) { std::string error; if(p2p.queue_down(error)) p2pUiStatus="Moved transfer down in queue."; else p2pUiStatus=error; redraw(); return; }
        if (p2pReannounceBtn.contains(x,y)) { std::string error; if(p2p.force_reannounce(error)) p2pUiStatus="Tracker reannounce requested."; else p2pUiStatus=error; redraw(); return; }
        if (p2pRecheckBtn.contains(x,y)) { std::string error; if(p2p.force_recheck(error)) p2pUiStatus="P2P data recheck requested."; else p2pUiStatus=error; redraw(); return; }
        if (p2pPriorityBtn.contains(x,y)) {
            const int selected=p2p.selected_file();
            if (selected<0) { p2pUiStatus="Select a P2P file before changing priority."; redraw(); return; }
            p2pPriorityPreset = p2pPriorityPreset==4 ? 6 : (p2pPriorityPreset==6 ? 0 : 4);
            std::string error;
            if (p2p.set_file_priority(selected,p2pPriorityPreset,error)) p2pUiStatus="Selected file priority updated."; else p2pUiStatus=error;
            redraw(); return;
        }
        for (std::size_t i=0;i<p2pFileRows.size();++i) {
            if (p2pFileRows[i].contains(x,y)) {
                std::vector<P2PFileInfo> fs=p2p.files();
                if (i<fs.size()) { std::string error; if (p2p.select_file(fs[i].index,error)) p2pUiStatus="Selected: "+fs[i].path; else p2pUiStatus=error; }
                redraw(); return;
            }
        }
        p2pMagnetFocused=false; p2pMagnetSelectAll=false; redraw(); return;
    }

    std::string security_auth_key_path() const {
        return home_dir() + "/.config/nougat-media-suite/security/abusech.key";
    }
    bool security_auth_key_configured() const {
        return exists_file(security_auth_key_path());
    }
    void save_security_auth_key(const std::string& key) {
        const std::string configBase = home_dir() + "/.config/nougat-media-suite";
        const std::string securityDir = configBase + "/security";
        mkdir((home_dir()+"/.config").c_str(),0755);
        mkdir(configBase.c_str(),0700);
        mkdir(securityDir.c_str(),0700);
        const std::string path = security_auth_key_path();
        if (key.empty()) {
            unlink(path.c_str());
            std::lock_guard<std::mutex> lock(securityState->mutex);
            securityState->status = "Threat intelligence key cleared. Local scanning remains available.";
            return;
        }
        std::ofstream out(path,std::ios::trunc);
        if (!out) {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            securityState->status = "Could not save the threat intelligence key.";
            return;
        }
        out << key << "\n";
        out.close();
        chmod(path.c_str(),0600);
        std::lock_guard<std::mutex> lock(securityState->mutex);
        securityState->status = "Free abuse.ch threat intelligence configured.";
    }
    std::string security_python() const {
        const std::string runtime = exe_dir()+"/components/security/runtime/venv/bin/python";
        return exists_file(runtime) ? runtime : std::string("python3");
    }
    void start_security_scan(const std::string& target, bool folder, const std::string& origin="Manual") {
        if (target.empty()) return;
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            if (securityState->busy) return;
        }
        if (securityWorker.joinable()) securityWorker.join();
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            securityState->busy = true;
            securityState->updated = false;
            securityState->folder = folder;
            securityState->target = target;
            securityState->verdict = "SCANNING";
            securityState->status = origin + " scan running. Scanner process exits when the result is ready.";
            securityState->report = "Scanning...";
        }
        securityScroll = 0;
        redraw();
        const std::shared_ptr<SecurityUiState> state = securityState;
        const std::string python = security_python();
        const std::string worker = exe_dir()+"/components/security/nougat_security_worker.py";
        const bool automatic = (origin == "Completed P2P download");
        securityWorker = std::thread([state,python,worker,target,folder,automatic]() {
            const std::string mode = folder ? " --folder " : (automatic ? " --auto-file " : " --file ");
            std::string command = shell_quote(python)+" "+shell_quote(worker)+mode+shell_quote(target)+" 2>&1";
            std::string report = run_command_capture(command);
            std::string verdict = "SCAN ERROR";
            const std::string prefix="VERDICT=";
            const size_t end=report.find('\n');
            const std::string first=report.substr(0,end==std::string::npos?report.size():end);
            if (first.rfind(prefix,0U)==0U) verdict=first.substr(prefix.size());
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=false;
            state->updated=true;
            state->verdict=verdict;
            state->report=report.empty()?"Scanner produced no report.":report;
            if (verdict=="THREAT DETECTED") state->status="WARNING: threat detected. Nougat changed nothing. Review the report and decide what to do.";
            else if (verdict=="SUSPICIOUS") state->status="WARNING: suspicious file characteristics detected. Nougat changed nothing.";
            else if (verdict=="NO THREATS DETECTED") state->status="Scan complete. No threats detected by every required/relevant check.";
            else if (verdict=="ANALYSIS INCOMPLETE") state->status="Analysis incomplete. Basic checks ran, but one or more required/relevant engines did not complete.";
            else if (verdict=="HISTORY") state->status="Recent scan history.";
            else state->status="Scan finished with an incomplete/error result.";
        });
    }
    void show_security_history() {
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            if (securityState->busy) return;
        }
        if (securityWorker.joinable()) securityWorker.join();
        const std::shared_ptr<SecurityUiState> state=securityState;
        const std::string python=security_python();
        const std::string worker=exe_dir()+"/components/security/nougat_security_worker.py";
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=true; state->updated=false; state->status="Loading recent scan history..."; state->verdict="WORKING";
        }
        securityWorker=std::thread([state,python,worker]() {
            const std::string report=run_command_capture(shell_quote(python)+" "+shell_quote(worker)+" --history 2>&1");
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=false; state->updated=true; state->verdict="HISTORY"; state->status="Recent scan history.";
            state->report=report.empty()?"No scan history yet.":report;
        });
    }
    void poll_security_worker() {
        bool updated=false;
        std::string verdict;
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            updated=securityState->updated;
            verdict=securityState->verdict;
            if (updated) securityState->updated=false;
        }
        if (!updated) return;
        if (securityWorker.joinable()) securityWorker.join();
        if (verdict=="THREAT DETECTED" || verdict=="SUSPICIOUS") {
            p2pUiStatus = "SECURITY WARNING: "+verdict+". See Search > Virus Scan. Nothing was quarantined.";
        }
        if (!pendingP2PAutoScanPaths.empty()) {
            const std::string next=pendingP2PAutoScanPaths.front();
            pendingP2PAutoScanPaths.erase(pendingP2PAutoScanPaths.begin());
            start_security_scan(next,false,"Completed P2P download");
            return;
        }
        if (!fullscreen && currentView==ViewMode::Nougat) redraw();
    }
    void maybe_auto_scan_completed_p2p() {
        P2PStatus st=p2p.status();
        if (!st.active || !st.seeding || st.save_path.empty()) return;
        const std::string signature=st.save_path+"|"+st.name+"|"+std::to_string(st.total);
        if (signature==lastP2PAutoScanTransfer) return;
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            if (securityState->busy) return;
        }
        std::vector<std::string> paths;
        for (const P2PFileInfo& f : p2p.files()) {
            if (f.path.empty()) continue;
            std::string path=st.save_path;
            if (!path.empty() && path.back()!='/') path+='/';
            path+=f.path;
            if (exists_file(path)) paths.push_back(path);
        }
        if (paths.empty()) return;
        lastP2PAutoScanTransfer=signature;
        pendingP2PAutoScanPaths=paths;
        const std::string first=pendingP2PAutoScanPaths.front();
        pendingP2PAutoScanPaths.erase(pendingP2PAutoScanPaths.begin());
        start_security_scan(first,false,"Completed P2P download");
    }
    void draw_security_screen(Drawable target) {
        const ViewPalette palette=palette_for(ViewMode::Nougat);
        const unsigned long ink=palette.text;
        text(target,28,94,"VIRUS SCAN",ink);
        button_on(target,securityScanFileBtn,"Scan File");
        button_on(target,securityScanFolderBtn,"Scan Folder");
        button_on(target,securityScanAgainBtn,"Scan Again");
        button_on(target,securityCommunityKeyBtn,security_auth_key_configured()?"Threat Intel ✓":"Threat Intel Key");
        button_on(target,securityHistoryBtn,"History");
        std::string status,verdict,report,targetPath;
        bool busy=false;
        {
            std::lock_guard<std::mutex> lock(securityState->mutex);
            status=securityState->status; verdict=securityState->verdict; report=securityState->report; targetPath=securityState->target; busy=securityState->busy;
        }
        const unsigned long warningInk=(verdict=="THREAT DETECTED")?rgb8(155,25,25):
            ((verdict=="SUSPICIOUS")?rgb8(135,78,20):((verdict=="ANALYSIS INCOMPLETE")?rgb8(151,112,42):ink));
        text(target,28,156,head_to_width(std::string("Status: ")+(busy?"Scanning - ":"")+status,W-56),warningInk);
        draw_nougat_panel(target,securityResultsBox);
        std::vector<std::string> lines;
        std::istringstream stream(report);
        std::string lineText;
        while(std::getline(stream,lineText)) lines.push_back(lineText);
        const int visible=std::max(1,(securityResultsBox.h-24)/18);
        securityScroll=std::max(0,std::min(securityScroll,std::max(0,(int)lines.size()-visible)));
        int y=securityResultsBox.y+22;
        for(int i=securityScroll;i<(int)lines.size() && i<securityScroll+visible;++i) {
            text(target,securityResultsBox.x+12,y,head_to_width(lines[(size_t)i],securityResultsBox.w-24),ink);
            y+=18;
        }
        if (!targetPath.empty()) text(target,securityResultsBox.x+12,securityResultsBox.y+securityResultsBox.h-8,tail_to_width("Last target: "+targetPath,securityResultsBox.w-24),palette.muted);
    }

    void handle_nougat_click(int x, int y) {
        if (nougatSearchPanelTab.contains(x,y)) {
            if (nougatPanel != NougatPanel::Search || nougatNetworkAdvanced) push_navigation_history();
            nougatPanel=NougatPanel::Search; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; nougatOutputFocused=false; p2pMagnetFocused=false;
            redraw(); return;
        }
        if (nougatCrawlerPanelTab.contains(x,y)) {
            if (nougatPanel != NougatPanel::Crawler || nougatNetworkAdvanced) push_navigation_history();
            nougatPanel=NougatPanel::Crawler; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; p2pMagnetFocused=false; redraw(); return;
        }
        if (nougatP2PPanelTab.contains(x,y)) {
            if (nougatPanel != NougatPanel::P2P || nougatNetworkAdvanced) push_navigation_history();
            nougatPanel=NougatPanel::P2P; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; nougatOutputFocused=false; redraw(); return;
        }
        if (nougatVirusScanPanelTab.contains(x,y)) {
            if (nougatPanel != NougatPanel::VirusScan || nougatNetworkAdvanced) push_navigation_history();
            nougatPanel=NougatPanel::VirusScan; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; nougatOutputFocused=false; p2pMagnetFocused=false; redraw(); return;
        }
        if (nougatPanel == NougatPanel::Search && nougatNetworkAdvancedBtn.contains(x,y)) {
            push_navigation_history();
            nougatNetworkAdvanced=!nougatNetworkAdvanced;
            nougatInputFocus=NougatInputFocus::NoFocus;
            if (nougatNetworkAdvanced) refresh_nougat_peers();
            redraw(); return;
        }
        if (nougatPanel == NougatPanel::P2P) { handle_p2p_click(x,y); return; }
        if (nougatPanel == NougatPanel::VirusScan) {
            if (securityScanFileBtn.contains(x,y)) { std::string p=choose_file_dialog(); if(!p.empty()) start_security_scan(p,false); return; }
            if (securityScanFolderBtn.contains(x,y)) { std::string p=choose_folder_dialog(); if(!p.empty()) start_security_scan(p,true); return; }
            if (securityScanAgainBtn.contains(x,y)) { std::string p; bool folder=false; { std::lock_guard<std::mutex> lock(securityState->mutex); p=securityState->target; folder=securityState->folder; } if(!p.empty()) start_security_scan(p,folder); return; }
            if (securityCommunityKeyBtn.contains(x,y)) { save_security_auth_key(choose_security_auth_key_dialog()); redraw(); return; }
            if (securityHistoryBtn.contains(x,y)) { show_security_history(); redraw(); return; }
            return;
        }
        if (nougatPanel == NougatPanel::Search && nougatNetworkAdvanced) {
            if (nougatPeerEntryRect.contains(x,y)) { focus_nougat_input(NougatInputFocus::Peer); return; }
            if (nougatAddPeerBtn.contains(x,y)) { add_nougat_peer(); return; }
            if (nougatRemovePeerBtn.contains(x,y)) { remove_selected_nougat_peer(); return; }
            if (nougatNodeBtn.contains(x,y)) { toggle_nougat_node(); redraw(); return; }
            if (nougatPeersToggleBtn.contains(x,y)) { nougatSearchPeers=!nougatSearchPeers; nougatSearchOffset=0; redraw(); return; }
            if (nougatPeerListBox.contains(x,y)) {
                const int local=(y-nougatPeerListBox.y-8)/22;
                const int index=nougatPeerScroll+local;
                std::size_t count=0; { std::lock_guard<std::mutex> lock(nougatState->mutex); count=nougatState->peers.size(); }
                if (index>=0 && index<(int)count) { nougatPeerSelected=index; redraw(); }
                return;
            }
            return;
        }
        if (nougatPanel == NougatPanel::Search) {
            if (nougatSearchRect.contains(x,y)) { focus_nougat_input(NougatInputFocus::Search); return; }
            if (nougatRawBtn.contains(x,y)) { nougatRaw=!nougatRaw; nougatSearchOffset=0; redraw(); return; }
            if (nougatSearchBtn.contains(x,y)) { nougatSearchOffset=0; start_nougat_search(); return; }
            for (const auto& hit : nougatResultHitboxes) {
                if (hit.index < 0) continue;
                reddmedia::NougatSearchResult result;
                { std::lock_guard<std::mutex> lock(nougatState->mutex); if (hit.index >= (int)nougatState->search.results.size()) continue; result=nougatState->search.results[(size_t)hit.index]; }
                std::string error;
                if (hit.open.contains(x,y)) { nougat.open_url(result.url,false,error); return; }
                if (hit.open_tor.contains(x,y)) {
                    if (result.url.rfind("magnet:?", 0U) == 0U) {
                        p2pMagnet = result.url;
                        nougatPanel = NougatPanel::P2P;
                        p2pUiStatus = "Magnet handed to P2P from Search.";
                        start_p2p_magnet();
                    } else nougat.open_url(result.url,true,error);
                    return;
                }
                if (hit.copy_url.contains(x,y)) { own_clipboard_text(result.url); return; }
            }
            return;
        }
        if (nougatPanel == NougatPanel::Crawler) {
            if (nougatCrawlSeedRect.contains(x,y)) { focus_nougat_input(NougatInputFocus::CrawlSeed); return; }
            if (nougatCrawlMinusBtn.contains(x,y)) { nougatMaxPages=std::max(1,nougatMaxPages-1); redraw(); return; }
            if (nougatCrawlPlusBtn.contains(x,y)) { nougatMaxPages=std::min(10000,nougatMaxPages+1); redraw(); return; }
            if (nougatSameDomainBtn.contains(x,y)) { nougatSameDomain=!nougatSameDomain; redraw(); return; }
            if (nougatStartCrawlBtn.contains(x,y)) { start_nougat_crawl(); return; }
            if (nougatCrawlLogBox.contains(x,y)) {
                nougatOutputFocused=true; nougatInputFocus=NougatInputFocus::NoFocus;
                int line_index=nougat_log_line_at(y);
                std::size_t count=0; { std::lock_guard<std::mutex> lock(nougatState->mutex); count=nougatState->crawl_log.size(); }
                if (line_index>=0 && line_index<(int)count) {
                    nougatOutputSelectionStart=line_index; nougatOutputSelectionEnd=line_index; nougatOutputSelecting=true; redraw();
                }
                return;
            }
        }
    }

    void draw_nougat_screen(Drawable target) {
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Nougat);
        const ViewPalette searchPalette = palette_for(ViewMode::Nougat);
        std::string node;
        std::string status;
        bool search_busy=false, crawl_busy=false;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); node=nougatState->node_id; status=nougatState->status; search_busy=nougatState->search_busy; crawl_busy=nougatState->crawl_busy; }
        nougat_tab_button(target,nougatSearchPanelTab,"Search",nougatPanel==NougatPanel::Search && !nougatNetworkAdvanced);
        nougat_tab_button(target,nougatCrawlerPanelTab,"Crawler",nougatPanel==NougatPanel::Crawler);
        nougat_tab_button(target,nougatP2PPanelTab,"P2P",nougatPanel==NougatPanel::P2P);
        nougat_tab_button(target,nougatVirusScanPanelTab,"Virus Scan",nougatPanel==NougatPanel::VirusScan);
        if (nougatPanel == NougatPanel::Search) {
            nougat_button(target,nougatNetworkAdvancedBtn,nougatNetworkAdvanced?"Back":"Network...",nougatNetworkAdvanced);
        }

        if (nougatPanel == NougatPanel::P2P) {
            draw_p2p_screen(target);
            return;
        }
        if (nougatPanel == NougatPanel::VirusScan) {
            draw_security_screen(target);
            return;
        }
        if (nougatPanel == NougatPanel::Search && nougatNetworkAdvanced) {
            text(target,28,84,"NETWORK / ADVANCED",nougat_cream());
            if (!node.empty()) text(target,28,101,"Node ID: "+node,searchPalette.muted);
            draw_nougat_input(target,nougatPeerEntryRect,nougatPeerEntry,NougatInputFocus::Peer);
            nougat_button(target,nougatAddPeerBtn,"Add Peer");
            nougat_button(target,nougatRemovePeerBtn,"Remove");
            nougat_button(target,nougatNodeBtn,nougat.node_running()?"STOP NODE":"START NODE",nougat.node_running());
            nougat_button(target,nougatPeersToggleBtn,"Search peers",nougatSearchPeers);
            draw_nougat_panel(target,nougatPeerListBox);
            std::vector<std::string> peers;
            { std::lock_guard<std::mutex> lock(nougatState->mutex); peers=nougatState->peers; }
            const int visible=std::max(1,((int)nougatPeerListBox.h-16)/22);
            nougatPeerScroll=std::max(0,std::min(nougatPeerScroll,std::max(0,(int)peers.size()-visible)));
            int y=nougatPeerListBox.y+22;
            for (int i=nougatPeerScroll; i<(int)peers.size() && i<nougatPeerScroll+visible; ++i) {
                if (i==nougatPeerSelected) fill_round(target,{nougatPeerListBox.x+6,y-16,(int)nougatPeerListBox.w-12,21},5,searchPalette.selection);
                text(target,nougatPeerListBox.x+9,y,peers[(size_t)i],searchPalette.text); y+=22;
            }
            text(target,28,H-20,status,searchPalette.text);
            return;
        }
        if (nougatPanel == NougatPanel::Search) {
            draw_nougat_input(target,nougatSearchRect,nougatSearchQuery,NougatInputFocus::Search);
            nougat_button(target,nougatRawBtn,"RAW",nougatRaw);
            nougat_button(target,nougatSearchBtn,search_busy?"SEARCHING":"SEARCH");
            text(target,28,160,status,searchPalette.text);
            draw_nougat_panel(target,nougatResultsBox);
            nougatResultHitboxes.clear();
            std::vector<reddmedia::NougatSearchResult> results;
            { std::lock_guard<std::mutex> lock(nougatState->mutex); results=nougatState->search.results; }
            const int card_h=98;
            const int visible=std::max(1,((int)nougatResultsBox.h-12)/card_h);
            nougatResultScroll=std::max(0,std::min(nougatResultScroll,std::max(0,(int)results.size()-visible)));
            int y=nougatResultsBox.y+8;
            for (int i=nougatResultScroll; i<(int)results.size() && i<nougatResultScroll+visible; ++i) {
                const auto& r=results[(size_t)i];
                Rect card={nougatResultsBox.x+8,y,(int)nougatResultsBox.w-16,card_h-6};
                fill_round(target,card,7,rgb8(250,240,222)); outline_round(target,card,7,searchPalette.border);
                Rect cardInset{card.x+2,card.y+2,card.w-4,card.h-4};
                outline_round(target,cardInset,5,rgb8(255,248,235));
                text(target,card.x+8,card.y+18,head_to_width(std::to_string(nougatSearchOffset+i+1)+". "+r.title,card.w-170),searchPalette.text);
                text(target,card.x+8,card.y+38,head_to_width(r.url,card.w-16),rgb8(151,91,36));
                text(target,card.x+8,card.y+58,head_to_width(r.snippet,card.w-16),searchPalette.muted);
                text(target,card.x+8,card.y+78,head_to_width(r.source_network+" | Node "+r.source_node,card.w-300),searchPalette.muted);
                Rect copy={card.x+card.w-278,card.y+63,82,24};
                Rect tor={card.x+card.w-190,card.y+63,94,24};
                Rect open={card.x+card.w-90,card.y+63,82,24};
                const bool magnetResult = r.url.rfind("magnet:?", 0U) == 0U;
                nougat_button(target,copy,"Copy URL");
                nougat_button(target,tor,magnetResult?"Open P2P":"Open Tor");
                nougat_button(target,open,"Open");
                nougatResultHitboxes.push_back({card,open,tor,copy,i});
                y+=card_h;
            }
            return;
        }
        text(target,28,102,"Seed URL",nougat_cream());
        draw_nougat_input(target,nougatCrawlSeedRect,nougatCrawlSeed,NougatInputFocus::CrawlSeed);
        nougat_button(target,nougatCrawlMinusBtn,"-"); nougat_button(target,nougatCrawlPlusBtn,"+");
        text(target,nougatCrawlMinusBtn.x-96,130,"Max pages: "+std::to_string(nougatMaxPages),nougat_cream());
        nougat_button(target,nougatSameDomainBtn,"Stay on domain",nougatSameDomain);
        nougat_button(target,nougatStartCrawlBtn,crawl_busy?"CRAWLING":"START CRAWL");
        draw_nougat_panel(target,nougatCrawlLogBox);
        std::vector<std::string> lines;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); lines=nougatState->crawl_log; }
        const int visible=std::max(1,((int)nougatCrawlLogBox.h-16)/18);
        const int max_scroll=std::max(0,(int)lines.size()-visible);
        nougatCrawlScroll=std::max(0,std::min(nougatCrawlScroll,max_scroll));
        if (crawl_busy && max_scroll>0) nougatCrawlScroll=max_scroll;
        int y=nougatCrawlLogBox.y+20;
        for (int i=nougatCrawlScroll; i<(int)lines.size() && i<nougatCrawlScroll+visible; ++i) {
            const bool selected=nougatOutputSelectionStart>=0 && i>=std::min(nougatOutputSelectionStart,nougatOutputSelectionEnd) && i<=std::max(nougatOutputSelectionStart,nougatOutputSelectionEnd);
            if (selected) fill_round(target,{nougatCrawlLogBox.x+6,y-14,(int)nougatCrawlLogBox.w-12,18},4,searchPalette.selection);
            text(target,nougatCrawlLogBox.x+9,y,head_to_width(lines[(size_t)i],nougatCrawlLogBox.w-18),searchPalette.text);
            y+=18;
        }
        text(target,28,174,status,searchPalette.text);
    }

    void poll_nougat_workers() {
        bool needs_redraw=false;
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            if (nougatState->updated) { nougatState->updated=false; needs_redraw=true; }
        }
        if (needs_redraw && !fullscreen && currentView==ViewMode::Nougat) redraw();
    }

    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Studio);
        section_text(target, 28, 70, "GOLD STUDIO", palette.text);
        text(target, 28, 96, "Nougat media-processing workspace foundation.", palette.muted);
        Rect panel{28, 118, std::max(240, W - 56), std::max(150, H - 148)};
        draw_primary_panel(target, panel, palette);
        text(target, panel.x + 16, panel.y + 30, "Planned processing engine: FFmpeg/libav-backed Convert, Audio Lab, Quick Edit, Batch, and full timeline Studio.", palette.text);
        text(target, panel.x + 16, panel.y + 56, "v0.0.37 keeps the Gold Studio navigation/palette foundation; processing tools remain roadmap work.", palette.muted);
    }

    void draw_debug_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Debug);
        const unsigned long dark = palette.text;
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Debug);
        button_on(target, serverStartBtn, "Start Server");
        button_on(target, serverStopBtn, "Stop Server");
        button_on(target, serverRefreshBtn, "Refresh Server");
        button_on(target, debugRunBtn, "Run Checks");
        button_on(target, debugRetryBtn, "Retry");
        button_on(target, debugMetadataBtn, "Refresh Metadata");
        button_on(target, debugTmdbBtn, "Test TMDb");
        button_on(target, debugLogsBtn, "Open Logs");
        button_on(target, debugCopyBtn, "Copy Report");
        button_on(target, debugExportTextBtn, "Export TXT");
        button_on(target, debugExportJsonBtn, "Export JSON");
        button_on(target, debugBundleBtn, "Support Bundle");

        reddmedia::DiagnosticReport report;
        std::string status;
        bool has_report = false;
        bool busy = false;
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            report = debugState->report;
            status = debugState->status;
            has_report = debugState->hasReport;
            busy = debugState->busy;
        }
        text(target, 28, 116, head_to_width(
            std::string("Status: ") + (busy ? "Working - " : "") + status, W - 56), dark);
        draw_primary_panel(target, debugListBox, palette);
        debugIssueRows.clear();
        if (!has_report) {
            text(target, debugListBox.x + 14, debugListBox.y + 30,
                 "Run Checks to inspect Nougat, server, library, playback, Search, P2P, AI, Stream, and system evidence.",
                 palette.muted);
            return;
        }
        unsigned long health_color = col(0x1111,0xdddd,0x2222);
        if (report.overall == reddmedia::DiagnosticSeverity::Warning) {
            health_color = col(0xeeee,0xcccc,0x1111);
        } else if (report.overall == reddmedia::DiagnosticSeverity::Critical) {
            health_color = col(0xdddd,0x1111,0x1111);
        }
        fill_circle(target, debugListBox.x + 14, debugListBox.y + 12, 12, health_color);
        text(target, debugListBox.x + 34, debugListBox.y + 23,
             std::string("Overall: ") + reddmedia::DiagnosticEngine::severity_name(report.overall) +
             "   Checked: " + local_time_text(report.checked_at) +
             "   Port 8096: " + (report.port_8096_open ? "open" : "closed"), dark);
        const int issue_height = 88;
        const int visible = std::max(1, (debugListBox.h - 46) / issue_height);
        const int max_scroll = std::max(0, static_cast<int>(report.issues.size()) - visible);
        debugScroll = std::max(0, std::min(debugScroll, max_scroll));
        int y = debugListBox.y + 38;
        for (int visible_index = 0; visible_index < visible; ++visible_index) {
            const int issue_index = debugScroll + visible_index;
            if (issue_index >= static_cast<int>(report.issues.size())) break;
            const reddmedia::DiagnosticIssue& issue = report.issues[static_cast<std::size_t>(issue_index)];
            Rect row = {debugListBox.x + 8, y, debugListBox.w - 16, issue_height - 6};
            outline(target, row, palette.border);
            unsigned long issue_color = col(0x1166,0x7777,0x2222);
            if (issue.severity == reddmedia::DiagnosticSeverity::Warning) {
                issue_color = col(0x9999,0x7777,0x0000);
            } else if (issue.severity == reddmedia::DiagnosticSeverity::Critical) {
                issue_color = col(0xaaaa,0x0000,0x0000);
            }
            text(target, row.x + 8, row.y + 17,
                 std::string("[") + reddmedia::DiagnosticEngine::severity_name(issue.severity) +
                 "] " + head_to_width(issue.title, row.w - 90), issue_color);
            text(target, row.x + 8, row.y + 37,
                 head_to_width(issue.detail, row.w - 16), dark);
            text(target, row.x + 8, row.y + 57,
                 "Action: " + head_to_width(issue.action, row.w - 72),
                 palette.muted);
            text(target, row.x + 8, row.y + 75, issue.code, palette.muted);
            debugIssueRows.push_back(row);
            y += issue_height;
        }
    }

    bool discover_mode_selected(reddmedia::RecommendationMode mode) const {
        return discoverMode == mode;
    }

    bool discover_target_selected(reddmedia::RecommendationSource source,
                                  reddmedia::RecommendationMediaType media_type) const {
        return discoverTargetSelected && discoverSource == source && discoverMediaType == media_type;
    }

    void draw_discover_selector(Drawable target, const Rect& r, const std::string& label,
                                bool active) {
        const ViewPalette palette = palette_for(ViewMode::Discover);
        const bool hover = r.contains(pointerWindowX, pointerWindowY);
        draw_sheet_tab_surface(target, r, palette, active, hover);
        const Rect visual{r.x+2,r.y+1,std::max(1,r.w-4),std::max(1,r.h-4)};
        text(target,visual.x+std::max(5,(visual.w-text_width(label))/2),
             visual.y+visual.h/2+5,label,palette.buttonText);
    }

    bool resolve_discover_local_play_target(const reddmedia::RecommendationResult& result,
                                            reddmedia::LibraryNode& playable,
                                            std::string& error) {
        std::vector<reddmedia::LibraryNode> roots;
        if (!libraryClient->load_all_recommendation_items(roots,error)) return false;
        auto match = std::find_if(roots.begin(),roots.end(),[&result](const auto& node) {
            return node.id == result.item.id;
        });
        if (match == roots.end()) {
            error = "That Local recommendation is no longer in the Jellyfin catalog. Refresh Library.";
            return false;
        }
        const reddmedia::LibraryNode root = *match;
        if (root.kind == reddmedia::LibraryNodeKind::Movie) {
            if (root.path.empty() || !exists_file(root.path)) {
                error = "That Local movie file is unavailable. Refresh Library.";
                return false;
            }
            playable = root;
            return true;
        }
        if (root.kind != reddmedia::LibraryNodeKind::Series) {
            error = "That Local TV recommendation did not resolve to a series.";
            return false;
        }

        std::vector<reddmedia::LibraryNode> seasons;
        if (!libraryClient->load_library_children(root,seasons,error)) return false;
        std::sort(seasons.begin(),seasons.end(),[](const auto& a,const auto& b) {
            if (a.season_number != b.season_number) return a.season_number < b.season_number;
            return a.name < b.name;
        });
        std::vector<reddmedia::LibraryNode> episodes;
        for (const auto& season : seasons) {
            if (season.kind != reddmedia::LibraryNodeKind::Season) continue;
            std::vector<reddmedia::LibraryNode> children;
            std::string child_error;
            if (!libraryClient->load_library_children(season,children,child_error)) continue;
            std::sort(children.begin(),children.end(),[](const auto& a,const auto& b) {
                if (a.season_number != b.season_number) return a.season_number < b.season_number;
                if (a.episode_number != b.episode_number) return a.episode_number < b.episode_number;
                return a.name < b.name;
            });
            for (auto& episode : children) {
                if (episode.kind == reddmedia::LibraryNodeKind::Episode &&
                    !episode.path.empty() && exists_file(episode.path)) {
                    episodes.push_back(std::move(episode));
                }
            }
        }
        if (episodes.empty()) {
            error = "That Local TV series has no playable episode in the Jellyfin catalog.";
            return false;
        }

        // At show level, resume from the most recently watched episode when
        // history can identify one inside this series folder. Otherwise start
        // with the first real episode.
        std::vector<reddmedia::ViewingRecord> history;
        std::string history_error;
        if (recommendationEngine->recent_history(reddmedia::RecommendationMediaType::Television,
                                                 history,history_error,200)) {
            for (const auto& record : history) {
                if (record.item.local_path.empty()) continue;
                auto watched = std::find_if(episodes.begin(),episodes.end(),[&record](const auto& episode) {
                    return (!record.item.id.empty() && episode.id == record.item.id) ||
                           episode.path == record.item.local_path;
                });
                if (watched != episodes.end()) {
                    playable = *watched;
                    libraryParents.clear();
                    libraryParents.push_back(root);
                    return true;
                }
            }
        }
        playable = episodes.front();
        libraryParents.clear();
        libraryParents.push_back(root);
        return true;
    }

    void open_discover_result() {
        reddmedia::RecommendationResult result;
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (!discoverState->hasResult) return;
            result = discoverState->result;
        }
        if (result.item.local_path.empty()) {
            if (result.item.media_type == reddmedia::RecommendationMediaType::Television &&
                result.item.id.find("tmdb:") != 0U) {
                libraryMediaType = reddmedia::LibraryMediaType::Television;
                libraryTypeChosen = true;
                libraryParents.clear();
                reddmedia::LibraryNode series;
                series.id = result.item.id;
                series.name = result.item.title;
                series.kind = reddmedia::LibraryNodeKind::Series;
                libraryParents.push_back(series);
                switch_view(ViewMode::Library);
                start_library_task(5, {}, series);
                return;
            }
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->status = "This is a TMDb recommendation and is not on your server.";
            }
            redraw();
            return;
        }
        reddmedia::LibraryNode playable;
        std::string resolve_error;
        if (!resolve_discover_local_play_target(result, playable, resolve_error)) {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->status = resolve_error;
            redraw();
            return;
        }
        if (playable.kind == reddmedia::LibraryNodeKind::Episode) prepare_tv_autoplay(playable);
        else cancel_tv_autoplay();
        std::string history_error;
        recommendationEngine->record_started(descriptor_for_node(playable), history_error);
        if (!open_media(playable.path, 0)) {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            discoverState->status = "Nougat could not start that Local media file in the native player.";
            redraw();
            return;
        }
        switch_view(ViewMode::VideoPlayer);
    }

    void draw_discover_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Discover);
        const unsigned long dark = palette.text;
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Discover);
        if (discoverServiceSettings) {
            button_on(target, discoverServicesBackBtn, "Back to Discover");
            text(target, 160, kPageControlBottom + 20, "MY STREAMING SERVICES - UNITED STATES", dark);
            text(target, 224, 97,
                 "Select services you use. Availability still shows every verified listing.",
                 palette.muted);
            const Rect services_box = {28, 120, std::max(240, W - 56), std::max(150, H - 148)};
            draw_primary_panel(target, services_box, palette);
            std::vector<reddmedia::WatchProvider> providers;
            std::string status;
            bool busy = false;
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                providers = discoverState->providerCatalog;
                status = discoverState->status;
                busy = discoverState->busy;
            }
            discoverProviderRows.clear();
            if (providers.empty()) {
                text(target, services_box.x + 12, services_box.y + 28,
                     busy ? "Loading watch services..." : head_to_width(status, services_box.w - 24),
                     palette.muted);
                return;
            }
            const int row_height = 28;
            const int visible = std::max(1, (services_box.h - 12) / row_height);
            const int max_scroll = std::max(0, static_cast<int>(providers.size()) - visible);
            discoverServicesScroll = std::max(0, std::min(discoverServicesScroll, max_scroll));
            int row_y = services_box.y + 6;
            for (int visible_index = 0; visible_index < visible; ++visible_index) {
                const int provider_index = discoverServicesScroll + visible_index;
                if (provider_index >= static_cast<int>(providers.size())) break;
                const reddmedia::WatchProvider& provider =
                    providers[static_cast<std::size_t>(provider_index)];
                const Rect row = {services_box.x + 6, row_y, services_box.w - 12, row_height - 2};
                if (watchPreferences.is_selected(provider.id)) {
                    fill_round(target, row, 6, palette.selection);
                }
                outline_round(target, row, 6, palette.border);
                const bool selected = watchPreferences.is_selected(provider.id);
                const Rect check{row.x + 5, row.y + 3, 22, 22};
                draw_sheet_checkbox(target, check, selected, palette);
                text(target, row.x + 34, row.y + 18,
                     head_to_width(provider.name, row.w - 42), dark);
                discoverProviderRows.push_back({row, provider.id});
                row_y += row_height;
            }
            return;
        }
        draw_discover_selector(target, discoverUsualTab, "Usual",
                               discover_mode_selected(reddmedia::RecommendationMode::Usual));
        draw_discover_selector(target, discoverRandomTab, "Random",
                               discover_mode_selected(reddmedia::RecommendationMode::Random));
        draw_discover_selector(target, discoverLocalMovieBtn, "Local Movie",
                               discover_target_selected(reddmedia::RecommendationSource::Local,
                                                        reddmedia::RecommendationMediaType::Movie));
        draw_discover_selector(target, discoverLocalTvBtn, "Local TV",
                               discover_target_selected(reddmedia::RecommendationSource::Local,
                                                        reddmedia::RecommendationMediaType::Television));
        draw_discover_selector(target, discoverLiveTvBtn, "Live TV",
                               discoverTargetSelected && discoverSource == reddmedia::RecommendationSource::LiveTV);
        draw_discover_selector(target, discoverExternalMovieBtn, "TMDb Movie",
                               discover_target_selected(reddmedia::RecommendationSource::External,
                                                        reddmedia::RecommendationMediaType::Movie));
        draw_discover_selector(target, discoverExternalTvBtn, "TMDb TV",
                               discover_target_selected(reddmedia::RecommendationSource::External,
                                                        reddmedia::RecommendationMediaType::Television));
        button_on(target, discoverTmdbTestBtn, "Test TMDb");
        button_on(target, discoverTmdbReplaceBtn, "Save / Replace");
        button_on(target, discoverTmdbClearBtn, "Clear TMDb");
        button_on(target, discoverMyServicesBtn, "My Services");
        text(target, 28, 102, head_to_width(recommendationEngine->external_credential_label(), W - 56), dark);

        reddmedia::RecommendationResult result;
        std::string status;
        bool has_result = false;
        bool busy = false;
        bool has_poster = false;
        reddmedia::LibraryPoster poster;
        reddmedia::WatchAvailability availability;
        bool has_availability = false;
        std::string availability_status;
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            result = discoverState->result;
            status = discoverState->status;
            has_result = discoverState->hasResult;
            busy = discoverState->busy;
            has_poster = discoverState->hasPoster;
            poster = discoverState->poster;
            availability = discoverState->availability;
            has_availability = discoverState->hasAvailability;
            availability_status = discoverState->availabilityStatus;
        }
        text(target, 28, 218, std::string("Status: ") + (busy ? "Working - " : "") + status, dark);
        draw_primary_panel(target, discoverResultBox, palette);
        text(target, 28, H - 12,
             "Watch availability by JustWatch via TMDb. This product uses the TMDB API but is not endorsed or certified by TMDB.",
             palette.muted);
        if (!has_result) {
            text(target, discoverResultBox.x + 14, discoverResultBox.y + 30,
                 "No recommendation selected.", palette.muted);
            return;
        }
        const Rect poster_area = {discoverResultBox.x + 16, discoverResultBox.y + 16,
                                  180, std::min(260, discoverResultBox.h - 32)};
        fill(target, poster_area, col(0x0808,0x0808,0x0808));
        if (has_poster) draw_poster_pixels(target, poster_area, poster);
        else text(target, poster_area.x + 48, poster_area.y + poster_area.h / 2,
                  "NO POSTER", col(0x9999,0x9999,0x9999));
        const int details_x = poster_area.x + poster_area.w + 20;
        const int details_width = discoverResultBox.x + discoverResultBox.w - details_x - 16;
        std::string title = result.item.title;
        if (result.item.year > 0) title += " (" + std::to_string(result.item.year) + ")";
        text(target, details_x, discoverResultBox.y + 32,
             head_to_width(title, details_width), dark);
        text(target, details_x, discoverResultBox.y + 58,
             result.item.local_path.empty() ? "TMDb" : "Local", col(0xc7c7,0x9f9f,0xd2d2));
        text(target, details_x, discoverResultBox.y + 84,
             head_to_width(result.reason, details_width), dark);

        std::vector<std::string> detail_lines;
        detail_lines.push_back("Description");
        const std::vector<std::string> overview_lines = wrap_text(
            result.item.overview.empty() ? "Description unavailable." : result.item.overview,
            details_width);
        detail_lines.insert(detail_lines.end(), overview_lines.begin(), overview_lines.end());
        if (result.item.local_path.empty()) {
            detail_lines.push_back("");
            detail_lines.push_back("Where to Watch - United States");
            if (!availability_status.empty()) detail_lines.push_back(availability_status);
            if (!has_availability || !availability.listing_found) {
                detail_lines.push_back("No current United States provider listing was returned.");
            } else if (availability.providers.empty()) {
                detail_lines.push_back("A listing exists, but no streaming, free, rent, or buy providers were returned.");
            } else {
                reddmedia::WatchProviderCategory previous =
                    static_cast<reddmedia::WatchProviderCategory>(-1);
                for (const reddmedia::WatchProvider& provider : availability.providers) {
                    if (provider.category != previous) {
                        detail_lines.push_back(std::string("-- ") +
                            reddmedia::watch_provider_category_name(provider.category) + " --");
                        previous = provider.category;
                    }
                    detail_lines.push_back(
                        std::string(watchPreferences.is_selected(provider.id) ? "[MY] " : "") +
                        provider.name);
                }
            }
            if (has_availability && availability.refreshed_at > 0) {
                detail_lines.push_back("Availability refreshed: " +
                                       local_time_text(availability.refreshed_at));
            }
            detail_lines.push_back("Provider data: JustWatch via TMDb.");
        }
        const int content_y = discoverResultBox.y + 108;
        const int line_height = 18;
        const int visible_lines = std::max(1,
            (discoverResultBox.y + discoverResultBox.h - content_y - 10) / line_height);
        const int max_scroll = std::max(0, static_cast<int>(detail_lines.size()) - visible_lines);
        discoverDetailsScroll = std::max(0, std::min(discoverDetailsScroll, max_scroll));
        int line_y = content_y;
        for (int index = discoverDetailsScroll;
             index < static_cast<int>(detail_lines.size()) &&
             line_y < discoverResultBox.y + discoverResultBox.h - 8;
             ++index) {
            const std::string& detail = detail_lines[static_cast<std::size_t>(index)];
            unsigned long color = palette.muted;
            if (detail == "Description" || detail == "Where to Watch - United States" ||
                detail.rfind("-- ", 0U) == 0U || detail.rfind("[MY] ", 0U) == 0U) {
                color = col(0xd2d2,0xa8a8,0xd9d9);
            }
            text(target, details_x, line_y, head_to_width(detail, details_width), color);
            line_y += line_height;
        }
        if (result.item.id.find("tmdb:") != 0U) {
            button_on(target, discoverOpenBtn,
                      result.item.local_path.empty() ? "Open in Library" : "Play in Nougat Media Suite");
        }
        if (result.item.local_path.empty() && has_availability &&
            availability.link.rfind("https://www.themoviedb.org/", 0U) == 0U) {
            button_on(target, discoverWatchBtn, "Open Watch Options");
        }
    }

    void start_p2p_magnet() {
        std::string error;
        if (p2p.start_magnet(p2pMagnet,p2pOutputFolder,error)) p2pUiStatus="Magnet added. Getting metadata...";
        else p2pUiStatus=error;
        p2pMagnetFocused=false; p2pMagnetSelectAll=false; redraw();
    }
    void open_p2p_torrent() {
        std::string path=choose_torrent_file_dialog();
        if (path.empty()) return;
        std::string error;
        if (p2p.start_torrent_file(path,p2pOutputFolder,error)) p2pUiStatus="P2P metadata file loaded.";
        else p2pUiStatus=error;
        redraw();
    }
    void play_selected_p2p() {
        auto_select_single_video();
        if (p2p.selected_file()<0) { p2pUiStatus="Select a video file first."; redraw(); return; }
        std::string error;
        p2p.prioritize_range(0,std::min<std::uint64_t>(16ULL*1024ULL*1024ULL,p2p.selected_file_size()));
        if (!p2pStream.start(error)) { p2pUiStatus=error; redraw(); return; }
        p2pUiStatus="Streaming. Nougat Media Suite will prioritize pieces around playback and seeks.";
        switch_view(ViewMode::VideoPlayer);
        if (!open_p2p_stream_location(p2pStream.url())) {
            p2pStream.stop();
            p2pUiStatus="VLC could not open the local P2P stream.";
            nougatPanel = NougatPanel::P2P;
            switch_view(ViewMode::Nougat);
        }
    }
    void toggle_p2p_transfer() {
        std::string error;
        if (p2p.is_paused()) {
            if (p2p.resume_transfer(error)) p2pUiStatus="P2P download resumed.";
            else p2pUiStatus=error;
        } else {
            if (currentMediaIsP2P) { p2pStream.stop(); cleanup_player(); currentMediaIsP2P=false; currentMediaIsNetwork=false; }
            if (p2p.pause_transfer(error)) p2pUiStatus="P2P download paused. Partial data and resume state preserved.";
            else p2pUiStatus=error;
        }
        redraw();
    }

    void remove_p2p_transfer() {
        if (currentMediaIsP2P) {
            p2pStream.stop();
            cleanup_player();
            currentMediaIsP2P=false;
            currentMediaIsNetwork=false;
        }
        std::string error;
        if (p2p.remove_transfer(error)) p2pUiStatus="P2P transfer removed. Downloaded files were left on disk.";
        else p2pUiStatus=error;
        redraw();
    }

    void redraw() {
        if (fullscreen) {
            draw_video_message();
            if (contextMenuOpen) draw_context_menu();
            XFlush(d);
            return;
        }
        Pixmap buffer = XCreatePixmap(d, win, W, H, DefaultDepth(d, screen));
        fill(buffer, {0,0,W,H}, col(0xdede,0xdede,0xdede));
        draw_controls(buffer);
        if (currentView != ViewMode::VideoPlayer) apply_page_clip(currentView);
        if (currentView == ViewMode::Home) draw_home_screen(buffer);
        if (currentView == ViewMode::Library) draw_library_screen(buffer);
        if (currentView == ViewMode::Discover) draw_discover_screen(buffer);
        if (currentView == ViewMode::LiveTV) draw_live_tv_screen(buffer);
        if (currentView == ViewMode::Nougat) draw_nougat_screen(buffer);
        if (currentView == ViewMode::Debug) draw_debug_screen(buffer);
        if (currentView == ViewMode::Stream) draw_stream_screen(buffer);
        if (currentView == ViewMode::Studio) draw_studio_screen(buffer);
        XSetClipMask(d,gc,None);
        if (currentView != ViewMode::VideoPlayer) draw_page_frame(buffer,currentView);
        draw_loading_bar(buffer);
        // Final chrome overlay: page backgrounds and loading strips must never
        // erase the larger selected-tab pointer.
        draw_active_top_tab_pointer(buffer);
        XCopyArea(d, buffer, win, gc, 0, 0, W, H, 0, 0);
        XFreePixmap(d, buffer);
        if (currentView == ViewMode::VideoPlayer) draw_video_message();
        if (contextMenuOpen) draw_context_menu();
        XFlush(d);
    }

    std::string ytdlp_engine_path() {
        return exe_dir() + "/tools/yt-dlp/yt-dlp";
    }
    void append_ytdlp_log(const std::string& s) {
        if (s.empty()) return;
        if (ytdlpLog == "No download output yet.") ytdlpLog.clear();
        ytdlpLog += s;
        if (ytdlpLog.size() > 24000) ytdlpLog = ytdlpLog.substr(ytdlpLog.size() - 24000);
    }
    void poll_ytdlp_process() {
        if (ytdlpPipe >= 0) {
            char buf[4096];
            for (;;) {
                ssize_t n = read(ytdlpPipe, buf, sizeof(buf)-1);
                if (n > 0) { buf[n]=0; append_ytdlp_log(buf); ytdlpProcessOutput.append(buf, static_cast<std::size_t>(n)); }
                else break;
            }
        }
        if (ytdlpPid > 0) {
            int status=0;
            pid_t r = waitpid(ytdlpPid, &status, WNOHANG);
            if (r == ytdlpPid) {
                if (ytdlpPipe >= 0) {
                    char tail[4096];
                    for (;;) { ssize_t n=read(ytdlpPipe,tail,sizeof(tail)-1); if(n>0){tail[n]=0;append_ytdlp_log(tail);ytdlpProcessOutput.append(tail,static_cast<std::size_t>(n));} else break; }
                    close(ytdlpPipe); ytdlpPipe=-1;
                }
                const bool ok = WIFEXITED(status) && WEXITSTATUS(status)==0;
                ytdlpPid = -1; ytdlpJob = YtDlpJob::Idle;
                ytdlpStatus = ok ? "Download complete." : "Download failed. See log.";
                if (currentView == ViewMode::Stream) redraw();
                ytdlpProcessOutput.clear();
            }
        }
        if (ytdlpStream.running()) {
            ytdlpStream.poll();
            append_ytdlp_log(ytdlpStream.take_log());
            if (ytdlpSeekBuffering) finish_ytdlp_buffer_if_ready();
            if (ytdlpStream.failed() && !ytdlpSeekBuffering) {
                ytdlpStatus="Stream cache feeder ended with an error. See log.";
                if (currentView==ViewMode::Stream) redraw();
            }
        }
    }
    void stop_ytdlp_process() {
        if (ytdlpPipe >= 0) { close(ytdlpPipe); ytdlpPipe=-1; }
        if (ytdlpPid > 0) {
            kill(ytdlpPid, SIGTERM);
            int status=0;
            bool done=false;
            for (int i=0; i<20; ++i) {
                pid_t r = waitpid(ytdlpPid, &status, WNOHANG);
                if (r == ytdlpPid || r == -1) { done=true; break; }
                usleep(25000);
            }
            if (!done) {
                kill(ytdlpPid, SIGKILL);
                waitpid(ytdlpPid, &status, WNOHANG);
            }
            ytdlpPid=-1;
        }
        ytdlpJob=YtDlpJob::Idle;
        ytdlpProcessOutput.clear();
        stop_ytdlp_stream_process();
    }
    std::string read_clipboard_x11() {
        Atom clipboard = XInternAtom(d, "CLIPBOARD", False);
        Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
        Atom property = XInternAtom(d, "REDDMEDIA_CLIPBOARD_TEXT", False);
        XDeleteProperty(d, win, property);
        XConvertSelection(d, clipboard, utf8, property, win, CurrentTime);
        XFlush(d);
        long long start = now_ms();
        while (now_ms() - start < 700) {
            while (XPending(d)) {
                XEvent ev; XNextEvent(d, &ev);
                if (ev.type == SelectionNotify) {
                    if (ev.xselection.property == None) return "";
                    Atom actualType = None; int actualFormat = 0; unsigned long nitems = 0, bytesAfter = 0; unsigned char* data = nullptr;
                    int rc = XGetWindowProperty(d, win, property, 0, 1024 * 1024, False, AnyPropertyType, &actualType, &actualFormat, &nitems, &bytesAfter, &data);
                    std::string out;
                    if (rc == Success && data && nitems > 0) out.assign((char*)data, (size_t)nitems);
                    if (data) XFree(data);
                    XDeleteProperty(d, win, property);
                    return out;
                }
            }
            usleep(10000);
        }
        return "";
    }
    void own_clipboard_text(const std::string& value) {
        ownedClipboardText = value;
        XSetSelectionOwner(d, clipboardAtom, win, CurrentTime);
        XFlush(d);
        if (XGetSelectionOwner(d, clipboardAtom) != win) ownedClipboardText.clear();
    }
    void handle_clipboard_selection_request(const XSelectionRequestEvent& req) {
        XSelectionEvent reply{};
        reply.type = SelectionNotify;
        reply.display = req.display;
        reply.requestor = req.requestor;
        reply.selection = req.selection;
        reply.target = req.target;
        reply.time = req.time;
        reply.property = None;
        Atom property = req.property == None ? req.target : req.property;
        if (req.selection == clipboardAtom && !ownedClipboardText.empty()) {
            if (req.target == targetsAtom) {
                Atom supported[] = {targetsAtom, utf8Atom, XA_STRING, textAtom};
                XChangeProperty(d, req.requestor, property, XA_ATOM, 32, PropModeReplace,
                                reinterpret_cast<unsigned char*>(supported), 4);
                reply.property = property;
            } else if (req.target == utf8Atom || req.target == XA_STRING || req.target == textAtom) {
                Atom type = req.target == XA_STRING ? XA_STRING : utf8Atom;
                XChangeProperty(d, req.requestor, property, type, 8, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(ownedClipboardText.data()),
                                static_cast<int>(ownedClipboardText.size()));
                reply.property = property;
            }
        }
        XSendEvent(d, req.requestor, False, 0, reinterpret_cast<XEvent*>(&reply));
        XFlush(d);
    }
    void copy_url_selection() {
        if (!urlSelectAll || ytdlpUrl.empty()) return;
        own_clipboard_text(ytdlpUrl);
        ytdlpStatus = "URL copied.";
        redraw();
    }
    void cut_url_selection() {
        if (!urlSelectAll || ytdlpUrl.empty()) return;
        own_clipboard_text(ytdlpUrl);
        ytdlpUrl.clear();
        urlSelectAll = false;
        ytdlpStatus = "URL cut.";
        redraw();
    }
    void paste_into_url() {
        urlFocused = true;
        XSetInputFocus(d, win, RevertToParent, CurrentTime);
        std::string clip;
        if (clipboardAtom != None && XGetSelectionOwner(d, clipboardAtom) == win && !ownedClipboardText.empty()) {
            clip = ownedClipboardText;
        } else {
            clip = read_clipboard_text();
            if (clip.empty()) clip = read_clipboard_x11();
        }
        clip.erase(std::remove(clip.begin(), clip.end(), '\r'), clip.end());
        while (!clip.empty() && (clip.back() == '\n' || clip.back() == '\t' || clip.back() == ' ')) clip.pop_back();
        while (!clip.empty() && (clip.front() == '\n' || clip.front() == '\t' || clip.front() == ' ')) clip.erase(clip.begin());
        if (!clip.empty()) {
            if (urlSelectAll) ytdlpUrl.clear();
            ytdlpUrl += clip;
            urlSelectAll = false;
            ytdlpStatus = "URL pasted.";
        } else ytdlpStatus = "Clipboard is empty.";
        redraw();
    }
    std::string clipboard_value() {
        if (clipboardAtom != None && XGetSelectionOwner(d, clipboardAtom) == win && !ownedClipboardText.empty()) return ownedClipboardText;
        std::string clip=read_clipboard_text();
        if (clip.empty()) clip=read_clipboard_x11();
        clip.erase(std::remove(clip.begin(),clip.end(),'\r'),clip.end());
        while (!clip.empty() && (clip.back()=='\n'||clip.back()=='\t'||clip.back()==' ')) clip.pop_back();
        while (!clip.empty() && (clip.front()=='\n'||clip.front()=='\t'||clip.front()==' ')) clip.erase(clip.begin());
        return clip;
    }
    void copy_p2p_url_selection() {
        if (!p2pMagnetSelectAll || p2pMagnet.empty()) return;
        own_clipboard_text(p2pMagnet); p2pUiStatus="Magnet copied."; redraw();
    }
    void cut_p2p_url_selection() {
        if (!p2pMagnetSelectAll || p2pMagnet.empty()) return;
        own_clipboard_text(p2pMagnet); p2pMagnet.clear(); p2pMagnetSelectAll=false; p2pUiStatus="Magnet cut."; redraw();
    }
    void paste_into_p2p_url() {
        p2pMagnetFocused=true; XSetInputFocus(d,win,RevertToParent,CurrentTime);
        std::string clip=clipboard_value();
        if (!clip.empty()) { if (p2pMagnetSelectAll) p2pMagnet.clear(); p2pMagnet+=clip; p2pMagnetSelectAll=false; p2pUiStatus="Magnet pasted."; }
        else p2pUiStatus="Clipboard is empty.";
        redraw();
    }

    void start_ytdlp_download() {
        sync_stream_platform_from_url();
        if (ytdlpPid > 0) { ytdlpStatus = "Download already running."; redraw(); return; }
        if (ytdlpUrl.empty()) { ytdlpStatus = "Paste or type a URL first."; redraw(); return; }
        std::string engine = ytdlp_engine_path();
        if (!exists_file(engine)) { ytdlpStatus = "Stream engine (yt-dlp) is missing."; redraw(); return; }
        int pipefd[2];
        if (pipe(pipefd) != 0) { ytdlpStatus = "Could not start download pipe."; redraw(); return; }
        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]); close(pipefd[1]);
            execl(engine.c_str(), engine.c_str(), "--newline", "-P", ytdlpOutputFolder.c_str(), ytdlpUrl.c_str(), (char*)nullptr);
            _exit(127);
        }
        close(pipefd[1]);
        if (pid < 0) { close(pipefd[0]); ytdlpStatus = "Could not start Stream download."; redraw(); return; }
        ytdlpPid = pid;
        ytdlpPipe = pipefd[0];
        ytdlpJob = YtDlpJob::Download;
        ytdlpProcessOutput.clear();
        fcntl(ytdlpPipe, F_SETFL, fcntl(ytdlpPipe, F_GETFL, 0) | O_NONBLOCK);
        ytdlpLog.clear();
        ytdlpStatus = "Downloading...";
        redraw();
    }
    void start_ytdlp_play() {
        cancel_tv_autoplay();
        if (ytdlpPid > 0) { ytdlpStatus = "Stream download is already running."; redraw(); return; }
        if (ytdlpUrl.empty()) { ytdlpStatus = "Paste or type a URL first."; redraw(); return; }
        std::string engine = ytdlp_engine_path();
        if (!exists_file(engine)) { ytdlpStatus = "Stream engine (yt-dlp) is missing."; redraw(); return; }

        ytdlpTotalDurationMs = 0;
        ytdlpLog.clear();
        ytdlpStatus = "Reading Stream duration...";
        redraw();
        ytdlpTotalDurationMs = resolve_ytdlp_duration_ms(engine, ytdlpUrl);
        start_ytdlp_cache_playback_at(0);
    }

    NavigationSnapshot capture_navigation_snapshot() const {
        NavigationSnapshot snapshot;
        snapshot.view = currentView;
        snapshot.library_type_chosen = libraryTypeChosen;
        snapshot.library_media_type = libraryMediaType;
        snapshot.library_parents = libraryParents;
        snapshot.library_selected = librarySelected;
        snapshot.library_scroll = libraryScroll;
        snapshot.discover_service_settings = discoverServiceSettings;
        snapshot.discover_mode = discoverMode;
        snapshot.discover_source = discoverSource;
        snapshot.discover_media_type = discoverMediaType;
        snapshot.discover_target_selected = discoverTargetSelected;
        snapshot.nougat_panel = nougatPanel;
        snapshot.nougat_network_advanced = nougatNetworkAdvanced;
        snapshot.stream_platform = streamPlatform;
        return snapshot;
    }

    bool same_navigation_snapshot(const NavigationSnapshot& a, const NavigationSnapshot& b) const {
        if (a.view != b.view ||
            a.library_type_chosen != b.library_type_chosen ||
            a.library_media_type != b.library_media_type ||
            a.library_selected != b.library_selected ||
            a.library_scroll != b.library_scroll ||
            a.discover_service_settings != b.discover_service_settings ||
            a.discover_mode != b.discover_mode ||
            a.discover_source != b.discover_source ||
            a.discover_media_type != b.discover_media_type ||
            a.discover_target_selected != b.discover_target_selected ||
            a.nougat_panel != b.nougat_panel ||
            a.nougat_network_advanced != b.nougat_network_advanced ||
            a.stream_platform != b.stream_platform ||
            a.library_parents.size() != b.library_parents.size()) return false;
        for (std::size_t i = 0; i < a.library_parents.size(); ++i) {
            if (a.library_parents[i].id != b.library_parents[i].id ||
                a.library_parents[i].kind != b.library_parents[i].kind) return false;
        }
        return true;
    }

    void push_navigation_history() {
        if (navigationRestoring) return;
        const NavigationSnapshot current = capture_navigation_snapshot();
        if (navigationBackStack.empty() || !same_navigation_snapshot(navigationBackStack.back(), current)) {
            navigationBackStack.push_back(current);
            if (navigationBackStack.size() > 64U) navigationBackStack.erase(navigationBackStack.begin());
        }
        navigationForwardStack.clear();
    }

    void apply_navigation_snapshot(const NavigationSnapshot& snapshot) {
        navigationRestoring = true;
        currentView = snapshot.view;
        libraryTypeChosen = snapshot.library_type_chosen;
        libraryMediaType = snapshot.library_media_type;
        libraryParents = snapshot.library_parents;
        librarySelected = snapshot.library_selected;
        libraryScroll = snapshot.library_scroll;
        discoverServiceSettings = snapshot.discover_service_settings;
        discoverMode = snapshot.discover_mode;
        discoverSource = snapshot.discover_source;
        discoverMediaType = snapshot.discover_media_type;
        discoverTargetSelected = snapshot.discover_target_selected;
        nougatPanel = snapshot.nougat_panel;
        nougatNetworkAdvanced = snapshot.nougat_network_advanced;
        streamPlatform = snapshot.stream_platform;
        urlFocused = false;
        urlSelectAll = false;
        p2pMagnetFocused = false;
        p2pMagnetSelectAll = false;
        close_context_menu();
        layout();
        apply_video_layout();
        navigationRestoring = false;

        if (currentView == ViewMode::Home) {
            start_home_task();
            redraw();
        } else if (currentView == ViewMode::Library) {
            if (!libraryTypeChosen) start_library_task(0);
            else if (libraryParents.empty()) start_library_task(4);
            else start_library_task(5, {}, libraryParents.back());
        } else {
            redraw();
        }
    }

    void navigate_back() {
        if (navigationBackStack.empty()) return;
        const NavigationSnapshot current = capture_navigation_snapshot();
        NavigationSnapshot target = navigationBackStack.back();
        navigationBackStack.pop_back();
        if (same_navigation_snapshot(target, current) && !navigationBackStack.empty()) {
            target = navigationBackStack.back();
            navigationBackStack.pop_back();
        }
        navigationForwardStack.push_back(current);
        if (navigationForwardStack.size() > 64U) navigationForwardStack.erase(navigationForwardStack.begin());
        apply_navigation_snapshot(target);
    }

    void navigate_forward() {
        if (navigationForwardStack.empty()) return;
        const NavigationSnapshot current = capture_navigation_snapshot();
        const NavigationSnapshot target = navigationForwardStack.back();
        navigationForwardStack.pop_back();
        if (navigationBackStack.empty() || !same_navigation_snapshot(navigationBackStack.back(), current)) {
            navigationBackStack.push_back(current);
            if (navigationBackStack.size() > 64U) navigationBackStack.erase(navigationBackStack.begin());
        }
        apply_navigation_snapshot(target);
    }

    void switch_view(ViewMode v) {
        if (currentView == v) return;
        if (currentView == ViewMode::VideoPlayer) { persist_current_resume(true); clear_seek_preview_hover(); }
        if (currentView == ViewMode::Home) set_home_hover({}, 0);
        push_navigation_history();
        currentView = v;
        urlFocused = false;
        urlSelectAll = false;
        p2pMagnetFocused = false;
        p2pMagnetSelectAll = false;
        close_context_menu();
        apply_video_layout();
        if (currentView == ViewMode::Home) start_home_task();
        if (currentView == ViewMode::LiveTV && liveTvTuners.empty()) refresh_live_tv_tuners(false);
        redraw();
    }
    void scroll_button_row(int& offset, int button_count, int delta, int viewport_width = -1) {
        if (viewport_width < 0) viewport_width = std::max(kCompactButtonW, W - 56);
        offset = clamp_button_scroll(offset + delta, button_count, viewport_width);
        layout();
        redraw();
    }
    void scroll_bottom_controls(int delta) {
        // All eight player actions participate in the scroll extent. The old
        // six-button count made half-screen scrolling stop at Fast Forward.
        scroll_button_row(controlsScrollX, 8, delta, std::max(kCompactButtonW, W - 20));
    }
    void scroll_top_navigation(int delta) {
        scroll_button_row(topNavScrollX, 9, delta, topNavViewportW);
    }

    void close_context_menu() {
        if (contextMenuOpen && contextMenu) {
            XDestroyWindow(d, contextMenu);
        }
        contextMenu=0; contextMenuOpen=false; contextMenuItems.clear();
    }
    int label_pixel_width(const std::string& label) {
        return 18 + (int)label.size() * 8;
    }
    void draw_context_menu() {
        if (!contextMenuOpen || !contextMenu) return;
        unsigned long bg = col(0xf4f4,0xf4f4,0xf4f4);
        unsigned long border = col(0x5555,0x5555,0x5555);
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long muted = col(0x7777,0x7777,0x7777);
        fill(contextMenu, {0,0,contextMenuW,contextMenuH}, bg);
        outline(contextMenu, {0,0,contextMenuW-1,contextMenuH-1}, border);
        for (size_t i=0; i<contextMenuItems.size(); ++i) {
            int y = 24 + (int)i*26;
            text(contextMenu, 12, y, contextMenuItems[i].label, contextMenuItems[i].action == MenuAction::NoAction ? muted : dark);
        }
    }
    void show_menu(Window target, int x, int y, const std::vector<MenuItem>& items) {
        close_context_menu();
        int wx=x, wy=y; Window child=0;
        if (target != win) XTranslateCoordinates(d, target, win, x, y, &wx, &wy, &child);
        contextMenuItems = items;
        contextMenuW = 190;
        for (const MenuItem& it : items) contextMenuW = std::max(contextMenuW, label_pixel_width(it.label));
        contextMenuW = std::min(contextMenuW, std::max(190, W-8));
        contextMenuH = std::max(32, 10 + (int)items.size()*26);
        wx = std::max(0, std::min(wx, std::max(0,W-contextMenuW-4)));
        wy = std::max(0, std::min(wy, std::max(0,H-contextMenuH-4)));
        contextMenu = XCreateSimpleWindow(d, win, wx, wy, contextMenuW, contextMenuH, 1, BlackPixel(d,screen), col(0xf4f4,0xf4f4,0xf4f4));
        XSelectInput(d, contextMenu, ExposureMask|ButtonPressMask);
        XMapRaised(d, contextMenu);
        contextMenuOpen=true;
        draw_context_menu();
        XFlush(d);
    }
    void show_file_menu(int x, int y) {
        std::vector<MenuItem> items;
        items.push_back({"Open File", MenuAction::OpenFile, 0});
        items.push_back({"Exit Nougat Media Suite", MenuAction::ExitApp, 0});
        show_menu(win, x, y, items);
    }
    void show_audio_menu(int x, int y) {
        std::vector<MenuItem> items;
        if (!mp) {
            items.push_back({"Open media first", MenuAction::NoAction, 0});
        } else if (!api.audio_get_track_description || !api.audio_set_track) {
            items.push_back({"Audio tracks unavailable", MenuAction::NoAction, 0});
        } else {
            int current = current_audio_track();
            std::vector<TrackChoice> tracks = audio_tracks();
            if (tracks.empty()) items.push_back({"No audio tracks found", MenuAction::NoAction, 0});
            for (const TrackChoice& t : tracks) {
                std::string label = (t.id == current ? "* " : "  ") + t.name;
                items.push_back({label, MenuAction::AudioTrack, t.id});
            }
        }
        show_menu(win, x, y, items);
    }
    void show_subtitle_menu(int x, int y) {
        std::vector<MenuItem> items;
        items.push_back({subtitlesOn ? "Subtitles Off" : "Subtitles On", MenuAction::SubtitleToggle, 0});
        items.push_back({"Load Subtitle File", MenuAction::SubtitleLoadFile, 0});
        items.push_back({"Open Subtitle Folder", MenuAction::SubtitleLoadFolder, 0});
        items.push_back({"Delay -0.5s (earlier)", MenuAction::SubtitleDelayMinus, 0});
        items.push_back({"Delay +0.5s (later)", MenuAction::SubtitleDelayPlus, 0});
        items.push_back({"Reset Subtitle Delay", MenuAction::SubtitleDelayReset, 0});
        items.push_back({subtitle_delay_label(), MenuAction::NoAction, 0});
        if (mp && api.video_get_spu_description && api.video_set_spu) {
            int current = current_subtitle_track();
            std::vector<TrackChoice> tracks = subtitle_tracks();
            if (!tracks.empty()) items.push_back({"Subtitle Tracks", MenuAction::NoAction, 0});
            for (const TrackChoice& t : tracks) {
                std::string name = t.name;
                if (t.id < 0) name = "Disable subtitles";
                std::string label = (t.id == current ? "* " : "  ") + name;
                items.push_back({label, MenuAction::SubtitleTrack, t.id});
            }
        }
        show_menu(win, x, y, items);
    }
    void show_ytdlp_menu(int x, int y) {
        std::vector<MenuItem> items;
        items.push_back({"Clear Log", MenuAction::YtDlpClearLog, 0});
        show_menu(win, x, y, items);
    }
    void show_url_context_menu(int x, int y) {
        std::vector<MenuItem> items;
        bool hasSelection = urlSelectAll && !ytdlpUrl.empty();
        items.push_back({"Cut", hasSelection ? MenuAction::UrlCut : MenuAction::NoAction, 0});
        items.push_back({"Copy", hasSelection ? MenuAction::UrlCopy : MenuAction::NoAction, 0});
        items.push_back({"Paste", MenuAction::UrlPaste, 0});
        show_menu(win, x, y, items);
    }
    void show_p2p_url_context_menu(int x, int y) {
        std::vector<MenuItem> items;
        bool hasSelection = p2pMagnetSelectAll && !p2pMagnet.empty();
        items.push_back({"Cut", hasSelection ? MenuAction::P2pUrlCut : MenuAction::NoAction, 0});
        items.push_back({"Copy", hasSelection ? MenuAction::P2pUrlCopy : MenuAction::NoAction, 0});
        items.push_back({"Paste", MenuAction::P2pUrlPaste, 0});
        show_menu(win, x, y, items);
    }
    void show_context_menu(Window target, int x, int y) {
        std::vector<MenuItem> items;
        items.push_back({paused ? "Play" : "Pause", MenuAction::TogglePlay, 0});
        items.push_back({fullscreen ? "Exit Fullscreen" : "Fullscreen", MenuAction::ToggleFullscreen, 0});
        items.push_back({"Subtitles On / Off", MenuAction::SubtitleToggle, 0});
        items.push_back({"Rewind 10 seconds", MenuAction::Rewind10, 0});
        items.push_back({"Forward 10 seconds", MenuAction::Forward10, 0});
        update_chapter_marks(true);
        if (chapterMarksAreReal) {
            items.push_back({"Previous Chapter", MenuAction::PrevChapter, 0});
            items.push_back({"Next Chapter", MenuAction::NextChapter, 0});
            int maxList = std::min((int)chapterMarksMs.size(), 10);
            for (int i=0; i<maxList; ++i) {
                std::string name = (i < (int)chapterNames.size() ? chapterNames[(size_t)i] : "Chapter");
                std::string label = "Chapter " + std::to_string(i+1) + ": " + name;
                items.push_back({label, MenuAction::ChapterJump, i});
            }
        } else {
            items.push_back({"No real chapters found", MenuAction::NoAction, 0});
        }
        items.push_back({"Open File", MenuAction::OpenFile, 0});
        show_menu(target, x, y, items);
    }
    void run_menu_action(const MenuItem& item) {
        MenuAction action = item.action;
        int value = item.value;
        close_context_menu();
        switch (action) {
            case MenuAction::OpenFile: do_open(); break;
            case MenuAction::ExitApp: shuttingDown=true; running=false; break;
            case MenuAction::TogglePlay: toggle_play(); break;
            case MenuAction::ToggleFullscreen: if (fullscreen) exit_fullscreen(); else toggle_fullscreen(); break;
            case MenuAction::Rewind10: seek_relative(-10000); break;
            case MenuAction::Forward10: seek_relative(10000); break;
            case MenuAction::SubtitleToggle: toggle_subtitles(); break;
            case MenuAction::SubtitleLoadFile: choose_and_load_subtitle_file(); break;
            case MenuAction::SubtitleLoadFolder: choose_and_load_subtitle_folder(); break;
            case MenuAction::SubtitleDelayPlus: change_subtitle_delay(500000); break;
            case MenuAction::SubtitleDelayMinus: change_subtitle_delay(-500000); break;
            case MenuAction::SubtitleDelayReset: reset_subtitle_delay(); break;
            case MenuAction::SubtitleTrack: set_subtitle_track(value); break;
            case MenuAction::AudioTrack: set_audio_track(value); break;
            case MenuAction::PrevChapter: previous_chapter(); break;
            case MenuAction::NextChapter: next_chapter(); break;
            case MenuAction::ChapterJump: jump_to_chapter_index(value); break;
            case MenuAction::YtDlpClearLog: ytdlpLog = "No download output yet."; ytdlpStatus = "Ready."; redraw(); break;
            case MenuAction::UrlCut: cut_url_selection(); break;
            case MenuAction::UrlCopy: copy_url_selection(); break;
            case MenuAction::UrlPaste: paste_into_url(); break;
            case MenuAction::P2pUrlCut: cut_p2p_url_selection(); break;
            case MenuAction::P2pUrlCopy: copy_p2p_url_selection(); break;
            case MenuAction::P2pUrlPaste: paste_into_p2p_url(); break;
            case MenuAction::NougatCopySelection: copy_nougat_output_selection(); break;
            case MenuAction::NougatSelectAll: select_all_nougat_output(); break;
            case MenuAction::NoAction: break;
        }
    }
    void handle_context_menu_click(int, int y) {
        int idx = (y - 8) / 26;
        if (idx >= 0 && idx < (int)contextMenuItems.size()) run_menu_action(contextMenuItems[(size_t)idx]);
        else close_context_menu();
    }

    bool pointer_crossed_hover_target(int old_x, int old_y, int new_x, int new_y) const {
        const Rect* targets[] = {
            &homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&nougatTab,&ytdlpTab,&studioTab,&debugTab,
            &openBtn,&rewindBtn,&playBtn,&stopBtn,&forwardBtn,&fsBtn,
            &libraryMoviesBtn,&libraryTvBtn,&libraryGridBtn,&libraryListViewBtn,&libraryAddFolderBtn,
            &libraryUnlinkFolderBtn,&libraryRefreshBtn,&libraryBackBtn,
            &discoverUsualTab,&discoverRandomTab,&discoverLocalMovieBtn,&discoverLocalTvBtn,&discoverLiveTvBtn,&discoverExternalMovieBtn,
            &discoverExternalTvBtn,&discoverTmdbTestBtn,&discoverTmdbReplaceBtn,&discoverTmdbClearBtn,&discoverMyServicesBtn,
            &serverStartBtn,&serverStopBtn,&serverRefreshBtn,
            &debugRunBtn,&debugRetryBtn,&debugMetadataBtn,&debugTmdbBtn,&debugLogsBtn,&debugCopyBtn,
            &debugExportTextBtn,&debugExportJsonBtn,&debugBundleBtn,
            &streamYoutubeTab,&streamVimeoTab,&streamRumbleTab,&streamRutubeTab,&streamVkTab,&streamOkTab,
            &ytdlpDownloadBtn,&ytdlpDirectWatchBtn,&ytdlpWebpageBtn,&ytdlpClearBtn,
            &p2pLoadMagnetBtn,&p2pOpenTorrentBtn,&p2pPlayBtn,&p2pStopResumeBtn,&p2pRemoveBtn,
            &p2pSpeedBtn,&p2pSeedRulesBtn,&p2pQueueUpBtn,&p2pQueueDownBtn,&p2pReannounceBtn,&p2pRecheckBtn,&p2pPriorityBtn,
            &liveTvDetectBtn,&liveTvRefreshBtn,&liveTvScanBtn,&liveTvWatchBtn,&liveTvChannelsBtn,&liveTvGuideBtn,&liveTvGuideRefreshBtn,&liveTvNowBtn,&liveTvRecordBtn
        };
        for (const Rect* target : targets) {
            if (target->contains(old_x, old_y) != target->contains(new_x, new_y)) return true;
        }
        return false;
    }

    bool handle_wheel(Window target, int x, int y, unsigned int button) {
        int delta = (button == Button4) ? -40 : 40;
        // Header routing always wins. Home's page/shelf wheel handlers must not
        // swallow wheel events intended for the top navigation strip.
        if (target == win && y < kTopBarH) { scroll_top_navigation(delta); return true; }
        if (currentView == ViewMode::Home && target == win) {
            if (homeContinueArea.contains(x,y)) {
                homeContinueScrollX = std::max(0, homeContinueScrollX + (button == Button4 ? -120 : 120));
                redraw();
                return true;
            }
            const int max_scroll = home_max_page_scroll();
            homePageScroll = std::max(0, std::min(max_scroll, homePageScroll + (button == Button4 ? -120 : 120)));
            redraw();
            return true;
        }
        if (target == win && y >= kPageControlY && y < kPageControlBottom + 4) {
            if (currentView == ViewMode::Stream) { scroll_button_row(streamSourceScrollX,6,delta); return true; }
            if (currentView == ViewMode::Nougat) { scroll_button_row(nougatPanelButtonsScrollX,5,delta); return true; }
            if (currentView == ViewMode::LiveTV) { scroll_button_row(liveTvButtonsScrollX,9,delta); return true; }
            if (currentView == ViewMode::Debug) { scroll_button_row(debugButtonsScrollX,12,delta); return true; }
            if (currentView == ViewMode::Discover && !discoverServiceSettings) { scroll_button_row(discoverButtonsScrollX,11,delta); return true; }
            if (currentView == ViewMode::Library) {
                const Rect frame=page_content_frame(ViewMode::Library);
                const int innerX=frame.x+16;
                const int viewport=std::max(kCompactButtonW,libraryListViewBtn.x-innerX-8);
                scroll_button_row(libraryButtonsScrollX,6,delta,viewport); return true;
            }
        }
        if (target == win && currentView == ViewMode::Stream && y >= 198 && y < 234) { scroll_button_row(ytdlpButtonsScrollX,4,delta); return true; }
        if (target == win && currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && y >= 224 && y < 266) { scroll_button_row(p2pButtonsScrollX,5,delta); return true; }
        if (currentView == ViewMode::LiveTV && target == win && liveTvListBox.contains(x,y)) {
            const int visible = liveTvGuideMode ? std::max(1,(liveTvListBox.h-108)/46) : std::max(1,(liveTvListBox.h-180)/26);
            const int maxScroll=std::max(0,static_cast<int>(liveTvChannels.size())-visible);
            liveTvGuideChannelScroll=std::max(0,std::min(maxScroll,liveTvGuideChannelScroll+(button==Button4?-1:1)));
            redraw(); return true;
        }
        if (currentView == ViewMode::Library && target == win && libraryListBox.contains(x,y)) {
            const std::size_t count = library_visible_indices().size();
            const LibraryGridMetrics grid = library_grid_metrics();
            const int max_scroll = std::max(0, static_cast<int>(count) - grid.visibleItems);
            libraryScroll = std::max(0, std::min(max_scroll,
                libraryScroll + (button == Button4 ? -grid.columns : grid.columns)));
            redraw();
            return true;
        }
        if (currentView == ViewMode::Discover && target == win) {
            if (discoverServiceSettings) {
                const Rect services_box = {28, 120, std::max(240, W - 56), std::max(150, H - 148)};
                if (services_box.contains(x,y)) {
                    std::size_t count = 0;
                    {
                        std::lock_guard<std::mutex> lock(discoverState->mutex);
                        count = discoverState->providerCatalog.size();
                    }
                    const int visible = std::max(1, (services_box.h - 12) / 28);
                    const int max_scroll = std::max(0, static_cast<int>(count) - visible);
                    discoverServicesScroll = std::max(0, std::min(max_scroll,
                        discoverServicesScroll + (button == Button4 ? -1 : 1)));
                    redraw();
                    return true;
                }
            } else if (discoverResultBox.contains(x,y)) {
                discoverDetailsScroll = std::max(0,
                    discoverDetailsScroll + (button == Button4 ? -1 : 1));
                redraw();
                return true;
            }
        }
        if (currentView == ViewMode::Debug && target == win && debugListBox.contains(x,y)) {
            debugScroll = std::max(0, debugScroll + (button == Button4 ? -1 : 1));
            redraw();
            return true;
        }
        if (currentView == ViewMode::Nougat && target == win) {
            if (nougatPanel == NougatPanel::Search && nougatResultsBox.contains(x,y)) {
                nougatResultScroll = std::max(0, nougatResultScroll + (button == Button4 ? -1 : 1));
                redraw();
                return true;
            }
            if (nougatPanel == NougatPanel::Crawler && nougatCrawlLogBox.contains(x,y)) {
                std::size_t count = 0;
                { std::lock_guard<std::mutex> lock(nougatState->mutex); count = nougatState->crawl_log.size(); }
                const int visible = std::max(1, ((int)nougatCrawlLogBox.h - 16) / 18);
                const int max_scroll = std::max(0, static_cast<int>(count) - visible);
                nougatCrawlScroll = std::max(0, std::min(max_scroll, nougatCrawlScroll + (button == Button4 ? -3 : 3)));
                redraw();
                return true;
            }
            if (nougatPanel == NougatPanel::Search && nougatNetworkAdvanced && nougatPeerListBox.contains(x,y)) {
                std::size_t count = 0;
                { std::lock_guard<std::mutex> lock(nougatState->mutex); count = nougatState->peers.size(); }
                const int visible = std::max(1, ((int)nougatPeerListBox.h - 16) / 22);
                const int max_scroll = std::max(0, static_cast<int>(count) - visible);
                nougatPeerScroll = std::max(0, std::min(max_scroll, nougatPeerScroll + (button == Button4 ? -2 : 2)));
                redraw();
                return true;
            }
        }
        if (currentView == ViewMode::VideoPlayer && target == win && y >= H-40 && !volRect.contains(x,y)) {
            scroll_bottom_controls(delta);
            return true;
        }
        if (target == video || (target == win && volRect.contains(x,y))) {
            adjust_volume(button == Button4 ? 5 : -5);
            return true;
        }
        return true;
    }

    void handle_button(Window target, int x, int y, unsigned int button, Time eventTime) {
        if (contextMenuOpen && target == contextMenu) { handle_context_menu_click(x,y); return; }
        if (contextMenuOpen) close_context_menu();
        if (currentView == ViewMode::Stream && target == win && ytdlpUrlRect.contains(x,y) && button == Button3) {
            urlFocused = true;
            XSetInputFocus(d, win, RevertToParent, CurrentTime);
            redraw();
            show_url_context_menu(x, y);
            return;
        }
        if (currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && target == win && p2pMagnetRect.contains(x,y) && button == Button3) {
            p2pMagnetFocused=true;
            XSetInputFocus(d,win,RevertToParent,CurrentTime);
            redraw(); show_p2p_url_context_menu(x,y); return;
        }
        if (currentView == ViewMode::Nougat && target == win && nougatPanel == NougatPanel::Crawler && nougatCrawlLogBox.contains(x,y) && button == Button3) {
            nougatOutputFocused=true; nougatInputFocus=NougatInputFocus::NoFocus;
            redraw(); show_nougat_output_context_menu(x,y); return;
        }
        if (target == video) {
            if (button == Button3) { show_context_menu(target, x, y); return; }
            if (button != Button1 && !(currentView == ViewMode::Stream && ytdlpUrlRect.contains(x,y) && button == Button3)) return;
            if (resumePromptVisible) {
                pendingVideoSingleClick = false;
                if (videoResumeBtn.contains(x,y)) { continue_pending_resume(); return; }
                if (videoRestartBtn.contains(x,y)) { start_over_pending_resume(); return; }
                if (videoCancelBtn.contains(x,y)) { cancel_pending_resume(); return; }
                return;
            }
            if (stoppedPlaybackVisible) {
                pendingVideoSingleClick = false;
                if (videoResumeBtn.contains(x,y)) {
                    const long long resume_at = stoppedPlaybackPositionMs;
                    stoppedPlaybackVisible = false;
                    open_media(currentPath, resume_at);
                    return;
                }
                if (videoRestartBtn.contains(x,y)) {
                    resumeStore.clear_position(currentPath);
                    homeNeedsRefresh.store(true);
                    stoppedPlaybackVisible = false;
                    open_media(currentPath, 0);
                    return;
                }
                if (videoLoadBtn.contains(x,y)) { stoppedPlaybackVisible = false; do_open(); return; }
                if (videoBackLibraryBtn.contains(x,y)) { stoppedPlaybackVisible = false; switch_view(ViewMode::Library); return; }
                return;
            }
            if (upNextVisible) {
                pendingVideoSingleClick = false;
                if (upNextHasEpisode && videoUpNextPlayBtn.contains(x,y)) { play_up_next_now(); return; }
                if (videoUpNextSeriesBtn.contains(x,y)) { back_to_series_from_up_next(); return; }
                if (videoUpNextReplayBtn.contains(x,y)) { replay_active_episode(); return; }
                return;
            }
            if (needResumePrompt && videoResumeBtn.contains(x,y)) { pendingVideoSingleClick=false; open_media(sessionPath, sessionTime); return; }
            if (needResumePrompt && videoRestartBtn.contains(x,y)) { pendingVideoSingleClick=false; needResumePrompt=false; open_media(sessionPath, 0); return; }
            if (needResumePrompt && videoCancelBtn.contains(x,y)) { pendingVideoSingleClick=false; needResumePrompt=false; redraw(); return; }
            if (lastClickTime && eventTime - lastClickTime < 350 && abs(x-lastClickX)<8 && abs(y-lastClickY)<8) {
                pendingVideoSingleClick=false;
                toggle_fullscreen(); lastClickTime=0; return;
            }
            lastClickTime=eventTime; lastClickX=x; lastClickY=y;
            pendingVideoSingleClick=true;
            pendingVideoSingleClickDeadlineMs=now_ms()+360;
            return;
        }
        if (button != Button1) return;
        const bool topNavHit = y < kTopBarH && x >= topNavClipX && x < topNavClipRight;
        if (topNavHit && homeTab.contains(x,y)) {
            if (currentView != ViewMode::Home) switch_view(ViewMode::Home);
            else start_home_task();
            return;
        }
        if (topNavHit && videoPlayerTab.contains(x,y)) {
            if (currentView != ViewMode::VideoPlayer) { switch_view(ViewMode::VideoPlayer); return; }
            std::vector<MenuItem> items;
            items.push_back({"Open File", MenuAction::OpenFile, 0});
            items.push_back({"Audio", MenuAction::NoAction, 0});
            std::vector<TrackChoice> ats = audio_tracks();
            int ca = current_audio_track();
            for (const TrackChoice& t : ats) items.push_back({(t.id==ca?"* ":"  ") + t.name, MenuAction::AudioTrack, t.id});
            items.push_back({subtitlesOn ? "Subtitles Off" : "Subtitles On", MenuAction::SubtitleToggle, 0});
            items.push_back({"Load Subtitle File", MenuAction::SubtitleLoadFile, 0});
            items.push_back({"Open Subtitle Folder", MenuAction::SubtitleLoadFolder, 0});
            items.push_back({"Delay -0.5s (earlier)", MenuAction::SubtitleDelayMinus, 0});
            items.push_back({"Delay +0.5s (later)", MenuAction::SubtitleDelayPlus, 0});
            items.push_back({"Reset Subtitle Delay", MenuAction::SubtitleDelayReset, 0});
            items.push_back({"Exit Nougat Media Suite", MenuAction::ExitApp, 0});
            show_menu(win, 8, kTopBarH, items);
            return;
        }
        if (topNavHit && libraryTab.contains(x,y)) {
            if (currentView != ViewMode::Library) {
                switch_view(ViewMode::Library);
                if (!libraryTypeChosen) start_library_task(0);
            }
            return;
        }
        if (topNavHit && discoverTab.contains(x,y)) {
            if (currentView != ViewMode::Discover) switch_view(ViewMode::Discover);
            return;
        }
        if (topNavHit && liveTvTab.contains(x,y)) {
            if (currentView != ViewMode::LiveTV) switch_view(ViewMode::LiveTV);
            return;
        }
        if (topNavHit && nougatTab.contains(x,y)) {
            if (currentView != ViewMode::Nougat) switch_view(ViewMode::Nougat);
            return;
        }
        if (topNavHit && ytdlpTab.contains(x,y)) {
            if (currentView != ViewMode::Stream) { switch_view(ViewMode::Stream); return; }
            show_ytdlp_menu(ytdlpTab.x, 26);
            return;
        }
        if (topNavHit && studioTab.contains(x,y)) {
            if (currentView != ViewMode::Studio) switch_view(ViewMode::Studio);
            return;
        }
        if (topNavHit && debugTab.contains(x,y)) {
            if (currentView != ViewMode::Debug) switch_view(ViewMode::Debug);
            return;
        }
        if (currentView == ViewMode::Home) {
            if (handle_home_scrollbar_press(x, y)) return;
            handle_home_click(x, y);
            return;
        }
        if (currentView == ViewMode::Nougat) {
            handle_nougat_click(x, y);
            return;
        }
        if (currentView == ViewMode::LiveTV) {
            handle_live_tv_click(x,y,eventTime);
            return;
        }
        if (currentView == ViewMode::Library) {
            if (librarySearchRect.contains(x,y)) {
                librarySearchFocused = true;
                librarySearchSelectAll = false;
                XSetInputFocus(d, win, RevertToParent, CurrentTime);
                redraw();
                return;
            }
            if (librarySearchBtn.contains(x,y)) {
                // Filtering is live while typing; Search explicitly commits the current query
                // and returns focus to the Library so mouse/keyboard navigation can continue.
                librarySearchFocused = false;
                librarySearchSelectAll = false;
                librarySelected = -1;
                libraryScroll = 0;
                redraw();
                return;
            }
            librarySearchFocused = false;
            librarySearchSelectAll = false;
            if (handle_library_scrollbar_press(x,y)) return;
            // Fixed view controls win hit-testing at the far right.
            if (libraryGridBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::Grid); return; }
            if (libraryListViewBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::List); return; }
            const Rect libraryFrame = page_content_frame(ViewMode::Library);
            const int libraryToolRight = libraryListViewBtn.x - 8;
            const bool inLibraryToolViewport = y >= kPageControlY && y < kPageControlBottom &&
                x >= libraryFrame.x + 16 && x < libraryToolRight;
            if (inLibraryToolViewport && libraryMoviesBtn.contains(x,y)) {
                select_library_type(reddmedia::LibraryMediaType::Movies);
                return;
            }
            if (inLibraryToolViewport && libraryTvBtn.contains(x,y)) {
                select_library_type(reddmedia::LibraryMediaType::Television);
                return;
            }
            if (inLibraryToolViewport && libraryAddFolderBtn.contains(x,y)) { add_library_folder(); return; }
            if (inLibraryToolViewport && libraryUnlinkFolderBtn.contains(x,y)) { unlink_library_folder(); return; }
            if (inLibraryToolViewport && libraryRefreshBtn.contains(x,y)) {
                if (libraryTypeChosen) start_library_task(2);
                return;
            }
            if (inLibraryToolViewport && libraryBackBtn.contains(x,y)) { library_back(); return; }
            for (std::size_t row = 0; row < libraryRows.size(); ++row) {
                if (libraryRows[row].contains(x,y)) {
                    if (row < libraryRowNodeIndices.size()) librarySelected = libraryRowNodeIndices[row];
                    redraw();
                    open_selected_library_item();
                    return;
                }
            }
            return;
        }
        if (currentView == ViewMode::Discover) {
            if (discoverServiceSettings) {
                if (discoverServicesBackBtn.contains(x,y)) {
                    push_navigation_history();
                    discoverServiceSettings = false;
                    redraw();
                    return;
                }
                for (const auto& row : discoverProviderRows) {
                    if (!row.first.contains(x,y)) continue;
                    std::string error;
                    if (!watchPreferences.toggle(row.second, error)) {
                        std::lock_guard<std::mutex> lock(discoverState->mutex);
                        discoverState->status = error;
                    }
                    redraw();
                    return;
                }
                return;
            }
            if (discoverUsualTab.contains(x,y)) {
                if (discoverMode != reddmedia::RecommendationMode::Usual) push_navigation_history();
                discoverMode = reddmedia::RecommendationMode::Usual;
                redraw();
                return;
            }
            if (discoverRandomTab.contains(x,y)) {
                if (discoverMode != reddmedia::RecommendationMode::Random) push_navigation_history();
                discoverMode = reddmedia::RecommendationMode::Random;
                redraw();
                return;
            }
            if (discoverLocalMovieBtn.contains(x,y)) {
                start_discover_task(reddmedia::RecommendationSource::Local,
                                    reddmedia::RecommendationMediaType::Movie);
                return;
            }
            if (discoverLocalTvBtn.contains(x,y)) {
                start_discover_task(reddmedia::RecommendationSource::Local,
                                    reddmedia::RecommendationMediaType::Television);
                return;
            }
            if (discoverLiveTvBtn.contains(x,y)) {
                start_discover_task(reddmedia::RecommendationSource::LiveTV,
                                    reddmedia::RecommendationMediaType::Television);
                return;
            }
            if (discoverExternalMovieBtn.contains(x,y)) {
                start_discover_task(reddmedia::RecommendationSource::External,
                                    reddmedia::RecommendationMediaType::Movie);
                return;
            }
            if (discoverExternalTvBtn.contains(x,y)) {
                start_discover_task(reddmedia::RecommendationSource::External,
                                    reddmedia::RecommendationMediaType::Television);
                return;
            }
            if (discoverTmdbTestBtn.contains(x,y)) {
                start_tmdb_credential_task(1);
                return;
            }
            if (discoverTmdbReplaceBtn.contains(x,y)) {
                const std::string credential = choose_tmdb_credential_dialog();
                if (!credential.empty()) start_tmdb_credential_task(2, credential);
                return;
            }
            if (discoverTmdbClearBtn.contains(x,y)) {
                start_tmdb_credential_task(3);
                return;
            }
            if (discoverMyServicesBtn.contains(x,y)) {
                push_navigation_history();
                start_provider_catalog_task();
                return;
            }
            if (discoverOpenBtn.contains(x,y)) { open_discover_result(); return; }
            if (discoverWatchBtn.contains(x,y)) { open_watch_options(); return; }
            return;
        }
        if (currentView == ViewMode::Debug) {
            if (serverStartBtn.contains(x,y)) { start_server_task(1); return; }
            if (serverStopBtn.contains(x,y)) { start_server_task(2); return; }
            if (serverRefreshBtn.contains(x,y)) { start_server_task(3); return; }
            if (debugRunBtn.contains(x,y) || debugRetryBtn.contains(x,y)) {
                start_debug_task();
                return;
            }
            if (debugMetadataBtn.contains(x,y)) {
                if (libraryTypeChosen) start_library_task(2);
                else {
                    {
                        std::lock_guard<std::mutex> lock(debugState->mutex);
                        debugState->status = "Choose Movies or TV in Library before refreshing metadata.";
                    }
                    redraw();
                }
                return;
            }
            if (debugTmdbBtn.contains(x,y)) { start_tmdb_credential_task(1); return; }
            if (debugLogsBtn.contains(x,y)) { open_debug_logs(); return; }
            if (debugCopyBtn.contains(x,y)) { copy_debug_report(); return; }
            if (debugExportTextBtn.contains(x,y)) { export_debug_report(1); return; }
            if (debugExportJsonBtn.contains(x,y)) { export_debug_report(2); return; }
            if (debugBundleBtn.contains(x,y)) { export_debug_report(3); return; }
            return;
        }
        if (currentView == ViewMode::Stream) {
            if (streamYoutubeTab.contains(x,y)) { select_stream_platform(StreamPlatform::YouTube); return; }
            if (streamVimeoTab.contains(x,y)) { select_stream_platform(StreamPlatform::Vimeo); return; }
            if (streamRumbleTab.contains(x,y)) { select_stream_platform(StreamPlatform::Rumble); return; }
            if (streamRutubeTab.contains(x,y)) { select_stream_platform(StreamPlatform::RuTube); return; }
            if (streamVkTab.contains(x,y)) { select_stream_platform(StreamPlatform::VK); return; }
            if (streamOkTab.contains(x,y)) { select_stream_platform(StreamPlatform::OK); return; }
            if (ytdlpUrlRect.contains(x,y)) {
                urlFocused=true;
                urlSelectAll=false;
                XSetInputFocus(d, win, RevertToParent, CurrentTime);
                ytdlpStatus = "URL field ready. Ctrl+A selects all. Right-click opens Cut / Copy / Paste.";
                redraw();
                return;
            }
            if (ytdlpOutputRect.contains(x,y)) {
                urlFocused=false;
                urlSelectAll=false;
                std::string folder = choose_folder_dialog();
                if (!folder.empty()) { ytdlpOutputFolder = folder; ytdlpStatus = "Output folder set."; }
                redraw();
                return;
            }
            if (ytdlpDownloadBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; start_ytdlp_download(); return; }
            if (ytdlpDirectWatchBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; direct_watch_stream(); return; }
            if (ytdlpWebpageBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; open_stream_webpage(); return; }
            if (ytdlpClearBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; ytdlpLog = "No stream activity yet."; ytdlpStatus = "Ready."; redraw(); return; }
            urlFocused=false;
            urlSelectAll=false;
            redraw();
            return;
        }
        if (openBtn.contains(x,y)) { do_open(); return; }
        if (rewindBtn.contains(x,y)) { seek_relative(-10000); return; }
        if (previousBtn.contains(x,y)) { play_relative_episode(-1); return; }
        if (playBtn.contains(x,y)) { toggle_play(); return; }
        if (nextBtn.contains(x,y)) { play_relative_episode(1); return; }
        if (forwardBtn.contains(x,y)) { seek_relative(10000); return; }
        if (stopBtn.contains(x,y)) { stop_media(); return; }
        if (fsBtn.contains(x,y)) { toggle_fullscreen(); return; }
        if (needResumePrompt && resumeBtn.contains(x,y)) { open_media(sessionPath, sessionTime); return; }
        if (needResumePrompt && loadBtn.contains(x,y)) { needResumePrompt=false; redraw(); do_open(); return; }
        if (seekRect.contains(x,y) && mp) {
            long long l = playback_length_ms();
            if (l > 0) seek_to_ms((long long)((double)(x-seekRect.x)/seekRect.w*l));
            draw_seek_time_only();
            return;
        }
        if (volRect.contains(x,y) && mp) {
            int v = std::max(0, std::min(200, (x-volRect.x)*200/volRect.w)); volumePercent=v; api.set_volume(mp, v); volumeDragging=true; draw_volume_only(); return;
        }
    }
    void run_pending_video_click() {
        if (!pendingVideoSingleClick) return;
        if (now_ms() < pendingVideoSingleClickDeadlineMs) return;
        pendingVideoSingleClick=false;
        lastClickTime=0;
        if (hasMedia && mp) toggle_play();
    }
    void show_pointer() {
        if (pointerHidden) { XDefineCursor(d, video, normalCursor); pointerHidden=false; XFlush(d); }
    }
    void hide_pointer() {
        if (!pointerHidden) { XDefineCursor(d, video, blankCursor); pointerHidden=true; XFlush(d); }
    }
    bool final_player_cleanup_bounded(int timeoutMs) {
        if (currentMediaIsYtDlpStream || ytdlpSeekBuffering || ytdlpStream.running()) stop_ytdlp_stream_process();
        libvlc_media_player_t* player = mp;
        mp = nullptr;
        hasMedia = false;
        paused = false;
        pendingSeek = false;
        currentMediaIsYtDlpStream = false;
        currentMediaIsNetwork = false;
        if (currentMediaIsLiveTv) {
            currentMediaIsLiveTv=false;
            liveTvPlayingChannel=-1;
            liveTvPlayingLabel.clear();
            liveTvTunerUse=LiveTvTunerUse::Idle;
        }
        if (!player) return true;

        auto done = std::make_shared<std::atomic<bool>>(false);
        const auto stopFn = api.stop;
        const auto releaseFn = api.player_release;
        std::thread worker([player, stopFn, releaseFn, done]() {
            if (stopFn) stopFn(player);
            if (releaseFn) releaseFn(player);
            done->store(true, std::memory_order_release);
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (!done->load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (done->load(std::memory_order_acquire)) {
            worker.join();
            return true;
        }
        worker.detach();
        return false;
    }

    void event_loop() {
        lastMouse = time(nullptr);
        redraw();
        int xfd = ConnectionNumber(d);
        while (running) {
            while (XPending(d)) {
                XEvent e; XNextEvent(d, &e);
                if (e.type == Expose) {
                    if (e.xexpose.count != 0) continue;
                    if (e.xexpose.window == win) {
                        redraw();
                    } else if (e.xexpose.window == video) {
                        draw_video_message();
                        XFlush(d);
                    } else if (seekPreviewWindow && e.xexpose.window == seekPreviewWindow) {
                        draw_seek_preview_window();
                    } else if (contextMenuOpen && e.xexpose.window == contextMenu) {
                        draw_context_menu();
                        XFlush(d);
                    }
                }
                else if (e.type == ConfigureNotify && e.xconfigure.window == win) resize(e.xconfigure.width, e.xconfigure.height);
                else if (e.type == ClientMessage) { shuttingDown=true; running=false; break; }
                else if (e.type == ButtonPress) {
                    if (e.xbutton.button == 8U) navigate_back();
                    else if (e.xbutton.button == 9U) navigate_forward();
                    else if (e.xbutton.button == Button4 || e.xbutton.button == Button5) handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
                else if (e.type == ButtonRelease) {
                    if (currentView == ViewMode::Nougat) nougatOutputSelecting = false;
                    volumeDragging = false;
                    homeVerticalScrollDragging = false;
                    homeContinueScrollDragging = false;
                    libraryVerticalScrollDragging = false;
                }
                else if (e.type == SelectionRequest) handle_clipboard_selection_request(e.xselectionrequest);
                else if (e.type == SelectionClear && e.xselectionclear.selection == clipboardAtom) ownedClipboardText.clear();
                else if (e.type == MotionNotify) {
                    // v0.0.34 direct-scroll repair: collapse queued X11 motion to
                    // the newest pointer position, and never continue a drag once
                    // Button1 is no longer physically held. This removes delayed
                    // replay/coasting after the scrollbar thumb is released.
                    XEvent newerMotion;
                    while (XCheckTypedWindowEvent(d, e.xmotion.window, MotionNotify, &newerMotion)) e = newerMotion;
                    if ((homeVerticalScrollDragging || homeContinueScrollDragging || libraryVerticalScrollDragging) &&
                        (e.xmotion.state & Button1Mask) == 0) {
                        homeVerticalScrollDragging = false;
                        homeContinueScrollDragging = false;
                        libraryVerticalScrollDragging = false;
                    }
                    lastMouse=time(nullptr); show_pointer();
                    if (e.xmotion.window == win) {
                        const int old_x = pointerWindowX;
                        const int old_y = pointerWindowY;
                        const bool moved = old_x != e.xmotion.x || old_y != e.xmotion.y;
                        pointerWindowX = e.xmotion.x;
                        pointerWindowY = e.xmotion.y;
                        if (moved && !fullscreen) {
                            // v0.0.27 flicker repair retained in v0.0.28: raw X11 pointer motion no longer
                            // schedules a full-window repaint. Repaint only when the
                            // pointer crosses a real hover target; dedicated Home and
                            // seek-preview paths update their own state separately.
                            const bool hover_changed = pointer_crossed_hover_target(old_x, old_y, e.xmotion.x, e.xmotion.y);
                            if (currentView == ViewMode::Home) {
                                handle_home_scrollbar_motion(e.xmotion.x, e.xmotion.y);
                                if (!homeVerticalScrollDragging && !homeContinueScrollDragging)
                                    update_home_hover_from_pointer(e.xmotion.x, e.xmotion.y);
                            }
                            if (currentView == ViewMode::Library) handle_library_scrollbar_motion(e.xmotion.y);
                            if (currentView == ViewMode::VideoPlayer) update_seek_preview_hover(e.xmotion.x, e.xmotion.y);
                            if (hover_changed) redraw();
                        }
                    } else if (e.xmotion.window == video) {
                        lastFullscreenOverlayMotionMs = now_ms();
                        if (fullscreen) draw_video_message();
                    }
                    handle_nougat_motion(e.xmotion);
                    if (volumeDragging && currentView==ViewMode::VideoPlayer && mp) {
                        const int v=std::max(0,std::min(200,(e.xmotion.x-volRect.x)*200/std::max(1,volRect.w)));
                        volumePercent=v; api.set_volume(mp,v); draw_volume_only();
                    }
                }
                else if (e.type == EnterNotify && e.xcrossing.window == video) { pointerInVideo=true; lastMouse=time(nullptr); show_pointer(); }
                else if (e.type == LeaveNotify && e.xcrossing.window == video) { pointerInVideo=false; show_pointer(); }
                else if (e.type == KeyPress) {
                    KeySym ks = XLookupKeysym(&e.xkey, 0);
                    if (currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && p2pMagnetFocused) {
                        if (ks == XK_Escape) { p2pMagnetFocused=false; p2pMagnetSelectAll=false; redraw(); }
                        else if (ks == XK_Return || ks == XK_KP_Enter) { start_p2p_magnet(); }
                        else if ((e.xkey.state & ControlMask) && (ks == XK_a || ks == XK_A)) { p2pMagnetSelectAll=!p2pMagnet.empty(); redraw(); }
                        else if (ks == XK_BackSpace) {
                            if (p2pMagnetSelectAll) { p2pMagnet.clear(); p2pMagnetSelectAll=false; }
                            else if (!p2pMagnet.empty()) p2pMagnet.pop_back();
                            redraw();
                        }
                        else if ((e.xkey.state & ControlMask) && (ks == XK_v || ks == XK_V)) paste_into_p2p_url();
                        else if ((e.xkey.state & ShiftMask) && ks == XK_Insert) paste_into_p2p_url();
                        else if ((e.xkey.state & ControlMask) && (ks == XK_u || ks == XK_U)) { p2pMagnet.clear(); p2pMagnetSelectAll=false; redraw(); }
                        else {
                            char buf[32]; KeySym outks=0; int n=XLookupString(&e.xkey,buf,sizeof(buf)-1,&outks,nullptr);
                            if (n>0) { if (p2pMagnetSelectAll) { p2pMagnet.clear(); p2pMagnetSelectAll=false; } buf[n]=0; p2pMagnet+=std::string(buf,n); redraw(); }
                        }
                    }
                    else if (currentView == ViewMode::Nougat) { handle_nougat_key(e.xkey, ks); }
                    else if (currentView == ViewMode::Stream && urlFocused) {
                        if (ks == XK_Escape) { urlFocused=false; urlSelectAll=false; redraw(); }
                        else if (ks == XK_Return || ks == XK_KP_Enter) { start_ytdlp_download(); }
                        else if ((e.xkey.state & ControlMask) && (ks == XK_a || ks == XK_A)) { urlSelectAll = !ytdlpUrl.empty(); redraw(); }
                        else if (ks == XK_BackSpace) {
                            if (urlSelectAll) { ytdlpUrl.clear(); urlSelectAll=false; }
                            else if (!ytdlpUrl.empty()) ytdlpUrl.pop_back();
                            redraw();
                        }
                        else if ((e.xkey.state & ControlMask) && (ks == XK_v || ks == XK_V)) { paste_into_url(); }
                        else if ((e.xkey.state & ShiftMask) && ks == XK_Insert) { paste_into_url(); }
                        else if ((e.xkey.state & ControlMask) && (ks == XK_u || ks == XK_U)) { ytdlpUrl.clear(); urlSelectAll=false; redraw(); }
                        else {
                            char buf[32]; KeySym outks=0; int n = XLookupString(&e.xkey, buf, sizeof(buf)-1, &outks, nullptr);
                            if (n > 0) {
                                if (urlSelectAll) { ytdlpUrl.clear(); urlSelectAll=false; }
                                buf[n]=0; ytdlpUrl += std::string(buf, n); redraw();
                            }
                        }
                    } else if (currentView == ViewMode::Library) {
                        if (librarySearchFocused) {
                            if (ks == XK_Escape) {
                                librarySearchFocused=false;
                                librarySearchSelectAll=false;
                                redraw();
                            } else if ((e.xkey.state & ControlMask) && (ks == XK_a || ks == XK_A)) {
                                librarySearchSelectAll=!librarySearchQuery.empty();
                                redraw();
                            } else if ((e.xkey.state & ControlMask) && (ks == XK_u || ks == XK_U)) {
                                librarySearchQuery.clear();
                                librarySearchSelectAll=false;
                                librarySelected=-1;
                                libraryScroll=0;
                                redraw();
                            } else if (ks == XK_BackSpace) {
                                if (librarySearchSelectAll) {
                                    librarySearchQuery.clear();
                                    librarySearchSelectAll=false;
                                } else if (!librarySearchQuery.empty()) {
                                    librarySearchQuery.pop_back();
                                }
                                librarySelected=-1;
                                libraryScroll=0;
                                redraw();
                            } else if (ks == XK_Return || ks == XK_KP_Enter) {
                                librarySearchFocused=false;
                                librarySearchSelectAll=false;
                                redraw();
                            } else {
                                char buf[64]; KeySym outks=0;
                                const int n=XLookupString(&e.xkey,buf,sizeof(buf)-1,&outks,nullptr);
                                if (n>0) {
                                    if (librarySearchSelectAll) {
                                        librarySearchQuery.clear();
                                        librarySearchSelectAll=false;
                                    }
                                    buf[n]=0;
                                    librarySearchQuery.append(buf,static_cast<std::size_t>(n));
                                    librarySelected=-1;
                                    libraryScroll=0;
                                    redraw();
                                }
                            }
                        } else {
                            const std::vector<int> visibleIndices = library_visible_indices();
                            const int count = static_cast<int>(visibleIndices.size());
                            auto selected_position = [&]() {
                                for (int i=0; i<count; ++i) {
                                    if (visibleIndices[static_cast<std::size_t>(i)] == librarySelected) return i;
                                }
                                return count > 0 ? 0 : -1;
                            };
                            if (ks == XK_Return || ks == XK_KP_Enter || ks == XK_space) {
                                open_selected_library_item();
                            } else if ((ks == XK_Up || ks == XK_Left) && count > 0) {
                                const LibraryGridMetrics grid = library_grid_metrics();
                                const int amount = ks == XK_Up ? grid.columns : 1;
                                const int position = std::max(0, selected_position() - amount);
                                librarySelected = visibleIndices[static_cast<std::size_t>(position)];
                                if (position < libraryScroll) libraryScroll = position;
                                redraw();
                            } else if ((ks == XK_Down || ks == XK_Right) && count > 0) {
                                const LibraryGridMetrics grid = library_grid_metrics();
                                const int amount = ks == XK_Down ? grid.columns : 1;
                                const int position = std::min(count - 1, std::max(0, selected_position()) + amount);
                                librarySelected = visibleIndices[static_cast<std::size_t>(position)];
                                if (position >= libraryScroll + grid.visibleItems) {
                                    libraryScroll = std::max(0, position - grid.visibleItems + grid.columns);
                                }
                                redraw();
                            } else if (ks == XK_r || ks == XK_R) {
                                if (libraryTypeChosen) start_library_task(2);
                            } else if (ks == XK_a || ks == XK_A) {
                                add_library_folder();
                            }
                        }
                    } else if (currentView == ViewMode::Discover) {
                        if (ks == XK_Escape && discoverServiceSettings) {
                            discoverServiceSettings = false;
                            redraw();
                        } else if (ks == XK_Up) {
                            if (discoverServiceSettings) discoverServicesScroll = std::max(0, discoverServicesScroll - 1);
                            else discoverDetailsScroll = std::max(0, discoverDetailsScroll - 1);
                            redraw();
                        } else if (ks == XK_Down) {
                            if (discoverServiceSettings) ++discoverServicesScroll;
                            else ++discoverDetailsScroll;
                            redraw();
                        }
                    } else if (currentView == ViewMode::Debug) {
                        if (ks == XK_Up) { debugScroll = std::max(0, debugScroll - 1); redraw(); }
                        else if (ks == XK_Down) { ++debugScroll; redraw(); }
                        else if (ks == XK_r || ks == XK_R) start_debug_task();
                    } else {
                        if (ks == XK_Escape) exit_fullscreen();
                        else if (ks == XK_space) toggle_play();
                        else if (ks == XK_Up) adjust_volume(5);
                        else if (ks == XK_Down) adjust_volume(-5);
                        else if (ks == XK_Left) seek_relative(-10000);
                        else if (ks == XK_Right) seek_relative(10000);
                        else if (ks == XK_o || ks == XK_O) do_open();
                        else if (ks == XK_f || ks == XK_F) toggle_fullscreen();
                    }
                }
            }
            run_pending_video_click();
            tick_resume_seek();
            poll_natural_playback_end();
            poll_up_next_overlay();
            poll_tv_autoplay_retry();
            poll_ytdlp_process();
            poll_nougat_workers();
            poll_home_worker();
            poll_home_preview();
            poll_home_preview_update();
            poll_seek_preview();
            poll_seek_preview_update();
            persist_current_resume(false);
            poll_library_worker();
            poll_poster_worker();
            poll_discover_worker();
            poll_debug_worker();
            poll_server_worker();
            bool server_busy = false;
            {
                std::lock_guard<std::mutex> lock(serverState->mutex);
                server_busy = serverState->busy;
            }
            if (!server_busy && mediaServer.poll()) {
                {
                    std::lock_guard<std::mutex> lock(serverState->mutex);
                    serverState->status = server_control_label();
                    serverState->state = mediaServer.state();
                    serverState->owned = mediaServer.owns_server();
                }
                if (currentView == ViewMode::Home && mediaServer.state() == reddmedia::MediaServerState::Ready) start_home_task();
                if (!fullscreen) redraw();
            }
            double loading_progress = 0.0;
            bool loading_determinate = false;
            std::string loading_label;
            if (!fullscreen && loading_state(loading_progress, loading_determinate, loading_label) &&
                now_ms() - lastLoadingRedrawMs >= 80) {
                lastLoadingRedrawMs = now_ms();
                redraw();
            }
            if (currentMediaIsYtDlpStream && mp) playback_length_ms();
            poll_security_worker();
            poll_live_tv_scan();
            poll_live_tv_guide();
            if (!fullscreen && currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && now_ms()-lastP2PRedrawMs >= 500) { lastP2PRedrawMs=now_ms(); maybe_auto_scan_completed_p2p(); redraw(); }
            if (pointerInVideo && time(nullptr) - lastMouse >= 3) hide_pointer();
            static time_t lastRedraw=0; time_t now=time(nullptr); if (!fullscreen && currentView == ViewMode::VideoPlayer && now != lastRedraw) { draw_seek_time_only(); lastRedraw=now; }
            fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds); timeval tv; tv.tv_sec=0; tv.tv_usec=100000; select(xfd+1, &fds, nullptr, nullptr, &tv);
        }
    }
    void shutdown() {
        shuttingDown = true;
        persist_current_resume(true);
        clear_seek_preview_hover();
        if (homeWorker.joinable()) {
            bool busy = false;
            { std::lock_guard<std::mutex> lock(homeState->mutex); busy = homeState->busy; }
            if (busy) homeWorker.detach(); else homeWorker.join();
        }
        if (libraryWorker.joinable()) {
            bool busy = false;
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                busy = libraryState->busy;
            }
            if (busy) libraryWorker.detach();
            else libraryWorker.join();
        }
        if (discoverWorker.joinable()) {
            bool busy = false;
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                busy = discoverState->busy;
            }
            if (busy) discoverWorker.detach();
            else discoverWorker.join();
        }
        if (posterWorker.joinable()) {
            bool busy = false;
            {
                std::lock_guard<std::mutex> lock(posterState->mutex);
                busy = posterState->busy;
            }
            if (busy) posterWorker.detach();
            else posterWorker.join();
        }
        if (serverWorker.joinable()) serverWorker.join();
        if (debugWorker.joinable()) {
            bool busy = false;
            {
                std::lock_guard<std::mutex> lock(debugState->mutex);
                busy = debugState->busy;
            }
            if (busy) debugWorker.detach();
            else debugWorker.join();
        }
        // v0.0.35 lifetime repair: these workers capture App/NougatBridge state,
        // so they must never be detached across App destruction.  Joining keeps
        // their owner alive until the operation has actually returned.
        if (nougatSearchWorker.joinable()) nougatSearchWorker.join();
        if (nougatCrawlWorker.joinable()) nougatCrawlWorker.join();
        if (liveTvScanWorker.joinable()) {
            { std::lock_guard<std::mutex> lock(liveTvScanState->mutex); liveTvScanState->cancel = true; }
            liveTvScanWorker.join();
        }
        if (liveTvGuideWorker.joinable()) {
            { std::lock_guard<std::mutex> lock(liveTvGuideState->mutex); liveTvGuideState->cancel = true; }
            liveTvGuideWorker.join();
        }
        if (securityWorker.joinable()) {
            bool busy=false;
            { std::lock_guard<std::mutex> lock(securityState->mutex); busy=securityState->busy; }
            if (busy) securityWorker.detach(); else securityWorker.join();
        }
        nougat.stop_node();
        // v0.0.33: closing the desktop UI must not stop the explicitly started
        // Nougat-owned media server. Stop Server is the only normal shutdown path.
        stop_ytdlp_process();
        close_context_menu();
        save_session();
        p2pStream.stop();
        p2p.shutdown();
        if (!final_player_cleanup_bounded(3000)) {
            if (d) XCloseDisplay(d);
            d = nullptr;
            _exit(0);
        }
        if (inst) api.release(inst);
        inst=nullptr;
        if (seekPreviewWindow && d) { XDestroyWindow(d, seekPreviewWindow); seekPreviewWindow = 0; }
        if (xextHandle) { dlclose(xextHandle); xextHandle = nullptr; xShapeCombineMask = nullptr; }
        free_quilt_tiles();
        if (d && metadataFontInfo) { XFreeFont(d, metadataFontInfo); metadataFontInfo = nullptr; }
        if (d && sectionFontInfo) { XFreeFont(d, sectionFontInfo); sectionFontInfo = nullptr; }
        if (d && fontInfo) { XFreeFont(d, fontInfo); fontInfo = nullptr; }
        if (d) XCloseDisplay(d);
        d=nullptr;
    }
};

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        printf("Nougat Media Suite v0.0.37\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v25-ui-state-self-test") {
        App app;
        struct ExpectedTint { StreamPlatform platform; unsigned char r,g,b; unsigned blend; };
        const ExpectedTint expected[] = {
            {StreamPlatform::YouTube,205,76,67,22},
            {StreamPlatform::Vimeo,23,213,255,22},
            {StreamPlatform::Rumble,128,154,79,22},
            {StreamPlatform::RuTube,168,107,178,20},
            {StreamPlatform::VK,91,142,174,20},
            {StreamPlatform::OK,211,135,48,22},
        };
        for (const auto& item : expected) {
            unsigned char r=0,g=0,b=0; unsigned blend=0;
            app.quilt_tint_for(ViewMode::Stream,r,g,b,blend,item.platform);
            if (r!=item.r || g!=item.g || b!=item.b || blend!=item.blend) {
                std::fprintf(stderr,"Nougat v0.0.25 Stream provider tint FAIL.\n");
                return 1;
            }
        }
        app.discoverMode = reddmedia::RecommendationMode::Random;
        app.discoverSource = reddmedia::RecommendationSource::Local;
        app.discoverMediaType = reddmedia::RecommendationMediaType::Movie;
        app.discoverTargetSelected = true;
        if (!app.discover_mode_selected(reddmedia::RecommendationMode::Random) ||
            !app.discover_target_selected(reddmedia::RecommendationSource::Local,
                                          reddmedia::RecommendationMediaType::Movie) ||
            app.discover_mode_selected(reddmedia::RecommendationMode::Usual) ||
            app.discover_target_selected(reddmedia::RecommendationSource::External,
                                         reddmedia::RecommendationMediaType::Movie)) {
            std::fprintf(stderr,"Nougat v0.0.25 Discover dual-selection state FAIL.\n");
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.29 UI state PASS: provider quilts and dual Discover selectors.\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v28-ui-state-self-test") {
        App app;
        if (App::home_grid_columns_for_width(650) < 3 ||
            App::home_grid_columns_for_width(430) < 2) {
            std::fprintf(stderr, "Nougat v0.0.28 responsive Home grid FAIL.\n");
            return 1;
        }
        reddmedia::LibraryPoster good;
        good.width = 480; good.height = 720; good.rgb.push_back(0);
        reddmedia::LibraryPoster tiny;
        tiny.width = 120; tiny.height = 180; tiny.rgb.push_back(0);
        reddmedia::LibraryPoster landscape;
        landscape.width = 640; landscape.height = 360; landscape.rgb.push_back(0);
        if (!App::poster_quality_ok(good) || App::poster_quality_ok(tiny) ||
            App::poster_quality_ok(landscape)) {
            std::fprintf(stderr, "Nougat v0.0.28 poster quality gate FAIL.\n");
            return 1;
        }
        const std::string metadata = App::x11_safe_text("2012 \xE2\x80\xA2 Comedy \xE2\x80\xA2 Romance");
        if (metadata.find("\xE2\x80\xA2") != std::string::npos ||
            metadata.find(static_cast<char>(0xB7)) == std::string::npos) {
            std::fprintf(stderr, "Nougat v0.0.28 metadata encoding repair FAIL.\n");
            return 1;
        }
        struct PageTint { ViewMode view; unsigned char r,g,b; unsigned blend; };
        const PageTint expected[] = {
            {ViewMode::Home,91,58,134,58},
            {ViewMode::VideoPlayer,91,52,31,62},
            {ViewMode::Library,77,120,61,56},
            {ViewMode::Discover,158,51,68,56},
            {ViewMode::Nougat,241,227,194,8},
            {ViewMode::Debug,41,40,48,70},
        };
        for (const auto& item : expected) {
            unsigned char r=0,g=0,b=0; unsigned blend=0;
            app.quilt_tint_for(item.view,r,g,b,blend);
            if (r!=item.r || g!=item.g || b!=item.b || blend!=item.blend) {
                std::fprintf(stderr, "Nougat v0.0.28 page palette FAIL.\n");
                return 1;
            }
        }
        std::printf("Nougat Media Suite v0.0.28 UI/artwork state PASS: palette, responsive grid, poster gate, metadata encoding.\n");
        return 0;
    }
    if (argc > 3 && std::string(argv[1]) == "--discover-local-resolver-self-test") {
        App app;
        reddmedia::RecommendationResult result;
        result.item.id = argv[2];
        result.item.title = "Resolver test";
        result.item.local_path = "catalog-entry";
        result.item.media_type = reddmedia::RecommendationMediaType::Television;
        if (argc > 4) {
            reddmedia::MediaDescriptor watched;
            watched.id = "episode-watched";
            watched.title = "Watched episode";
            watched.local_path = argv[4];
            watched.media_type = reddmedia::RecommendationMediaType::Television;
            std::string history_error;
            if (!app.recommendationEngine->record_started(watched,history_error)) {
                std::fprintf(stderr,"Nougat Media Suite Discover resolver history setup FAIL: %s\n",history_error.c_str());
                return 1;
            }
        }
        reddmedia::LibraryNode playable;
        std::string error;
        if (!app.resolve_discover_local_play_target(result,playable,error) ||
            playable.kind != reddmedia::LibraryNodeKind::Episode || playable.path != argv[3]) {
            std::fprintf(stderr,"Nougat Media Suite Discover local-play resolver FAIL: %s\n",error.c_str());
            return 1;
        }
        std::printf("Nougat Media Suite Discover local-play resolver PASS: %s\n",playable.path.c_str());
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v29-tv-reliability-self-test") {
        const std::string base = "/tmp/nougat-v29-up-next-" + std::to_string(static_cast<long long>(getpid()));
        const std::string series_dir = base + "/Show Name";
        const std::string season_dir = series_dir + "/Season 01";
        mkdir(base.c_str(), 0700);
        mkdir(series_dir.c_str(), 0700);
        mkdir(season_dir.c_str(), 0700);
        const std::string e13 = season_dir + "/Show.Name.S01E13.mkv";
        const std::string e14 = season_dir + "/Show.Name.S01E14.mkv";
        const std::string e22 = season_dir + "/Show.Name.S01E22.mkv";
        { std::ofstream(e13).put('\0'); std::ofstream(e14).put('\0'); std::ofstream(e22).put('\0'); }

        App app;
        reddmedia::LibraryNode current = app.inferred_episode_node_for_path(e13);
        std::vector<reddmedia::LibraryNode> queue;
        int index = -1;
        const bool folder_ok = app.build_same_folder_episode_queue(current, queue, index);
        const bool order_ok = folder_ok && index >= 0 && index + 1 < static_cast<int>(queue.size()) &&
            queue[static_cast<std::size_t>(index + 1)].path == e14;
        const bool series_ok = current.series_name == "Show Name";

        // Exercise the actual Up Next overlay state machine without an X11 draw.
        app.tvAutoplayQueue = queue;
        app.tvAutoplayIndex = index;
        app.tvAutoplayArmed = folder_ok;
        app.show_up_next_overlay(false);
        const int countdown = app.up_next_seconds_remaining();
        const bool overlay_ok = app.upNextVisible && app.upNextHasEpisode &&
            app.upNextTargetIndex == index + 1 && app.upNextEpisode.path == e14 &&
            countdown >= 9 && countdown <= 10 &&
            app.upNextMessage == "Playing automatically in 10 seconds.";

        int season = 0, episode = 0;
        const bool parse_ok = parse_episode_code(e13, season, episode) && season == 1 && episode == 13;

        // Natural filename fallback remains available for catalog-confirmed Episode
        // nodes even when SxxExx/1xNN tokens are absent.
        const std::string natural_dir = base + "/Natural";
        mkdir(natural_dir.c_str(), 0700);
        const std::string n2 = natural_dir + "/Episode 2.mkv";
        const std::string n10 = natural_dir + "/Episode 10.mkv";
        { std::ofstream(n2).put('\0'); std::ofstream(n10).put('\0'); }
        reddmedia::LibraryNode natural_current;
        natural_current.kind = reddmedia::LibraryNodeKind::Episode;
        natural_current.path = n2;
        natural_current.name = "Episode 2";
        std::vector<reddmedia::LibraryNode> natural_queue;
        int natural_index = -1;
        const bool natural_ok = app.build_same_folder_episode_queue(natural_current, natural_queue, natural_index) &&
            natural_index >= 0 && natural_index + 1 < static_cast<int>(natural_queue.size()) &&
            natural_queue[static_cast<std::size_t>(natural_index + 1)].path == n10;

        const bool vimeo_ok = App::stream_platform_index(StreamPlatform::YouTube) == 0 &&
            App::stream_platform_index(StreamPlatform::Vimeo) == 1 &&
            std::string(app.stream_platform_name(StreamPlatform::Vimeo)) == "Vimeo" &&
            app.stream_platform_home(StreamPlatform::Vimeo) == "https://vimeo.com/";
        const bool art_ok = [](){
            reddmedia::LibraryPoster portrait; portrait.width=480; portrait.height=720; portrait.rgb.push_back(0);
            reddmedia::LibraryPoster still; still.width=640; still.height=360; still.rgb.push_back(0);
            return App::home_artwork_quality_ok(portrait) && App::home_artwork_quality_ok(still);
        }();

        unlink(e13.c_str()); unlink(e14.c_str()); unlink(e22.c_str());
        unlink(n2.c_str()); unlink(n10.c_str());
        rmdir(natural_dir.c_str()); rmdir(season_dir.c_str()); rmdir(series_dir.c_str()); rmdir(base.c_str());
        if (!parse_ok || !order_ok || !series_ok || !overlay_ok || !natural_ok || !vimeo_ok || !art_ok) {
            std::fprintf(stderr, "Nougat v0.0.29 TV/UI reliability self-test FAIL. parse=%d order=%d series=%d overlay=%d natural=%d vimeo=%d art=%d\n",
                         parse_ok, order_ok, series_ok, overlay_ok, natural_ok, vimeo_ok, art_ok);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.29 TV/UI reliability PASS: same-folder Up Next, 10-second overlay, natural fallback, Vimeo, and Home artwork.\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v30-ui-library-player-self-test") {
        App app;

        // Library Grid must use the available height for multiple visible DVD/poster rows.
        app.libraryListBox = {20, 170, 1260, 540};
        app.libraryMovieView = LibraryDisplayMode::Grid;
        app.libraryMediaType = reddmedia::LibraryMediaType::Movies;
        const LibraryGridMetrics grid = app.library_grid_metrics();
        const bool grid_ok = grid.rows >= 2 && grid.columns >= 5 &&
            grid.posterHeight == grid.tileWidth * 3 / 2 &&
            grid.visibleItems == grid.rows * grid.columns;

        // Retained Home geometry contract. v0.0.37 intentionally promotes
        // Continue Watching to the same physical poster-card family as LOCAL,
        // while retaining the portrait 2:3 geometry established in v0.0.30.
        const int continue_h = app.home_card_height(true, 160);
        const int local_h = app.home_card_height(false, 160);
        const bool home_ratio_ok = local_h == std::max(168, 160 * 3 / 2) + 50 &&
            continue_h == local_h;

        // Previous/Next are episode navigation, independent of the +/-10 second seek buttons.
        const std::string nav_base = "/tmp/nougat-v30-nav-" +
            std::to_string(static_cast<long long>(getpid()));
        mkdir(nav_base.c_str(), 0700);
        std::vector<reddmedia::LibraryNode> nav_queue(3);
        for (int i = 0; i < 3; ++i) {
            nav_queue[static_cast<std::size_t>(i)].kind = reddmedia::LibraryNodeKind::Episode;
            nav_queue[static_cast<std::size_t>(i)].path = nav_base + "/Episode" + std::to_string(i + 1) + ".mkv";
            std::ofstream(nav_queue[static_cast<std::size_t>(i)].path).put('\0');
        }
        app.tvAutoplayQueue = nav_queue;
        app.tvAutoplayIndex = 1;
        app.tvAutoplayArmed = true;
        const bool nav_middle_ok = app.episode_navigation_available(-1) && app.episode_navigation_available(1);
        app.tvAutoplayIndex = 0;
        const bool nav_first_ok = !app.episode_navigation_available(-1) && app.episode_navigation_available(1);
        app.tvAutoplayIndex = 2;
        const bool nav_last_ok = app.episode_navigation_available(-1) && !app.episode_navigation_available(1);
        for (const auto& node : nav_queue) unlink(node.path.c_str());
        rmdir(nav_base.c_str());

        // Persistent metadata cache must round-trip real metadata without delimiter corruption.
        const std::string cache_dir = "/tmp/nougat-v30-cache-" +
            std::to_string(static_cast<long long>(getpid()));
        reddmedia::LibraryMetadataCache cache(cache_dir);
        reddmedia::LibraryNode cached;
        cached.kind = reddmedia::LibraryNodeKind::Episode;
        cached.id = "episode-id";
        cached.parent_id = "season-id";
        cached.series_id = "series-id";
        cached.name = "Title\twith delimiter";
        cached.path = "/media/Show/Season 01/Show.S01E02.mkv";
        cached.overview = "Line one\nLine two";
        cached.series_name = "Show Name";
        cached.episode_title = "Purple Giraffe";
        cached.genres = {"Comedy", "Romance"};
        cached.primary_image_tag = "image-tag";
        cached.tmdb_poster_path = "/poster.jpg";
        cached.production_year = 2005;
        cached.season_number = 1;
        cached.episode_number = 2;
        std::string cache_error;
        const bool cache_store_ok = cache.store("children:season-id", {cached}, cache_error);
        std::vector<reddmedia::LibraryNode> loaded;
        const bool cache_load_ok = cache.load("children:season-id", loaded, cache_error);
        const bool cache_ok = cache_store_ok && cache_load_ok && loaded.size() == 1 &&
            loaded[0].name == cached.name && loaded[0].overview == cached.overview &&
            loaded[0].genres == cached.genres && loaded[0].season_number == 1 &&
            loaded[0].episode_number == 2;
        cache.remove("children:season-id", cache_error);
        rmdir(cache_dir.c_str());

        if (!grid_ok || !home_ratio_ok || !nav_middle_ok || !nav_first_ok || !nav_last_ok || !cache_ok) {
            std::fprintf(stderr,
                "Nougat v0.0.30 UI/Library/Player self-test FAIL. grid=%d home=%d nav=%d/%d/%d cache=%d\n",
                grid_ok, home_ratio_ok, nav_middle_ok, nav_first_ok, nav_last_ok, cache_ok);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.30 UI/Library/Player PASS: multi-row DVD grid, portrait Home posters, persistent cache, Previous/Next, and volume/panel polish contract.\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v31-ui-sheet-self-test") {
        App app;
        const bool palette_ok =
            app.quilt_view_index(ViewMode::Home) == 0 &&
            app.quilt_view_index(ViewMode::VideoPlayer) == 1 &&
            app.quilt_view_index(ViewMode::Library) == 2 &&
            app.quilt_view_index(ViewMode::Discover) == 3 &&
            app.quilt_view_index(ViewMode::Nougat) == 4 &&
            app.quilt_view_index(ViewMode::Stream) == 5 &&
            app.quilt_view_index(ViewMode::Debug) == 7;
        const bool geometry_ok = App::kCompactButtonW == 116 && App::kCompactButtonH == 26;
        const bool state_ok = static_cast<int>(App::SheetControlState::Normal) !=
                              static_cast<int>(App::SheetControlState::Pressed);
        const Rect tiny{0,0,32,26};
        const bool icon_ok = tiny.w == 32 && tiny.h == App::kCompactButtonH;
        if (!palette_ok || !geometry_ok || !state_ok || !icon_ok) {
            std::fprintf(stderr,
                "Nougat v0.0.31 exact UI-sheet component self-test FAIL. palette=%d geometry=%d state=%d icon=%d\n",
                palette_ok, geometry_ok, state_ok, icon_ok);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.31 exact UI-sheet component PASS: page palettes retained; sheet buttons, tabs, fields, panels, tracks, knobs and checkboxes active.\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--embedding-model-test") {
        reddmedia::EmbeddingEngine engine(
            exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf");
        std::vector<float> embedding;
        std::string error;
        if (!engine.embed_document("Nougat Media Suite offline embedding validation", embedding, error) ||
            embedding.empty()) {
            std::fprintf(stderr, "Nougat Media Suite embedding model FAIL: %s\n", error.c_str());
            return 1;
        }
        std::printf("Nougat Media Suite embedding model PASS: %zu dimensions%s.\n",
                    embedding.size(), engine.using_real_model() ? " (llama.cpp)" : " (test stub)");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--discover-ai-self-test") {
        const std::string history_path = "/tmp/reddmedia-v18-discover-" +
            std::to_string(static_cast<long long>(getpid())) + ".sqlite3";
        unlink(history_path.c_str());
        reddmedia::RecommendationEngine engine(
            exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf",
            history_path);
        reddmedia::MediaDescriptor watched_movie;
        watched_movie.id = "watched-movie";
        watched_movie.title = "A Space Voyage";
        watched_movie.overview = "Explorers travel through deep space.";
        watched_movie.media_type = reddmedia::RecommendationMediaType::Movie;
        reddmedia::MediaDescriptor watched_tv = watched_movie;
        watched_tv.id = "watched-tv";
        watched_tv.title = "Space Station Crew";
        watched_tv.media_type = reddmedia::RecommendationMediaType::Television;
        std::string error;
        if (!engine.record_started(watched_movie, error) ||
            !engine.record_started(watched_tv, error)) {
            std::fprintf(stderr, "Nougat Media Suite Discover AI FAIL: %s\n", error.c_str());
            unlink(history_path.c_str());
            return 1;
        }
        std::vector<reddmedia::MediaDescriptor> local_items;
        reddmedia::MediaDescriptor movie = watched_movie;
        movie.id = "local-movie";
        movie.title = "Galaxy Travelers";
        movie.local_path = "/validation/local-movie.mkv";
        local_items.push_back(movie);
        reddmedia::MediaDescriptor television = watched_tv;
        television.id = "local-tv";
        television.title = "Orbital Crew";
        local_items.push_back(television);
        for (const reddmedia::RecommendationMode mode : {
                 reddmedia::RecommendationMode::Usual,
                 reddmedia::RecommendationMode::Random}) {
            for (const reddmedia::RecommendationMediaType type : {
                     reddmedia::RecommendationMediaType::Movie,
                     reddmedia::RecommendationMediaType::Television}) {
                reddmedia::RecommendationRequest request;
                request.source = reddmedia::RecommendationSource::Local;
                request.media_type = type;
                request.mode = mode;
                reddmedia::RecommendationResult result;
                if (!engine.recommend(request, local_items, result, error) || result.item.id.empty()) {
                    std::fprintf(stderr, "Nougat Media Suite Discover AI FAIL: %s\n", error.c_str());
                    unlink(history_path.c_str());
                    return 1;
                }
            }
        }
        unlink(history_path.c_str());
        std::printf("Nougat Media Suite Discover AI PASS: Local Usual/Random Movie/TV.\n");
        return 0;
    }
    if (argc > 1 &&
        (std::string(argv[1]) == "--media-server-lifecycle-test" ||
         std::string(argv[1]) == "--media-server-parent-death-hold")) {
        reddmedia::MediaServerManager server;
        server.start();
        bool ready = false;
        for (int attempt = 0; attempt < 600; ++attempt) {
            server.poll();
            if (server.state() == reddmedia::MediaServerState::Ready) {
                ready = true;
                break;
            }
            usleep(200000);
        }
        if (!ready) {
            std::fprintf(stderr, "Nougat Media Suite integrated server lifecycle FAIL: startup timeout.\n");
            server.stop();
            return 1;
        }
        if (std::string(argv[1]) == "--media-server-parent-death-hold") {
            std::printf("Nougat Media Suite integrated server parent-death test READY.\n");
            std::fflush(stdout);
            while (true) pause();
        }
        server.stop();
        if (server.state() != reddmedia::MediaServerState::Stopped || server.probe_health() || server.owns_server()) {
            std::fprintf(stderr, "Nougat Media Suite integrated server lifecycle FAIL: owned process tree or port 8096 survived explicit stop.\n");
            return 1;
        }
        std::printf("Nougat Media Suite integrated server graceful shutdown PASS: owned process tree stopped and port 8096 released.\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v32-p2p-player-repair-self-test") {
        App app;
        const bool search_palette = true; // seam color is source-regression tested without an X11 display
        app.W = 960; app.H = 720; app.layout();
        const int knobD = 26;
        const bool volume_geometry = app.volumeHousingRect.x < app.volRect.x &&
            app.volumeHousingRect.y < app.volRect.y &&
            app.volumeHousingRect.x + app.volumeHousingRect.w > app.volRect.x + app.volRect.w &&
            app.volumeHousingRect.y + app.volumeHousingRect.h > app.volRect.y + app.volRect.h &&
            app.volRect.h > 0 && app.seekRect.h > app.volRect.h &&
            app.volRect.w < app.seekRect.w &&
            knobD == 26;
        const bool p2p_controls = app.p2pRemoveBtn.w > 0 && app.p2pPlayBtn.w > 0 && app.p2pStopResumeBtn.w > 0;
        if (!search_palette || !volume_geometry || !p2p_controls) {
            std::fprintf(stderr,"Nougat v0.0.32 P2P/player repair self-test FAIL. search=%d volume=%d p2p=%d\n",
                         search_palette?1:0, volume_geometry?1:0, p2p_controls?1:0);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.32 P2P/player repair PASS: Search seam contrast, seek-style volume geometry, Home fixed-header clipping/scrollbar contract, P2P controls and streaming scheduler contract active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--v33-integration-self-test") {
        App app;
        app.W = 720; app.H = 620; app.layout();
        const Rect homeFrame = app.page_content_frame(ViewMode::Home);
        const Rect playerFrame = app.page_content_frame(ViewMode::VideoPlayer);
        const bool navOrder = app.homeTab.x < app.videoPlayerTab.x && app.videoPlayerTab.x < app.libraryTab.x &&
            app.libraryTab.x < app.discoverTab.x && app.discoverTab.x < app.liveTvTab.x &&
            app.liveTvTab.x < app.nougatTab.x && app.nougatTab.x < app.ytdlpTab.x && app.ytdlpTab.x < app.debugTab.x;
        const bool navClip = app.topNavClipX > 0 && app.topNavClipRight < app.W && app.topNavClipRight > app.topNavClipX;
        const bool frames = homeFrame.x > 0 && homeFrame.y == App::kTopBarH && homeFrame.w < app.W &&
            playerFrame.x == 0 && playerFrame.y == App::kTopBarH && playerFrame.w == app.W;
        const bool libraryContainment = app.libraryListViewBtn.x > homeFrame.x &&
            app.libraryVerticalScrollTrack.x + app.libraryVerticalScrollTrack.w < app.W &&
            app.libraryListBox.x + app.libraryListBox.w < app.libraryVerticalScrollTrack.x;
        const bool liveTvLayout = app.liveTvTab.w > 0 && app.liveTvDetectBtn.w > 0 && app.liveTvListBox.w > 0;
        std::string plusError;
        const bool plus = app.p2p.set_speed_limits(5120, 1024, plusError) &&
            app.p2p.set_seed_rules(1.5, 120, plusError) &&
            app.p2p.plus_settings().download_limit_kib == 5120 &&
            app.p2p.plus_settings().upload_limit_kib == 1024 &&
            app.p2p.plus_settings().seed_ratio_limit == 1.5 &&
            app.p2p.plus_settings().seed_time_limit_minutes == 120;
        if (!navOrder || !navClip || !frames || !libraryContainment || !liveTvLayout || !plus) {
            std::fprintf(stderr, "Nougat v0.0.34 integration self-test FAIL. nav=%d clip=%d frames=%d library=%d live=%d p2p=%d\n",
                         navOrder?1:0, navClip?1:0, frames?1:0, libraryContainment?1:0, liveTvLayout?1:0, plus?1:0);
            return 1;
        }
        std::string tunerStatus;
        (void)app.tunerBackend.detect(tunerStatus);
        if (tunerStatus.empty()) {
            std::fprintf(stderr, "Nougat v0.0.34 tuner discovery self-test FAIL.\n");
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.34 integration PASS: clipped page/nav viewports, Library containment, P2P Plus controls, persistent-server architecture and Live TV discovery scaffold active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--v34-ui-polish-self-test") {
        App app;
        app.W = 1294; app.H = 704; app.layout();
        const int expectedLeft = app.top_nav_left_bound();
        const bool navLeft = app.homeTab.x == expectedLeft && app.topNavClipX == expectedLeft;
        const bool navRightPreserved = app.topNavClipRight == app.W - 154;
        const bool topTabs = app.homeTab.w == App::kTopTabW && app.homeTab.h == App::kTopTabH &&
            App::kTopTabH > App::kCompactButtonH &&
            app.videoPlayerTab.x - app.homeTab.x == App::kTopTabW + App::kTopTabGap &&
            App::kTopTabW > App::kTopTabH * 2;
        const bool sheetPlayerControls = app.seekRect.h == 20 && app.volRect.h == 16 &&
            app.volumeHousingRect.w == App::kSheetVolumeW && app.volumeHousingRect.h == App::kSheetVolumeH;
        const bool liveHeaderClear = app.liveTvDetectBtn.y == App::kPageControlY && app.liveTvListBox.y > app.liveTvDetectBtn.y + app.liveTvDetectBtn.h;
        const bool discoverLive = app.discoverLiveTvBtn.w == App::kCompactButtonW &&
            app.discoverLocalTvBtn.x < app.discoverLiveTvBtn.x && app.discoverLiveTvBtn.x < app.discoverExternalMovieBtn.x;
        const int cw = 190;
        const bool homeGeometry = app.home_card_height(false,cw) == std::max(168,cw*3/2)+50 &&
            app.home_card_height(true,cw) == app.home_card_height(false,cw);
        const bool frames = app.page_uses_connected_square_frame(ViewMode::Home) &&
            app.page_uses_connected_square_frame(ViewMode::Library) &&
            app.page_uses_connected_square_frame(ViewMode::Discover) &&
            app.page_uses_connected_square_frame(ViewMode::LiveTV) &&
            app.page_uses_connected_square_frame(ViewMode::Nougat) &&
            app.page_uses_connected_square_frame(ViewMode::Stream) &&
            app.page_uses_connected_square_frame(ViewMode::Studio) &&
            app.page_uses_connected_square_frame(ViewMode::Debug) &&
            !app.page_uses_connected_square_frame(ViewMode::VideoPlayer);
        if (!navLeft || !navRightPreserved || !topTabs || !sheetPlayerControls || !liveHeaderClear || !discoverLive || !homeGeometry || !frames) {
            std::fprintf(stderr,"Nougat v0.0.34 UI polish self-test FAIL. navL=%d navR=%d tabs=%d player=%d live=%d discover=%d home=%d frames=%d\n",
                navLeft?1:0,navRightPreserved?1:0,topTabs?1:0,sheetPlayerControls?1:0,liveHeaderClear?1:0,discoverLive?1:0,homeGeometry?1:0,frames?1:0);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.34 UI polish PASS: actual-sheet top tabs/seek/volume, left-shifted nav, direct scroll dragging, connected affected-page corners, Live TV header spacing, Discover Live TV/TMDb labels, and fixed Home card geometry active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--v35-cleanup-self-test") {
        App app;
        app.W = 640; app.H = 650; app.layout();
        const bool studioOrder = app.ytdlpTab.x < app.studioTab.x && app.studioTab.x < app.debugTab.x;
        const bool squareSearch = app.page_uses_connected_square_frame(ViewMode::Nougat);
        const bool alignedRows = app.streamYoutubeTab.y == App::kPageControlY &&
            app.nougatSearchPanelTab.y == App::kPageControlY &&
            app.liveTvDetectBtn.y == App::kPageControlY && app.debugRunBtn.y == App::kPageControlY &&
            app.discoverUsualTab.y == App::kPageControlY && app.libraryMoviesBtn.y == App::kPageControlY &&
            app.libraryListViewBtn.y == App::kPageControlY && app.libraryGridBtn.y == App::kPageControlY;
        const bool libraryViewsRight = app.libraryGridBtn.x + app.libraryGridBtn.w == app.page_content_frame(ViewMode::Library).x +
            app.page_content_frame(ViewMode::Library).w - 16 && app.libraryListViewBtn.x < app.libraryGridBtn.x;
        const bool playerSheet = app.seekRect.h == 20 && app.volRect.h == 16 &&
            app.volumeHousingRect.w == App::kSheetVolumeW && app.volumeHousingRect.h == App::kSheetVolumeH &&
            app.volRect.x == app.volumeHousingRect.x + 50 && app.volRect.w == 229 &&
            app.seekRect.w > 0;
        app.debugButtonsScrollX = 100000;
        app.controlsScrollX = 100000;
        app.libraryButtonsScrollX = 100000;
        app.layout();
        const bool debugScrollReach = app.debugBundleBtn.x + app.debugBundleBtn.w <= app.W - 28 &&
            app.debugBundleBtn.x >= 28;
        const bool playerScrollReach = app.fsBtn.x + app.fsBtn.w <= app.W - 10 && app.fsBtn.x >= 10;
        const int libraryToolRight = app.libraryListViewBtn.x - 8;
        const bool librarySingleRowReach = app.libraryBackBtn.y == App::kPageControlY &&
            app.libraryBackBtn.x + app.libraryBackBtn.w <= libraryToolRight &&
            app.libraryBackBtn.x >= app.page_content_frame(ViewMode::Library).x + 16;
        const bool systemServerControls = app.serverStartBtn.y == App::kPageControlY &&
            app.serverStopBtn.y == App::kPageControlY && app.serverRefreshBtn.y == App::kPageControlY;
        const bool activePointerScale = App::kTopTabPointerHalfW >= 16 && App::kTopTabPointerH >= 10;
        const bool studioPalette = app.quilt_view_index(ViewMode::Studio) == 9;
        App wideApp;
        wideApp.W = 1300; wideApp.H = 760; wideApp.layout();
        const int wideControlRight = wideApp.fsBtn.x + wideApp.fsBtn.w;
        const bool playerControlsCentered = std::abs(wideApp.openBtn.x - (wideApp.W - wideControlRight)) <= 1;
        reddmedia::TunerDevice invalidTuner;
        invalidTuner.video_path = "/dev/video0";
        invalidTuner.readable = true;
        std::string tunerStatus;
        const bool scanGuard = !app.tunerBackend.begin_channel_scan(invalidTuner, tunerStatus) &&
            tunerStatus.find("DVB frontend") != std::string::npos;
        if (!studioOrder || !squareSearch || !alignedRows || !libraryViewsRight || !playerSheet ||
            !debugScrollReach || !playerScrollReach || !librarySingleRowReach || !systemServerControls || !activePointerScale ||
            !studioPalette || !playerControlsCentered || !scanGuard) {
            std::fprintf(stderr,
                "Nougat v0.0.35 cleanup self-test FAIL. studio=%d search=%d rows=%d libview=%d player=%d debug=%d pscroll=%d librow=%d pointer=%d palette=%d centered=%d scan=%d\n",
                studioOrder?1:0, squareSearch?1:0, alignedRows?1:0, libraryViewsRight?1:0, playerSheet?1:0,
                debugScrollReach?1:0, playerScrollReach?1:0, librarySingleRowReach?1:0, activePointerScale?1:0,
                studioPalette?1:0, playerControlsCentered?1:0, scanGuard?1:0);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.35 cleanup PASS: app-wide top-row alignment, centered full-width player controls, one-row Library scrolling, exact-sheet VOLUME sprite geometry, full Debug/player action scrolling, enlarged active-tab pointer, Gold Studio identity, and native ATSC scan guard active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--v36-library-ui-player-self-test") {
        App app;
        app.W = 640; app.H = 650; app.layout();
        const Rect libraryFrame = app.page_content_frame(ViewMode::Library);
        const bool searchBelowButtons = app.librarySearchRect.y >= App::kPageControlBottom + 8 &&
            app.librarySearchRect.x == libraryFrame.x + 16 &&
            app.librarySearchBtn.y == app.librarySearchRect.y &&
            app.librarySearchBtn.x == app.librarySearchRect.x + app.librarySearchRect.w + 8 &&
            app.librarySearchBtn.x + app.librarySearchBtn.w == libraryFrame.x + libraryFrame.w - 16 &&
            app.libraryListBox.y > app.librarySearchRect.y + app.librarySearchRect.h;
        const bool viewPairStillRight = app.libraryGridBtn.x + app.libraryGridBtn.w == libraryFrame.x + libraryFrame.w - 16 &&
            app.libraryListViewBtn.x < app.libraryGridBtn.x;
        const bool seekNative = app.seekRect.w >= App::kSheetSeekW && app.seekRect.h == App::kSheetSeekH &&
            app.seekRect.x > 0 && app.seekRect.x + app.seekRect.w < app.W && App::kSheetSeekSpriteH == 33 && App::kSheetSeekFrames == 101;
        const int seekSpriteTop = app.seekRect.y - (App::kSheetSeekSpriteH - App::kSheetSeekH) / 2;
        const bool playerStack = seekSpriteTop >= App::kTopBarH &&
            seekSpriteTop + App::kSheetSeekSpriteH < app.volumeHousingRect.y - 4 &&
            app.volumeHousingRect.y + app.volumeHousingRect.h <= app.openBtn.y - 4;
        const bool headerStatus = App::kServerStatusDiameter >= 16;
        {
            std::lock_guard<std::mutex> lock(app.libraryState->mutex);
            reddmedia::LibraryNode alien; alien.id="m1"; alien.name="Alien"; alien.kind=reddmedia::LibraryNodeKind::Movie;
            reddmedia::LibraryNode heat; heat.id="m2"; heat.name="Heat"; heat.kind=reddmedia::LibraryNodeKind::Movie;
            app.libraryState->nodes={alien,heat};
        }
        app.librarySearchQuery="ALI";
        const std::vector<int> filtered=app.library_visible_indices();
        const bool searchFilter = filtered.size() == 1 && filtered[0] == 0;
        if (!searchBelowButtons || !viewPairStillRight || !seekNative || !playerStack || !headerStatus || !searchFilter) {
            std::fprintf(stderr,
                "Nougat v0.0.36 Library/UI/player self-test FAIL. searchrow=%d viewright=%d seek=%d stack=%d status=%d filter=%d\n",
                searchBelowButtons?1:0, viewPairStillRight?1:0, seekNative?1:0, playerStack?1:0,
                headerStatus?1:0, searchFilter?1:0);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.36 PASS: Library Search row/filter, collection-ready navigation geometry, exact-sheet seek sprite geometry with side times, stable player stack, and sheet-style server status geometry active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--v37-live-tv-system-self-test") {
        App app;
        app.W=998; app.H=700; app.layout();
        app.currentView=ViewMode::Debug;
        const bool systemName=app.current_view_name()=="System";
        const bool systemServer=app.serverStartBtn.y==App::kPageControlY && app.serverStopBtn.y==App::kPageControlY &&
            app.serverRefreshBtn.y==App::kPageControlY;
        const bool homeSame=app.home_card_height(true,180)==app.home_card_height(false,180) &&
            app.home_card_artwork_height(true,180)==app.home_card_artwork_height(false,180);
        const bool seekWide=app.seekRect.w>App::kSheetSeekW && app.seekRect.x>=80 &&
            app.seekRect.x+app.seekRect.w<=app.W-80;
        const bool statusButton=App::kServerStatusDiameter==20;
        const bool liveButtons=app.liveTvChannelsBtn.y==App::kPageControlY && app.liveTvGuideBtn.y==App::kPageControlY &&
            app.liveTvGuideRefreshBtn.y==App::kPageControlY && app.liveTvNowBtn.y==App::kPageControlY;
        reddmedia::TunerDevice fakeTuner; fakeTuner.frontend_path="/dev/dvb/adapter2/frontend0"; fakeTuner.readable=true;
        reddmedia::LiveTvChannel fakeChannel; fakeChannel.id="3.1"; fakeChannel.name="KSNW-DT";
        fakeChannel.frequency="479000000"; fakeChannel.program_number=3; fakeChannel.physical_channel=15;
        std::string mrl,status; std::vector<std::string> options;
        const bool liveInput=app.tunerBackend.live_playback_input(fakeTuner,fakeChannel,mrl,options,status) && mrl=="atsc://" &&
            std::find(options.begin(),options.end(),":dvb-adapter=2")!=options.end() &&
            std::find(options.begin(),options.end(),":program=3")!=options.end();
        const bool guideModel=app.liveTvTunerUse==LiveTvTunerUse::Idle && !app.liveTvGuideMode;
        const Rect libraryFrame=app.page_content_frame(ViewMode::Library);
        const bool librarySearchButton=app.librarySearchRect.y==app.librarySearchBtn.y &&
            app.librarySearchBtn.x==app.librarySearchRect.x+app.librarySearchRect.w+8 &&
            app.librarySearchBtn.x+app.librarySearchBtn.w==libraryFrame.x+libraryFrame.w-16;
        {
            std::lock_guard<std::mutex> lock(app.libraryState->mutex);
            reddmedia::LibraryNode one; one.id="one"; one.name="The Search Test"; one.kind=reddmedia::LibraryNodeKind::Movie;
            reddmedia::LibraryNode two; two.id="two"; two.name="Something Else"; two.kind=reddmedia::LibraryNodeKind::Movie;
            app.libraryState->nodes={one,two};
        }
        app.librarySearchQuery="search test";
        const auto searchVisible=app.library_visible_indices();
        const bool librarySearchWorks=searchVisible.size()==1 && searchVisible[0]==0;
        std::vector<reddmedia::LiveTvProgram> testPrograms(1);
        testPrograms[0].channel_id="3.1"; testPrograms[0].title="Guide Test"; testPrograms[0].description="Description";
        testPrograms[0].start_unix=1700000000LL; testPrograms[0].duration_seconds=1800; testPrograms[0].event_id=77;
        std::string guideError;
        const bool guideSave=app.tunerBackend.save_guide(testPrograms,guideError);
        const auto guideReload=app.tunerBackend.load_guide();
        const bool guideCache=guideSave && guideReload.size()==1 && guideReload[0].channel_id=="3.1" &&
            guideReload[0].title=="Guide Test" && guideReload[0].duration_seconds==1800 && guideReload[0].event_id==77;
        if (!systemName || !systemServer || !homeSame || !seekWide || !statusButton || !liveButtons || !liveInput || !guideModel ||
            !librarySearchButton || !librarySearchWorks || !guideCache) {
            std::fprintf(stderr,"Nougat v0.0.37 self-test FAIL. system=%d server=%d home=%d seek=%d status=%d livebuttons=%d input=%d guide=%d searchbtn=%d search=%d cache=%d\n",
                systemName?1:0,systemServer?1:0,homeSame?1:0,seekWide?1:0,statusButton?1:0,liveButtons?1:0,liveInput?1:0,guideModel?1:0,
                librarySearchButton?1:0,librarySearchWorks?1:0,guideCache?1:0);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.37 PASS: System controls, working Library Search+button, full-size Continue Watching, wide exact-sheet seek, stitched Server state, logical Live TV controls, native ATSC Watch Live input, and persisted guide model active.\n");
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "--p2p-engine-info") {
        P2PEngine engine;
        printf("%s\n", engine.libtorrent_version().c_str());
        engine.shutdown();
        return 0;
    }
    if (argc > 2 && std::string(argv[1]) == "--media-library-api-test") {
        reddmedia::JellyfinApiClient client;
        std::string error;
        if (!client.initialize(error) || !client.add_media_folder(argv[2], error)) {
            fprintf(stderr, "Nougat Media Suite native library API FAIL: %s\n", error.c_str());
            return 1;
        }
        std::vector<reddmedia::LibraryVideo> videos;
        if (!client.wait_for_video_in_folder(argv[2], videos, error, 180)) {
            fprintf(stderr, "Nougat Media Suite native library API FAIL: %s\n", error.c_str());
            return 1;
        }
        bool found = false;
        for (const reddmedia::LibraryVideo& video : videos) {
            if (video.path.find(argv[2]) == 0U) {
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Nougat Media Suite native library API FAIL: indexed test video not found.\n");
            return 1;
        }
        printf("Nougat Media Suite native library API PASS: %zu video(s) cataloged for direct playback.\n", videos.size());
        return 0;
    }
    App app;
    if (!app.init()) return 1;
    app.event_loop();
    app.shutdown();
    return 0;
}
