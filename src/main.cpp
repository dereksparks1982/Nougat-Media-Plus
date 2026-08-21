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
#include "media_server/jellyfin_api_client.hpp"
#include "media_server/library_poster.hpp"
#include "media_server/media_server_manager.hpp"
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
enum class ViewMode { VideoPlayer, Library, Discover, Nougat, Stream, P2P, Debug };
enum class NougatPanel { Search, Crawler, P2P };
enum class StreamPlatform { YouTube, Rumble, RuTube, VK, OK };
enum class LibraryDisplayMode { Grid, List };
enum class NougatInputFocus { NoFocus, Search, CrawlSeed, Peer };
enum class YtDlpJob { Idle, Download };
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
};
struct ServerUiState {
    std::mutex mutex;
    std::string status = "Server controls ready.";
    reddmedia::MediaServerState state = reddmedia::MediaServerState::Stopped;
    bool owned = false;
    bool busy = false;
    bool updated = false;
    double progress = 0.0;
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
class App {
public:
    Display* d=nullptr; int screen=0; Window win=0, video=0; GC gc=0; XFontStruct* fontInfo=nullptr;
    int W=1000,H=650;
    int videoW=980, videoH=530;
    Rect openBtn, rewindBtn, playBtn, stopBtn, forwardBtn, fsBtn, seekRect, volRect, resumeBtn, loadBtn;
    Rect videoResumeBtn, videoLoadBtn;
    Rect videoPlayerTab, libraryTab, discoverTab, nougatTab, ytdlpTab, debugTab;
    Rect libraryMoviesBtn, libraryTvBtn, libraryGridBtn, libraryListViewBtn, libraryAddFolderBtn, libraryUnlinkFolderBtn;
    Rect libraryRefreshBtn, libraryBackBtn, libraryListBox;
    Rect serverStartBtn, serverStopBtn, serverRefreshBtn;
    Rect discoverUsualTab, discoverRandomTab;
    Rect discoverLocalMovieBtn, discoverLocalTvBtn, discoverExternalMovieBtn, discoverExternalTvBtn;
    Rect discoverTmdbTestBtn, discoverTmdbReplaceBtn, discoverTmdbClearBtn;
    Rect discoverResultBox, discoverOpenBtn, discoverWatchBtn, discoverMyServicesBtn;
    Rect discoverServicesBackBtn;
    Rect debugRunBtn, debugRetryBtn, debugMetadataBtn, debugTmdbBtn;
    Rect debugServerBtn, debugLogsBtn, debugCopyBtn, debugListBox;
    Rect streamYoutubeTab, streamRumbleTab, streamRutubeTab, streamVkTab, streamOkTab;
    Rect ytdlpUrlRect, ytdlpOutputRect, ytdlpDownloadBtn, ytdlpPlayBtn, ytdlpDirectWatchBtn, ytdlpWebpageBtn, ytdlpClearBtn, ytdlpFolderBtn;
    Rect p2pMagnetRect, p2pOutputRect, p2pLoadMagnetBtn, p2pOpenTorrentBtn, p2pPlayBtn, p2pStopResumeBtn;
    Rect nougatSearchPanelTab, nougatCrawlerPanelTab, nougatP2PPanelTab, nougatNetworkAdvancedBtn;
    Rect nougatSearchRect, nougatSearchBtn, nougatRawBtn, nougatPeersToggleBtn, nougatResultsBox;
    Rect nougatCrawlSeedRect, nougatCrawlMinusBtn, nougatCrawlPlusBtn, nougatSameDomainBtn, nougatStartCrawlBtn, nougatCrawlLogBox;
    Rect nougatPeerEntryRect, nougatAddPeerBtn, nougatRemovePeerBtn, nougatNodeBtn, nougatPeerListBox;
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
    long long sessionTime=0;
    Cursor blankCursor=0, normalCursor=0;
    bool pointerInVideo=false, pointerHidden=false;
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
    ViewMode currentView = ViewMode::VideoPlayer;
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
    int libraryButtonsScrollX = 0;
    int discoverButtonsScrollX = 0;
    int ytdlpButtonsScrollX = 0;
    int streamSourceScrollX = 0;
    int p2pButtonsScrollX = 0;
    int debugButtonsScrollX = 0;
    int volumePercent = 100;
    bool volumeDragging = false;
    StreamPlatform streamPlatform = StreamPlatform::YouTube;
    LibraryDisplayMode libraryMovieView = LibraryDisplayMode::Grid;
    LibraryDisplayMode libraryTvView = LibraryDisplayMode::Grid;
    bool tvAutoplayArmed = false;
    bool playbackEndHandled = false;
    std::vector<reddmedia::LibraryNode> tvAutoplayQueue;
    int tvAutoplayIndex = -1;
    reddmedia::LibraryNode activeLibraryItem;
    bool activeLibraryItemValid = false;
    P2PEngine p2p;
    P2PStreamServer p2pStream{p2p};
    reddmedia::MediaServerManager mediaServer;
    std::shared_ptr<reddmedia::JellyfinApiClient> libraryClient =
        std::make_shared<reddmedia::JellyfinApiClient>();
    std::shared_ptr<LibraryUiState> libraryState = std::make_shared<LibraryUiState>();
    std::thread libraryWorker;
    std::vector<Rect> libraryRows;
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
    void text(Drawable target, int x, int y, const std::string& s, unsigned long c) {
        XSetForeground(d, gc, c); XDrawString(d, target, gc, x, y, s.c_str(), (int)s.size());
    }
    int text_width(const std::string& s) {
        if (s.empty()) return 0;
        if (fontInfo) return XTextWidth(fontInfo, s.c_str(), (int)s.size());
        return (int)s.size() * 8;
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
        switch (platform) {
            case StreamPlatform::YouTube:
                return {col(0x0f0f,0x0f0f,0x0f0f), col(0x2121,0x2121,0x2121), col(0xf1f1,0xf1f1,0xf1f1),
                        col(0x5a5a,0x5a5a,0x5a5a), col(0xffff,0xffff,0xffff), col(0xaaaa,0xaaaa,0xaaaa),
                        col(0xffff,0x0000,0x0000), col(0xffff,0x0000,0x0000), col(0x8f8f,0x0000,0x0000),
                        col(0xffff,0x4d4d,0x4d4d), col(0xffff,0xffff,0xffff), col(0xffff,0x0000,0x0000)};
            case StreamPlatform::Rumble:
                return {col(0x0d0d,0x1515,0x0e0e), col(0x1929,0x261a,0x1b1b), col(0xf2f2,0xf7f7,0xefef),
                        col(0x5f5f,0x7c7c,0x4d4d), col(0xf8f8,0xffff,0xf4f4), col(0xbdbd,0xd0d0,0xb5b5),
                        col(0x8585,0xc7c7,0x4242), col(0x8585,0xc7c7,0x4242), col(0x4c4c,0x7a7a,0x2121),
                        col(0xa9a9,0xe8e8,0x6c6c), col(0x1111,0x1818,0x1010), col(0x8585,0xc7c7,0x4242)};
            case StreamPlatform::RuTube:
                return {col(0x1010,0x0909,0x4343), col(0x1c1c,0x1414,0x5a5a), col(0xf4f4,0xf1f1,0xffff),
                        col(0x9a9a,0x1313,0xeded), col(0xffff,0xffff,0xffff), col(0xbdbd,0xb4b4,0xdada),
                        col(0x1212,0x3a3a,0xeded), col(0xeded,0x1414,0x3b3b), col(0x9292,0x0909,0x2525),
                        col(0xffff,0x5b5b,0x7474), col(0xffff,0xffff,0xffff), col(0x1212,0xcccc,0xeded)};
            case StreamPlatform::VK:
                return {col(0x0b0b,0x1818,0x2727), col(0x1414,0x2a2a,0x4545), col(0xf0f0,0xf6f6,0xffff),
                        col(0x0077,0x7777,0xffff), col(0xffff,0xffff,0xffff), col(0xaaaa,0xc2c2,0xdddd),
                        col(0x0077,0x7777,0xffff), col(0x0077,0x7777,0xffff), col(0x004f,0x4f4f,0xa8a8),
                        col(0x4f4f,0xa3a3,0xffff), col(0xffff,0xffff,0xffff), col(0x0000,0xeaea,0xffff)};
            case StreamPlatform::OK:
                return {col(0x2323,0x1717,0x0e0e), col(0x3a3a,0x2424,0x1414), col(0xffff,0xf4f4,0xe8e8),
                        col(0xeeee,0x8282,0x0808), col(0xffff,0xf7f7,0xf0f0), col(0xe3e3,0xb8b8,0x8787),
                        col(0xeeee,0x8282,0x0808), col(0xeeee,0x8282,0x0808), col(0xa6a6,0x4d4d,0x0000),
                        col(0xffff,0xaaaa,0x4343), col(0x1e1e,0x1010,0x0606), col(0xffff,0x9d9d,0x2424)};
        }
        return {col(0x2418,0x1818,0x1212), col(0x4930,0x3030,0x2525), col(0xf3e5,0xe5e5,0xcccc),
                col(0xd2a5,0xa5a5,0x6d6d), col(0xf3f3,0xe5e5,0xcccc), col(0xcdcd,0xb9b9,0x9e9e),
                col(0xb96f,0x6f6f,0x3636), col(0xb96f,0x6f6f,0x3636), col(0x6b35,0x3535,0x1a1a),
                col(0xe0a0,0xa0a0,0x5656), col(0xffff,0xffff,0xffff), col(0xd2a5,0xa5a5,0x6d6d)};
    }

    ViewPalette palette_for(ViewMode view) {
        if (view == ViewMode::VideoPlayer) return {
            col(0x1b1b,0x0f0f,0x0808), col(0x341a,0x1a1a,0x0d0d), col(0xf5e6,0xe6e6,0xc8c8),
            col(0x9a5a,0x5a5a,0x2626), col(0xf8eb,0xebeb,0xd3d3), col(0xd3b9,0xb9b9,0x8c8c),
            col(0x7b3f,0x3f3f,0x1717), col(0x6b35,0x3535,0x1a1a), col(0x3b1c,0x1c1c,0x0d0d),
            col(0xa965,0x6565,0x3232), col(0xffff,0xf5f5,0xe3e3), col(0xd2a5,0xa5a5,0x6d6d)};
        if (view == ViewMode::Library) return {
            col(0x101c,0x1c1c,0x1313), col(0x1b32,0x3232,0x2222), col(0xeaf2,0xf2f2,0xe4e4),
            col(0x4f7c,0x7c7c,0x5858), col(0xf2f7,0xf7f7,0xeeee), col(0xb8cc,0xcccc,0xb8b8),
            col(0x597f,0x7f7f,0x5f5f), col(0x3474,0x7474,0x4242), col(0x1942,0x4242,0x2424),
            col(0x6ca2,0xa2a2,0x7777), col(0xffff,0xffff,0xffff), col(0x8faa,0xaaaa,0x7777)};
        if (view == ViewMode::Discover) return {
            col(0x1d10,0x1010,0x2424), col(0x3420,0x2020,0x4242), col(0xf2eb,0xebeb,0xf5f5),
            col(0x8053,0x5353,0x9292), col(0xf7ed,0xeded,0xfafa), col(0xd2bd,0xbdbd,0xdbdb),
            col(0x7652,0x5252,0x8989), col(0x7040,0x4040,0x8282), col(0x4020,0x2020,0x4e4e),
            col(0xa069,0x6969,0xb3b3), col(0xffff,0xffff,0xffff), col(0xbd7a,0x7a7a,0xd1d1)};
        if (view == ViewMode::Stream) return stream_palette_for(streamPlatform);
        if (view == ViewMode::P2P) return {
            col(0x0915,0x1515,0x2525), col(0x142c,0x2c2c,0x4848), col(0xe9f1,0xf1f1,0xf9f9),
            col(0x4a82,0x8282,0xacac), col(0xf2f8,0xf8f8,0xffff), col(0xb8cf,0xcfcf,0xe4e4),
            col(0x315f,0x5f5f,0x8d8d), col(0x2d62,0x6262,0x9292), col(0x1638,0x3838,0x5858),
            col(0x5e91,0x9191,0xbdbd), col(0xffff,0xffff,0xffff), col(0x60a7,0xa7a7,0xd7d7)};
        if (view == ViewMode::Debug) return {
            col(0x201a,0x1a1a,0x1111), col(0x3a2f,0x2f2f,0x1818), col(0xf6ed,0xeded,0xd8d8),
            col(0xa87a,0x7a7a,0x2828), col(0xfff6,0xf6f6,0xdbdb), col(0xd8c3,0xc3c3,0x9090),
            col(0x8c68,0x6868,0x2424), col(0xc68a,0x8a8a,0x0909), col(0x7450,0x5050,0x0000),
            col(0xf1b7,0xb7b7,0x3737), col(0x1711,0x1111,0x0707), col(0xf0b4,0xb4b4,0x2b2b)};
        return {col(0x2418,0x1818,0x1212), col(0x4930,0x3030,0x2525), col(0xf3e5,0xe5e5,0xcccc),
                col(0xd2a5,0xa5a5,0x6d6d), col(0xf3f3,0xe5e5,0xcccc), col(0xcdcd,0xb9b9,0x9e9e),
                col(0xb96f,0x6f6f,0x3636), col(0xb96f,0x6f6f,0x3636), col(0x6b35,0x3535,0x1a1a),
                col(0xe0a0,0xa0a0,0x5656), col(0xffff,0xffff,0xffff), col(0xd2a5,0xa5a5,0x6d6d)};
    }

    void button_on(Drawable target, const Rect& r, const std::string& label) {
        const ViewPalette palette = palette_for(currentView);
        fill(target, r, palette.button);
        outline(target, r, palette.buttonDark);
        line(target, r.x + 1, r.y + 1, r.x + (int)r.w - 2, r.y + 1, palette.buttonLight);
        const int label_x = r.x + std::max(5, (r.w - text_width(label)) / 2);
        text(target, label_x, r.y + 18, head_to_width(label, r.w - 10), palette.buttonText);
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
        if (fontInfo) XSetFont(d, gc, fontInfo->fid);
        layout();
        video = XCreateSimpleWindow(d, win, 10, 42, W-20, H-120, 0, BlackPixel(d,screen), BlackPixel(d,screen));
        XSelectInput(d, video, ExposureMask|ButtonPressMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|KeyPressMask);
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
        mediaServer.start();
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            serverState->status = server_control_label();
            serverState->state = mediaServer.state();
            serverState->owned = mediaServer.owns_server();
        }
        return true;
    }
    static constexpr int kCompactButtonW = 116;
    static constexpr int kCompactButtonH = 26;

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

    void layout() {
        const int topStatusReserve = 178;
        topNavViewportW = std::max(kCompactButtonW, W - topStatusReserve);
        topNavScrollX = clamp_button_scroll(topNavScrollX, 6, topNavViewportW);
        int topX = -topNavScrollX;
        videoPlayerTab = {topX,0,kCompactButtonW,26}; topX += kCompactButtonW;
        libraryTab = {topX,0,kCompactButtonW,26}; topX += kCompactButtonW;
        discoverTab = {topX,0,kCompactButtonW,26}; topX += kCompactButtonW;
        nougatTab = {topX,0,kCompactButtonW,26}; topX += kCompactButtonW;
        ytdlpTab = {topX,0,kCompactButtonW,26}; topX += kCompactButtonW;
        debugTab = {topX,0,kCompactButtonW,26};

        const int bottomY = H - 32;
        const int volumeY = H - 64;
        const int seekY = H - 96;
        const int controlCount = 6;
        const int controlTotalW = controlCount * kCompactButtonW;
        const int controlViewportW = std::max(kCompactButtonW, W - 20);
        controlsScrollX = clamp_button_scroll(controlsScrollX, controlCount, controlViewportW);
        int x = controlTotalW <= controlViewportW ? std::max(10, (W - controlTotalW) / 2) : 10 - controlsScrollX;
        openBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        rewindBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        playBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        stopBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        forwardBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;
        fsBtn = {x, bottomY, kCompactButtonW, kCompactButtonH};

        const int currentTimeWidth = 64;
        const int totalTimeWidth = 74;
        const int seekX = 10 + currentTimeWidth + 10;
        const int seekRightPad = totalTimeWidth + 20;
        seekRect = {seekX, seekY, std::max(220, W-seekX-seekRightPad), 18};
        volRect = {110, volumeY, std::max(120, W-130), 18};

        const int promptX = std::max(20, W/2-kCompactButtonW);
        resumeBtn = {promptX, H/2+40, kCompactButtonW, kCompactButtonH};
        loadBtn = {promptX+kCompactButtonW, H/2+40, kCompactButtonW, kCompactButtonH};

        layout_button_row({&streamYoutubeTab,&streamRumbleTab,&streamRutubeTab,&streamVkTab,&streamOkTab},
                          54, streamSourceScrollX);
        ytdlpUrlRect = {28, 120, std::max(240, W-56), 28};
        ytdlpOutputRect = {28, 160, std::max(240, W-56), 28};
        layout_button_row({&ytdlpDownloadBtn,&ytdlpPlayBtn,&ytdlpDirectWatchBtn,&ytdlpWebpageBtn,&ytdlpClearBtn},
                          202, ytdlpButtonsScrollX);
        ytdlpFolderBtn = {0,0,0,0};

        p2pMagnetRect = {28, 148, std::max(240, W-56), 28};
        p2pOutputRect = {28, 188, std::max(240, W-56), 28};
        layout_button_row({&p2pLoadMagnetBtn,&p2pOpenTorrentBtn,&p2pPlayBtn,&p2pStopResumeBtn}, 230, p2pButtonsScrollX);

        nougatSearchPanelTab = {28,54,kCompactButtonW,kCompactButtonH};
        nougatCrawlerPanelTab = {28+kCompactButtonW,54,kCompactButtonW,kCompactButtonH};
        nougatP2PPanelTab = {28+kCompactButtonW*2,54,kCompactButtonW,kCompactButtonH};
        nougatNetworkAdvancedBtn = {std::max(28+kCompactButtonW*3+16, W-144),54,kCompactButtonW,kCompactButtonH};
        nougatSearchRect = {28, 104, std::max(220, W-400), 30};
        nougatRawBtn = {std::max(260, W-360),104,kCompactButtonW,kCompactButtonH};
        nougatPeersToggleBtn = {std::max(380, W-240),104,kCompactButtonW,kCompactButtonH};
        nougatSearchBtn = {std::max(500, W-120),104,kCompactButtonW,kCompactButtonH};
        nougatResultsBox = {28, 166, std::max(240, W-56), std::max(120, H-194)};
        nougatCrawlSeedRect = {28, 104, std::max(240, W-300), 30};
        nougatCrawlMinusBtn = {std::max(320, W-260),104,36,26};
        nougatCrawlPlusBtn = {std::max(364, W-216),104,36,26};
        nougatSameDomainBtn = {28, 146, kCompactButtonW, kCompactButtonH};
        nougatStartCrawlBtn = {28+kCompactButtonW, 146, kCompactButtonW, kCompactButtonH};
        nougatCrawlLogBox = {28, 190, std::max(240, W-56), std::max(120, H-218)};
        nougatPeerEntryRect = {28, 120, std::max(220, W-520), 30};
        nougatAddPeerBtn = {std::max(300, W-480),120,kCompactButtonW,kCompactButtonH};
        nougatRemovePeerBtn = {std::max(420, W-360),120,kCompactButtonW,kCompactButtonH};
        nougatNodeBtn = {std::max(540, W-240),120,kCompactButtonW,kCompactButtonH};
        nougatPeersToggleBtn = {std::max(660, W-120),120,kCompactButtonW,kCompactButtonH};
        nougatPeerListBox = {28, 170, std::max(240, W-56), std::max(120, H-198)};

        const std::string libraryHeading = libraryParents.empty() ? "MEDIA LIBRARY" : libraryParents.back().name;
        const int libraryViewX = std::min(std::max(150, 28 + text_width(libraryHeading) + 12), std::max(150, W - 82));
        libraryListViewBtn = {libraryViewX, 38, 32, kCompactButtonH};
        libraryGridBtn = {libraryViewX + 36, 38, 32, kCompactButtonH};
        layout_button_row({&libraryMoviesBtn,&libraryTvBtn,
                           &libraryAddFolderBtn,&libraryUnlinkFolderBtn,&libraryRefreshBtn,&libraryBackBtn,
                           &serverStartBtn,&serverStopBtn,&serverRefreshBtn},
                          76, libraryButtonsScrollX);
        libraryListBox = {28, 134, std::max(240, W-56), std::max(100, H-162)};

        layout_button_row({&discoverUsualTab,&discoverRandomTab,&discoverLocalMovieBtn,&discoverLocalTvBtn,
                           &discoverExternalMovieBtn,&discoverExternalTvBtn,&discoverTmdbTestBtn,
                           &discoverTmdbReplaceBtn,&discoverTmdbClearBtn,&discoverMyServicesBtn},
                          76, discoverButtonsScrollX);
        discoverResultBox = {28, 134, std::max(240, W-56), std::max(150, H-212)};
        discoverOpenBtn = {28, H-66, kCompactButtonW, kCompactButtonH};
        discoverWatchBtn = {28+kCompactButtonW, H-66, kCompactButtonW, kCompactButtonH};
        discoverServicesBackBtn = {28,76,kCompactButtonW,kCompactButtonH};

        layout_button_row({&debugRunBtn,&debugRetryBtn,&debugMetadataBtn,&debugTmdbBtn,&debugServerBtn,&debugLogsBtn,&debugCopyBtn},
                          76, debugButtonsScrollX);
        debugListBox = {28, 126, std::max(240, W-56), std::max(150, H-154)};
        update_video_prompt_layout();
    }
    void update_video_prompt_layout() {
        const int promptX = std::max(20, videoW/2-kCompactButtonW);
        const int promptY = std::max(88, videoH/2+36);
        videoResumeBtn = {promptX, promptY, kCompactButtonW, kCompactButtonH};
        videoLoadBtn = {promptX+kCompactButtonW, promptY, kCompactButtonW, kCompactButtonH};
    }
    void apply_video_layout() {
        if (!video) return;
        if (currentView == ViewMode::Library || currentView == ViewMode::Discover ||
            currentView == ViewMode::Nougat || currentView == ViewMode::Stream || currentView == ViewMode::P2P ||
            currentView == ViewMode::Debug) {
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
            videoH = std::max(100, H-170);
            XMoveResizeWindow(d, video, 10, 42, videoW, videoH);
        }
        update_video_prompt_layout();
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
    }
    void open_media(const std::string& path, long long seek=0) {
        if (!inst || !api.media_new_path) return;
        p2pStream.stop();
        cleanup_player();
        libvlc_media_t* media = api.media_new_path(inst, path.c_str());
        if (!media) return;
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, volumePercent);
        playbackEndHandled = false;
        currentPath = path; currentMediaIsP2P=false; currentMediaIsNetwork=false; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; chapterScanComplete=false;
        playbackCacheValid=false; cachedPlaybackTimeMs=0; cachedPlaybackLengthMs=0;
        api.play(mp);
        auto_load_subtitle_for_current_media();
        if (seek > 0) {
            pendingSeek = true;
            pendingSeekMs = seek;
            pendingSeekDeadline = time(nullptr) + 6;
        }
        redraw();
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
        activeLibraryItemValid=false;
        playbackEndHandled=false;
    }
    void do_open() {
        std::string p = choose_file_dialog();
        if (!p.empty()) { cancel_tv_autoplay(); open_media(p, 0); }
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
        } else {
            api.set_pause(mp, 0);
            paused = false;
        }
        redraw();
    }
    void stop_media() {
        cancel_tv_autoplay();
        if (currentMediaIsYtDlpStream) {
            cleanup_player();
            currentMediaIsNetwork=false;
            ytdlpStatus="Play stream stopped.";
            redraw();
            return;
        }
        if (mp) { api.stop(mp); paused=false; cachedPlaybackTimeMs=0; playbackCacheValid=true; redraw(); }
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
        if (hasMedia && !needResumePrompt && vlcErr.empty()) return;
        XClearWindow(d, video);
        if (!vlcErr.empty()) {
            text(video, 22, 40, vlcErr, col(0xffff,0xdddd,0xdddd));
            text(video, 22, 64, "On Ubuntu: install VLC, then reopen this executable.", col(0xdddd,0xdddd,0xdddd));
            return;
        }
        if (needResumePrompt) {
            std::string title = "Resume " + basename_only(sessionPath) + "?";
            text(video, 28, 44, title, col(0xeeee,0xeeee,0xeeee));
            text(video, 28, 68, "Last position: " + format_time(sessionTime), col(0xcccc,0xcccc,0xcccc));
            button_on(video, videoResumeBtn, "Resume");
            button_on(video, videoLoadBtn, "Load Different");
            return;
        }
        if (!hasMedia) {
            text(video, 28, 44, "Open a media file to start.", col(0xeeee,0xeeee,0xeeee));
            text(video, 28, 68, "File menu or Open button.", col(0xcccc,0xcccc,0xcccc));
        }
    }
    unsigned long tab_accent(ViewMode view) {
        if (view == ViewMode::VideoPlayer) return col(0xd2a5,0xa5a5,0x6d6d);
        if (view == ViewMode::Library) return col(0x8faa,0xaaaa,0x7777);
        if (view == ViewMode::Discover) return col(0xbd7a,0x7a7a,0xd1d1);
        if (view == ViewMode::Nougat) return col(0xd2a5,0xa5a5,0x6d6d);
        if (view == ViewMode::Stream) return stream_palette_for(streamPlatform).accent;
        if (view == ViewMode::P2P) return col(0x60a7,0xa7a7,0xd7d7);
        return col(0xf0b4,0xb4b4,0x2b2b);
    }

    void draw_top_bar(Drawable target) {
        const unsigned long suiteChocolate = col(0x4a4a,0x1f1f,0x0b0b);
        const unsigned long topText = col(0xf2f2,0xe6e6,0xc9c9);
        const unsigned long divider = col(0x2d2d,0x1111,0x0808);
        fill(target, {0,0,W,26}, suiteChocolate);

        XRectangle navClip{0,0,(unsigned short)std::max(1,topNavViewportW),26};
        XSetClipRectangles(d, gc, 0, 0, &navClip, 1, Unsorted);
        const auto draw_tab = [&](const Rect& tab, const char* label, ViewMode view) {
            const bool active = currentView == view;
            const ViewPalette tabPalette = palette_for(view);
            fill(target, tab, active ? tabPalette.buttonLight : tabPalette.button);
            if (active) fill(target, {tab.x, 23, tab.w, 3}, tab_accent(view));
            line(target, tab.x + tab.w - 1, 0, tab.x + tab.w - 1, 25, divider);
            text(target, tab.x + std::max(5,(tab.w-text_width(label))/2), 17, label, tabPalette.buttonText);
        };
        draw_tab(videoPlayerTab,"Video Player",ViewMode::VideoPlayer);
        draw_tab(libraryTab,"Library",ViewMode::Library);
        draw_tab(discoverTab,"Discover",ViewMode::Discover);
        draw_tab(nougatTab,"Search",ViewMode::Nougat);
        draw_tab(ytdlpTab,"Stream",ViewMode::Stream);
        draw_tab(debugTab,"Debug",ViewMode::Debug);
        XSetClipMask(d, gc, None);

        fill(target, {topNavViewportW,0,std::max(0,W-topNavViewportW),26}, suiteChocolate);
        outline(target, {0,24,W,1}, divider);
        const std::string versionLabel = "v0.0.21";
        const int versionWidth = text_width(versionLabel);
        const int versionX = W - 10 - versionWidth;
        const int treeX = versionX - nougat_media_suite_icon::kTopBar14Size - 6;
        bool serverBusy = false;
        reddmedia::MediaServerState serverStateValue = reddmedia::MediaServerState::Stopped;
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            serverBusy = serverState->busy;
            serverStateValue = serverState->state;
        }
        unsigned long light = col(0xe0e0, 0x3131, 0x4545);
        if (serverStateValue == reddmedia::MediaServerState::Ready && !serverBusy) light = col(0x8585,0xc7c7,0x4242);
        else if (serverStateValue == reddmedia::MediaServerState::Starting || serverBusy) light = col(0xf0f0,0xb4b4,0x2b2b);
        const std::string serverLabel = "Server:";
        const int serverX = treeX - 34 - text_width(serverLabel);
        if (serverX >= topNavViewportW) {
            text(target, serverX, 17, serverLabel, topText);
            fill_circle(target, serverX + text_width(serverLabel) + 7, 8, 10, light);
        }
        draw_suite_badge(target, treeX, 5, 0x4a, 0x1f, 0x0b);
        text(target, versionX, 17, versionLabel, topText);
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

    void draw_seek_time_row(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        const unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        const unsigned long markDark = col(0x5555,0x3333,0x3333);
        const unsigned long markReal = col(0xffff,0xd6d6,0xd6d6);
        fill(target, {0, std::max(0, seekRect.y-6), W, seekRect.h+16}, palette.background);
        fill(target, seekRect, palette.field);
        outline(target, seekRect, palette.border);
        long long t=0,l=0;
        if (mp) { t=playback_time_ms(); l=playback_length_ms(); }
        if (l > 0) {
            update_chapter_marks(false);
            int pos = (int)((double)t / (double)l * seekRect.w);
            fill(target, {seekRect.x, seekRect.y, std::max(1,pos), seekRect.h}, companyRed);
            for (long long markMs : chapterMarksMs) {
                if (markMs <= 0 || markMs >= l) continue;
                int mx = seekRect.x + (int)((double)markMs / (double)l * seekRect.w);
                line(target, mx, seekRect.y, mx, seekRect.y + seekRect.h, chapterMarksAreReal ? markReal : markDark);
            }
        }
        text(target, 10, seekRect.y+14, format_time(t), palette.text);
        int totalX = seekRect.x + seekRect.w + 10;
        text(target, totalX, seekRect.y+14, format_time(l), palette.text);
    }

    void draw_volume_bar(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        const unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        fill(target, {0, std::max(0, volRect.y-6), W, volRect.h+16}, palette.background);
        fill(target, volRect, palette.field);
        outline(target, volRect, palette.border);
        int vol = mp ? api.get_volume(mp) : volumePercent;
        if (vol < 0) vol = volumePercent;
        vol = std::max(0,std::min(200,vol));
        volumePercent = vol;
        fill(target, {volRect.x, volRect.y, volRect.w*vol/200, volRect.h}, companyRed);
        const int normalX = volRect.x + volRect.w / 2;
        line(target, normalX, volRect.y - 3, normalX, volRect.y + volRect.h + 3, palette.muted);
        text(target, normalX - 16, volRect.y - 5, "100%", palette.muted);
        text(target, 10, volRect.y+14, "Volume " + std::to_string(vol) + "%", palette.text);
    }

    void draw_controls(Drawable target) {
        draw_top_bar(target);
        if (currentView != ViewMode::VideoPlayer) return;
        const ViewPalette palette = palette_for(ViewMode::VideoPlayer);
        fill(target, {0, std::max(26, H-38), W, std::max(0, H-std::max(26, H-38))}, palette.panel);
        button_on(target, openBtn, "Open");
        button_on(target, rewindBtn, "Rewind 10s");
        button_on(target, playBtn, "Play/Pause");
        button_on(target, stopBtn, "Stop");
        button_on(target, forwardBtn, "Fast Forward 10s");
        button_on(target, fsBtn, "Fullscreen");
        draw_seek_time_row(target);
        draw_volume_bar(target);
    }

    bool loading_state(double& progress) {
        {
            std::lock_guard<std::mutex> lock(serverState->mutex);
            if (serverState->busy) { progress = serverState->progress; return true; }
        }
        if (currentView == ViewMode::Library) {
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                if (libraryState->busy) { progress = libraryState->progress; return true; }
            }
            {
                std::lock_guard<std::mutex> lock(posterState->mutex);
                if (posterState->busy) { progress = posterState->progress; return true; }
            }
        }
        if (currentView == ViewMode::Discover) {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) { progress = discoverState->progress; return true; }
        }
        if (currentView == ViewMode::Debug) {
            {
                std::lock_guard<std::mutex> lock(debugState->mutex);
                if (debugState->busy) { progress = debugState->progress; return true; }
            }
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                if (libraryState->busy) { progress = libraryState->progress; return true; }
            }
            {
                std::lock_guard<std::mutex> lock(posterState->mutex);
                if (posterState->busy) { progress = posterState->progress; return true; }
            }
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                if (discoverState->busy) { progress = discoverState->progress; return true; }
            }
        }
        return false;
    }

    void draw_loading_bar(Drawable target) {
        double progress = 0.0;
        if (!loading_state(progress)) return;
        const Rect track = {0, 26, W, 6};
        fill(target, track, col(0x8888,0x8888,0x8888));
        if (progress > 0.0) {
            const int loaded = std::max(2, std::min(W, static_cast<int>(progress * W)));
            fill(target, {0, 26, loaded, 6}, col(0xbbbb,0x0000,0x0000));
        } else {
            const int chunk = std::max(80, W / 5);
            const int position = static_cast<int>((now_ms() / 8) % (W + chunk)) - chunk;
            fill(target, {position, 26, chunk, 6}, col(0xbbbb,0x0000,0x0000));
        }
    }

    void draw_seek_time_only() {
        if (fullscreen || currentView != ViewMode::VideoPlayer) return;
        const int y0 = std::max(0, seekRect.y - 6);
        const int h = std::min(H - y0, seekRect.h + 16);
        if (h <= 0) return;
        Pixmap buffer = XCreatePixmap(d, win, W, H, DefaultDepth(d, screen));
        draw_seek_time_row(buffer);
        XCopyArea(d, buffer, win, gc, 0, y0, W, h, 0, y0);
        XFreePixmap(d, buffer);
        XFlush(d);
    }

    void draw_volume_only() {
        if (fullscreen || currentView != ViewMode::VideoPlayer) return;
        const int x0 = std::max(0, volRect.x - 62);
        const int y0 = std::max(0, volRect.y - 6);
        const int w = std::min(W - x0, volRect.w + 72);
        const int h = std::min(H - y0, volRect.h + 16);
        if (w <= 0 || h <= 0) return;
        Pixmap buffer = XCreatePixmap(d, win, W, H, DefaultDepth(d, screen));
        draw_volume_bar(buffer);
        XCopyArea(d, buffer, win, gc, x0, y0, w, h, x0, y0);
        XFreePixmap(d, buffer);
        XFlush(d);
    }

    const char* stream_platform_name(StreamPlatform platform) const {
        switch (platform) {
            case StreamPlatform::YouTube: return "YouTube";
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
            case StreamPlatform::Rumble: return "https://rumble.com/";
            case StreamPlatform::RuTube: return "https://rutube.ru/";
            case StreamPlatform::VK: return "https://vk.com/video";
            case StreamPlatform::OK: return "https://ok.ru/video";
        }
        return {};
    }

    void select_stream_platform(StreamPlatform platform) {
        streamPlatform = platform;
        urlFocused = false;
        urlSelectAll = false;
        ytdlpStatus = std::string(stream_platform_name(platform)) + " selected. Paste a video URL, Direct Watch, download, or open the webpage.";
        redraw();
    }

    void open_stream_webpage() {
        const std::string target = ytdlpUrl.empty() ? stream_platform_home(streamPlatform) : ytdlpUrl;
        if (target.empty()) return;
        launch_external_target(target);
        ytdlpStatus = std::string("Opened ") + stream_platform_name(streamPlatform) + " webpage in your default browser.";
        redraw();
    }

    void direct_watch_stream() {
        if (ytdlpUrl.empty()) {
            ytdlpStatus = "Paste a video URL first.";
            redraw();
            return;
        }
        ytdlpStatus = std::string("Direct Watch: opening ") + stream_platform_name(streamPlatform) + " in Nougat Media Suite's native player...";
        redraw();
        start_ytdlp_play();
    }

    void draw_stream_screen(Drawable target) {
        const ViewPalette palette = stream_palette_for(streamPlatform);
        fill(target, {0,26,W,H-26}, palette.background);
        fill(target, {18,36,W-36,52}, palette.panel);
        outline(target, {18,36,W-36,52}, palette.border);
        text(target, 28, 58, "STREAM", palette.text);
        text(target, 96, 58, std::string("Online video: ") + stream_platform_name(streamPlatform), palette.muted);

        const auto source_button = [&](const Rect& r, const char* label, StreamPlatform platform) {
            const bool selected = streamPlatform == platform;
            const ViewPalette own = stream_palette_for(platform);
            fill(target, r, selected ? own.buttonLight : own.buttonDark);
            outline(target, r, selected ? own.accent : own.border);
            if (selected) fill(target, {r.x, r.y + r.h - 3, r.w, 3}, own.accent);
            text(target, r.x + std::max(6,(r.w-text_width(label))/2), r.y+18, label,
                 selected ? own.buttonText : own.text);
        };
        source_button(streamYoutubeTab,"YouTube",StreamPlatform::YouTube);
        source_button(streamRumbleTab,"Rumble",StreamPlatform::Rumble);
        source_button(streamRutubeTab,"RuTube",StreamPlatform::RuTube);
        source_button(streamVkTab,"VK",StreamPlatform::VK);
        source_button(streamOkTab,"OK",StreamPlatform::OK);

        const unsigned long focusBorder = urlFocused ? palette.accent : palette.border;
        fill(target, ytdlpUrlRect, palette.field);
        outline(target, ytdlpUrlRect, focusBorder);
        text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y-8, "Video URL", palette.text);
        int urlTextMax = std::max(24, ytdlpUrlRect.w - 18);
        std::string visibleUrl = ytdlpUrl.empty() ? std::string("") : tail_to_width(ytdlpUrl, urlTextMax);
        XRectangle urlClip{(short)(ytdlpUrlRect.x+5),(short)(ytdlpUrlRect.y+2),
                           (unsigned short)std::max(1,ytdlpUrlRect.w-10),(unsigned short)std::max(1,ytdlpUrlRect.h-4)};
        XSetClipRectangles(d, gc, 0, 0, &urlClip, 1, Unsorted);
        const unsigned long fieldInk = col(0x1717,0x1111,0x0b0b);
        if (visibleUrl.empty() && !urlFocused) {
            text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y+18,
                 std::string("paste a ") + stream_platform_name(streamPlatform) + " video URL", palette.muted);
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
        XSetClipMask(d,gc,None);

        fill(target,ytdlpOutputRect,palette.field);
        outline(target,ytdlpOutputRect,palette.border);
        text(target,ytdlpOutputRect.x+8,ytdlpOutputRect.y+18,
             tail_to_width("Output folder: "+ytdlpOutputFolder,ytdlpOutputRect.w-16),fieldInk);
        button_on(target,ytdlpDownloadBtn,"Download");
        button_on(target,ytdlpPlayBtn,"Play");
        button_on(target,ytdlpDirectWatchBtn,"Direct Watch");
        button_on(target,ytdlpWebpageBtn,"Open Webpage");
        button_on(target,ytdlpClearBtn,"Clear Log");
        text(target,28,246,head_to_width("Status: "+ytdlpStatus,W-56),palette.text);

        Rect logBox={28,264,std::max(240,W-56),std::max(100,H-289)};
        fill(target,logBox,palette.panel);
        outline(target,logBox,palette.border);
        fill(target,{logBox.x,logBox.y,6,logBox.h},palette.accent);
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
        const ViewPalette palette = palette_for(ViewMode::P2P);
        const unsigned long dark = palette.text;
        const unsigned long fieldInk = col(0x0d0d,0x1b1b,0x2a2a);
        unsigned long border = p2pMagnetFocused ? col(0x7070,0xb0b0,0xdada) : palette.border;
        fill(target, {0,86,W,H-86}, palette.background);
        text(target, 28, 108, "P2P STREAMING", dark);
        text(target, 28, 130, "Open a magnet or P2P metadata file, choose a video, then Play.", dark);
        fill(target, p2pMagnetRect, palette.field);
        outline(target, p2pMagnetRect, border);
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
        XSetClipMask(d,gc,None);
        fill(target,p2pOutputRect,palette.field);
        outline(target,p2pOutputRect,palette.border);
        text(target,p2pOutputRect.x+8,p2pOutputRect.y+18,tail_to_width("Download folder: "+p2pOutputFolder,p2pOutputRect.w-16),fieldInk);
        button_on(target,p2pLoadMagnetBtn,"Load Magnet");
        button_on(target,p2pOpenTorrentBtn,"Open P2P File");
        button_on(target,p2pPlayBtn,"Play");
        button_on(target,p2pStopResumeBtn,p2p.is_paused()?"Resume Download":"Stop Download");

        auto_select_single_video();
        P2PStatus st=p2p.status();
        int y=278;
        text(target,28,y,"Status: "+p2pUiStatus,dark); y+=20;
        if (st.active) {
            int pct=std::max(0,std::min(100,(int)(st.progress*100.0f)));
            text(target,28,y,"Transfer: "+(st.name.empty()?std::string("loading metadata"):st.name),dark); y+=20;
            text(target,28,y,"State: "+st.state+"   Progress: "+std::to_string(pct)+"%   Downloaded: "+format_bytes(st.downloaded),dark); y+=20;
            text(target,28,y,"Down: "+format_bytes(st.download_rate)+"/s   Up: "+format_bytes(st.upload_rate)+"/s   Peers: "+std::to_string(st.peers)+"   Seeds: "+std::to_string(st.seeds),dark); y+=22;
            if (!st.error.empty()) { text(target,28,y,"P2P: "+st.error,col(0x9900,0,0)); y+=20; }
        }
        p2pFileRows.clear();
        std::vector<P2PFileInfo> fs=p2p.files();
        Rect fileBox={28,y,std::max(240,W-56),std::max(80,H-y-24)};
        fill(target,fileBox,palette.panel); outline(target,fileBox,palette.border);
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
        if (mediaServer.state() == reddmedia::MediaServerState::Ready && !mediaServer.owns_server()) {
            return "Server: Ready (independent; Stop preserves it)";
        }
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
            serverState->progress = 0.05;
            serverState->state = reddmedia::MediaServerState::Starting;
            if (operation == 1) serverState->status = "Starting integrated server...";
            else if (operation == 2) serverState->status = "Stopping owned integrated server...";
            else serverState->status = "Refreshing server status...";
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

    void start_library_task(int operation,
                            const std::string& folder = {},
                            const reddmedia::LibraryNode& parent = {}) {
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            if (libraryState->busy) return;
        }
        if (libraryWorker.joinable()) libraryWorker.join();
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            libraryState->busy = true;
            libraryState->updated = false;
            libraryState->progress = 0.05;
            if (operation == 1) libraryState->status = "Linking folder and scanning metadata...";
            else if (operation == 2) libraryState->status = "Refreshing this library...";
            else if (operation == 3) libraryState->status = "Unlinking folder...";
            else libraryState->status = "Loading real library metadata...";
        }
        if (operation == 2) {
            std::lock_guard<std::mutex> lock(posterState->mutex);
            posterState->failed.clear();
        }
        redraw();

        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        const std::shared_ptr<LibraryUiState> state = libraryState;
        const reddmedia::LibraryMediaType media_type = libraryMediaType;
        const bool type_chosen = libraryTypeChosen;
        libraryWorker = std::thread([client, engine, state, operation, folder, parent,
                                     media_type, type_chosen]() {
            const auto set_progress = [state](double progress) {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = progress;
            };
            std::string error;
            bool ok = client->initialize(error);
            set_progress(0.18);
            if (ok && operation == 1) ok = client->add_media_folder(folder, media_type, error);
            if (ok && operation == 2) ok = client->refresh_library(error);
            if (ok && operation == 3) ok = client->unlink_media_folder(folder, media_type, error);
            set_progress(0.48);
            std::vector<reddmedia::MediaFolder> folders;
            if (ok) ok = client->load_media_folders(folders, error);
            set_progress(0.62);
            std::vector<reddmedia::LibraryNode> nodes;
            if (ok && operation == 5) ok = client->load_library_children(parent, nodes, error);
            else if (ok && type_chosen && operation != 0) {
                ok = client->load_library_roots(media_type, nodes, error);
            }
            if (ok && engine->external_credential_available()) {
                for (std::size_t index = 0; index < nodes.size(); ++index) {
                    reddmedia::LibraryNode& node = nodes[index];
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
                    if (node.poster_item_id.empty() && node.tmdb_poster_path.empty() &&
                        !node.series_tmdb_id.empty()) {
                        engine->load_tv_poster_path(node.series_tmdb_id,
                            node.kind == reddmedia::LibraryNodeKind::Series ? 0 : node.season_number,
                            node.tmdb_poster_path, fallback_error);
                    } else if (node.kind == reddmedia::LibraryNodeKind::Movie &&
                               node.poster_item_id.empty() && node.tmdb_poster_path.empty() &&
                               !node.tmdb_id.empty()) {
                        engine->load_movie_poster_path(node.tmdb_id,
                                                      node.tmdb_poster_path,
                                                      fallback_error);
                    }
                    set_progress(0.62 + 0.27 * static_cast<double>(index + 1U) /
                        static_cast<double>(std::max<std::size_t>(1U, nodes.size())));
                }
            }
            set_progress(0.90);

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

    bool prepare_tv_autoplay(const reddmedia::LibraryNode& selected) {
        tvAutoplayQueue.clear();
        tvAutoplayIndex = -1;
        tvAutoplayArmed = false;
        activeLibraryItemValid = false;
        playbackEndHandled = false;
        if (selected.kind != reddmedia::LibraryNodeKind::Episode) return false;
        reddmedia::LibraryNode series;
        bool have_series = false;
        for (const auto& parent : libraryParents) {
            if (parent.kind == reddmedia::LibraryNodeKind::Series) { series = parent; have_series = true; break; }
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
            if ((!selected.id.empty() && tvAutoplayQueue[i].id == selected.id) ||
                (!selected.path.empty() && tvAutoplayQueue[i].path == selected.path)) {
                tvAutoplayIndex = static_cast<int>(i);
                break;
            }
        }
        if (tvAutoplayIndex < 0) return false;
        activeLibraryItem = selected;
        activeLibraryItemValid = true;
        tvAutoplayArmed = true;
        return true;
    }

    void mark_active_episode_completed() {
        if (!activeLibraryItemValid) return;
        std::string error;
        recommendationEngine->record_completed(descriptor_for_node(activeLibraryItem), error);
    }

    bool play_next_tv_episode() {
        if (!tvAutoplayArmed || tvAutoplayIndex < 0) return false;
        const int next = tvAutoplayIndex + 1;
        if (next >= static_cast<int>(tvAutoplayQueue.size())) {
            tvAutoplayArmed = false;
            return false;
        }
        tvAutoplayIndex = next;
        activeLibraryItem = tvAutoplayQueue[static_cast<std::size_t>(next)];
        activeLibraryItemValid = true;
        std::string history_error;
        recommendationEngine->record_started(descriptor_for_node(activeLibraryItem), history_error);
        open_media(activeLibraryItem.path, 0);
        return true;
    }

    void poll_natural_playback_end() {
        if (!mp || paused || playbackEndHandled || currentMediaIsNetwork || currentMediaIsP2P || currentMediaIsYtDlpStream) return;
        bool ended = false;
        if (api.get_state) ended = api.get_state(mp) == 6;
        if (!ended) {
            const long long length = playback_length_ms();
            const long long position = playback_time_ms();
            ended = length > 0 && position >= std::max<long long>(0, length - 350);
        }
        if (!ended) return;
        playbackEndHandled = true;
        mark_active_episode_completed();
        if (!play_next_tv_episode() && !fullscreen) redraw();
    }

    void open_library_node(const reddmedia::LibraryNode& selected) {
        if (selected.kind == reddmedia::LibraryNodeKind::MovieCollection ||
            selected.kind == reddmedia::LibraryNodeKind::Series ||
            selected.kind == reddmedia::LibraryNodeKind::Season) {
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
        std::string history_error;
        recommendationEngine->record_started(descriptor_for_node(selected), history_error);
        switch_view(ViewMode::VideoPlayer);
        open_media(selected.path, 0);
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
        libraryParents.pop_back();
        librarySelected = -1;
        libraryScroll = 0;
        if (libraryParents.empty()) start_library_task(4);
        else start_library_task(5, {}, libraryParents.back());
    }

    std::string library_poster_key(const reddmedia::LibraryNode& node) const {
        if (!node.poster_item_id.empty()) {
            return "jellyfin:" + node.poster_item_id + ":" + node.poster_image_tag;
        }
        if (!node.tmdb_poster_path.empty()) return "tmdb:" + node.tmdb_poster_path;
        return "";
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
        }
        const std::shared_ptr<PosterUiState> posters = posterState;
        const std::shared_ptr<reddmedia::JellyfinApiClient> client = libraryClient;
        const std::shared_ptr<reddmedia::RecommendationEngine> engine = recommendationEngine;
        posterWorker = std::thread([posters, client, engine, nodes]() {
            std::size_t total = 0;
            for (const auto& node : nodes) {
                if (!node.poster_item_id.empty() || !node.tmdb_poster_path.empty()) ++total;
            }
            std::size_t completed = 0;
            for (const auto& node : nodes) {
                std::string key;
                if (!node.poster_item_id.empty()) {
                    key = "jellyfin:" + node.poster_item_id + ":" + node.poster_image_tag;
                } else if (!node.tmdb_poster_path.empty()) {
                    key = "tmdb:" + node.tmdb_poster_path;
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
                    const bool source_loaded = !node.poster_item_id.empty()
                        ? client->load_primary_image_bmp(node.poster_item_id,
                            node.poster_image_tag, 132, 158, bytes, error)
                        : engine->load_external_poster_bmp(node.tmdb_poster_path,
                            132, 158, bytes, error);
                    const bool loaded = source_loaded &&
                        reddmedia::decode_library_poster_bmp(bytes, poster, error);
                    std::lock_guard<std::mutex> lock(posters->mutex);
                    if (loaded) posters->cache[key] = std::move(poster);
                    else posters->failed.insert(key);
                }
                ++completed;
                std::lock_guard<std::mutex> lock(posters->mutex);
                posters->progress = total == 0U ? 1.0 :
                    static_cast<double>(completed) / static_cast<double>(total);
            }
            std::lock_guard<std::mutex> lock(posters->mutex);
            posters->busy = false;
            posters->updated = true;
            posters->progress = 1.0;
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

    void draw_library_poster(Drawable target,
                             const Rect& area,
                             const reddmedia::LibraryNode& node) {
        fill(target, area, col(0x0808,0x0808,0x0808));
        const std::string key = library_poster_key(node);
        if (key.empty()) {
            text(target, area.x + 12, area.y + area.h / 2, "NO POSTER",
                 col(0x9999,0x9999,0x9999));
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
        if (available) draw_poster_pixels(target, area, poster);
        else text(target, area.x + 12, area.y + area.h / 2,
                  loading ? "LOADING..." : "NO POSTER",
                  col(0x9999,0x9999,0x9999));
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
        metrics.columns = std::max(1, (inner_width + metrics.gap) / (140 + metrics.gap));
        metrics.rows = std::max(1, (inner_height + metrics.gap) / (180 + metrics.gap));
        metrics.tileWidth = std::max(108,
            (inner_width - (metrics.columns - 1) * metrics.gap) / metrics.columns);
        metrics.tileHeight = std::max(170,
            (inner_height - (metrics.rows - 1) * metrics.gap) / metrics.rows);
        metrics.posterHeight = std::max(118, metrics.tileHeight - 50);
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
        fill(target, r, active ? palette.selection : palette.button);
        outline(target, r, active ? palette.accent : palette.buttonDark);
        if (mode == LibraryDisplayMode::List) {
            const int x1 = r.x + 7;
            const int x2 = r.x + r.w - 7;
            for (int row = 0; row < 3; ++row) {
                const int y = r.y + 7 + row * 6;
                line(target, x1, y, x2, y, palette.buttonText);
            }
        } else {
            const int size = 6;
            const int gap = 4;
            const int total = size * 2 + gap;
            const int sx = r.x + (r.w - total) / 2;
            const int sy = r.y + (r.h - total) / 2;
            for (int row = 0; row < 2; ++row) {
                for (int column = 0; column < 2; ++column) {
                    Rect square{sx + column * (size + gap), sy + row * (size + gap), size, size};
                    fill(target, square, palette.buttonText);
                }
            }
        }
    }

    void draw_library_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Library);
        fill(target,{0,26,W,H-26},palette.background);
        text(target,28,58,libraryParents.empty()?"MEDIA LIBRARY":libraryParents.back().name,palette.text);
        draw_library_view_button(target,libraryListViewBtn,LibraryDisplayMode::List,current_library_display_mode()==LibraryDisplayMode::List);
        draw_library_view_button(target,libraryGridBtn,LibraryDisplayMode::Grid,current_library_display_mode()==LibraryDisplayMode::Grid);
        button_on(target,libraryMoviesBtn,"Movies");
        button_on(target,libraryTvBtn,"TV");
        button_on(target,libraryAddFolderBtn,"Link Folder");
        button_on(target,libraryUnlinkFolderBtn,"Unlink Folder");
        button_on(target,libraryRefreshBtn,"Refresh Library");
        button_on(target,libraryBackBtn,"Back");
        button_on(target,serverStartBtn,"Start Server");
        button_on(target,serverStopBtn,"Stop Server");
        button_on(target,serverRefreshBtn,"Refresh Server");

        std::vector<reddmedia::LibraryNode> nodes;
        std::string status;
        bool busy=false;
        {
            std::lock_guard<std::mutex> lock(libraryState->mutex);
            nodes=libraryState->nodes; status=libraryState->status; busy=libraryState->busy;
        }
        text(target,28,122,head_to_width(std::string("Status: ")+(busy?"Working - ":"")+status,W-56),palette.text);
        fill(target,libraryListBox,palette.panel);
        outline(target,libraryListBox,palette.border);
        libraryRows.clear();
        const LibraryGridMetrics grid=library_grid_metrics();
        const int max_scroll=std::max(0,static_cast<int>(nodes.size())-grid.visibleItems);
        libraryScroll=std::max(0,std::min(libraryScroll,max_scroll));

        if (current_library_display_mode()==LibraryDisplayMode::List) {
            const int rowHeight=32;
            const int visible=std::max(1,(libraryListBox.h-12)/rowHeight);
            for(int visibleIndex=0;visibleIndex<visible;++visibleIndex) {
                const int nodeIndex=libraryScroll+visibleIndex;
                if(nodeIndex>=static_cast<int>(nodes.size())) break;
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
            }
        } else {
            for(int visible_index=0;visible_index<grid.visibleItems;++visible_index) {
                const int node_index=libraryScroll+visible_index;
                if(node_index>=static_cast<int>(nodes.size())) break;
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
            }
        }
        if(nodes.empty() && !busy) text(target,libraryListBox.x+12,libraryListBox.y+28,
            libraryTypeChosen?"No real titles to show.":"Choose Movies or TV.",palette.muted);
    }

    void start_discover_task(reddmedia::RecommendationSource source,
                             reddmedia::RecommendationMediaType media_type) {
        {
            std::lock_guard<std::mutex> lock(discoverState->mutex);
            if (discoverState->busy) return;
        }
        if (source == reddmedia::RecommendationSource::External &&
            !recommendationEngine->external_credential_available()) {
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->status =
                    "External recommendations need a validated TMDb credential. Use Save / Replace.";
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

    reddmedia::DiagnosticInput diagnostic_input() {
        reddmedia::DiagnosticInput input;
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
        input.tmdb_configured = recommendationEngine->external_credential_available();
        input.ai_model_path = exe_dir() +
            "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf";
        input.ai_runtime_path = exe_dir() + "/components/ai/runtime";
        return input;
    }

    void start_debug_task() {
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            if (debugState->busy) return;
        }
        if (debugWorker.joinable()) debugWorker.join();
        const reddmedia::DiagnosticInput input = diagnostic_input();
        {
            std::lock_guard<std::mutex> lock(debugState->mutex);
            debugState->busy = true;
            debugState->updated = false;
            debugState->progress = 0.10;
            debugState->status = "Running evidence-based health checks...";
        }
        redraw();
        const std::shared_ptr<DebugUiState> state = debugState;
        const reddmedia::DiagnosticEngine engine = diagnosticEngine;
        debugWorker = std::thread([state, engine, input]() {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                state->progress = 0.45;
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

    unsigned long nougat_cocoa() { return col(0x1717,0x0d0d,0x0808); }
    unsigned long nougat_chocolate() { return col(0x4a4a,0x1f1f,0x0b0b); }
    unsigned long nougat_tan() { return col(0xd2d2,0xa5a5,0x6d6d); }
    unsigned long nougat_caramel() { return col(0xc9c9,0x8282,0x2c2c); }
    unsigned long nougat_cream() { return col(0xf2f2,0xe6e6,0xc9c9); }
    unsigned long nougat_light() { return col(0xdfdf,0xc6c6,0xa5a5); }
    unsigned long nougat_dark() { return col(0x1a1a,0x1212,0x0e0e); }

    void nougat_button(Drawable target, const Rect& r, const std::string& label, bool selected=false) {
        fill(target, r, selected ? nougat_tan() : nougat_caramel());
        outline(target, r, nougat_chocolate());
        text(target, r.x + 9, r.y + 20, label, selected ? nougat_dark() : nougat_cream());
    }

    void draw_nougat_input(Drawable target, const Rect& r, const std::string& value, NougatInputFocus field) {
        fill(target, r, nougat_cream());
        outline(target, r, field == nougatInputFocus ? nougat_caramel() : nougat_tan());
        const std::string shown = tail_to_width(value, std::max(20, (int)r.w - 16));
        if (nougatInputFocus == field && nougatInputSelectAll && !value.empty()) {
            fill(target, {r.x+5, r.y+5, std::min((int)r.w-10, text_width(shown)+6), (int)r.h-10}, nougat_tan());
        }
        text(target, r.x+8, r.y+20, shown, nougat_dark());
        if (nougatInputFocus == field && !nougatInputSelectAll) {
            int cx = r.x + 8 + text_width(shown);
            cx = std::min(cx, r.x + r.w - 8);
            line(target, cx, r.y + 5, cx, r.y + r.h - 5, nougat_dark());
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
        for (std::size_t i=0;i<p2pFileRows.size();++i) {
            if (p2pFileRows[i].contains(x,y)) {
                std::vector<P2PFileInfo> fs=p2p.files();
                if (i<fs.size()) { std::string error; if (p2p.select_file(fs[i].index,error)) p2pUiStatus="Selected: "+fs[i].path; else p2pUiStatus=error; }
                redraw(); return;
            }
        }
        p2pMagnetFocused=false; p2pMagnetSelectAll=false; redraw(); return;
    }

    void handle_nougat_click(int x, int y) {
        if (nougatSearchPanelTab.contains(x,y)) {
            nougatPanel=NougatPanel::Search; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; nougatOutputFocused=false; p2pMagnetFocused=false;
            redraw(); return;
        }
        if (nougatCrawlerPanelTab.contains(x,y)) {
            nougatPanel=NougatPanel::Crawler; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; p2pMagnetFocused=false; redraw(); return;
        }
        if (nougatP2PPanelTab.contains(x,y)) {
            nougatPanel=NougatPanel::P2P; nougatNetworkAdvanced=false;
            nougatInputFocus=NougatInputFocus::NoFocus; nougatOutputFocused=false; redraw(); return;
        }
        if (nougatPanel == NougatPanel::Search && nougatNetworkAdvancedBtn.contains(x,y)) {
            nougatNetworkAdvanced=!nougatNetworkAdvanced;
            nougatInputFocus=NougatInputFocus::NoFocus;
            if (nougatNetworkAdvanced) refresh_nougat_peers();
            redraw(); return;
        }
        if (nougatPanel == NougatPanel::P2P) { handle_p2p_click(x,y); return; }
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
                if (hit.open_tor.contains(x,y)) { nougat.open_url(result.url,true,error); return; }
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
        fill(target, {0,26,W,H-26}, nougat_cocoa());
        text(target, 28, 44, "SEARCH", nougat_tan());
        text(target, 96, 44, "Nougat Media Suite | Just Nougat it.", nougat_light());
        std::string node;
        std::string status;
        bool search_busy=false, crawl_busy=false;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); node=nougatState->node_id; status=nougatState->status; search_busy=nougatState->search_busy; crawl_busy=nougatState->crawl_busy; }
        if (!node.empty()) text(target, std::max(500,W-220), 44, "Node " + node, nougat_light());
        nougat_button(target,nougatSearchPanelTab,"Search",nougatPanel==NougatPanel::Search && !nougatNetworkAdvanced);
        nougat_button(target,nougatCrawlerPanelTab,"Crawler",nougatPanel==NougatPanel::Crawler);
        nougat_button(target,nougatP2PPanelTab,"P2P",nougatPanel==NougatPanel::P2P);
        if (nougatPanel == NougatPanel::Search) {
            nougat_button(target,nougatNetworkAdvancedBtn,nougatNetworkAdvanced?"Back":"Network...",nougatNetworkAdvanced);
        }

        if (nougatPanel == NougatPanel::P2P) {
            draw_p2p_screen(target);
            return;
        }
        if (nougatPanel == NougatPanel::Search && nougatNetworkAdvanced) {
            text(target,28,106,"NETWORK / ADVANCED",nougat_cream());
            draw_nougat_input(target,nougatPeerEntryRect,nougatPeerEntry,NougatInputFocus::Peer);
            nougat_button(target,nougatAddPeerBtn,"Add Peer");
            nougat_button(target,nougatRemovePeerBtn,"Remove");
            nougat_button(target,nougatNodeBtn,nougat.node_running()?"STOP NODE":"START NODE",nougat.node_running());
            nougat_button(target,nougatPeersToggleBtn,nougatSearchPeers?"Search peers [x]":"Search peers",nougatSearchPeers);
            fill(target,nougatPeerListBox,nougat_chocolate()); outline(target,nougatPeerListBox,nougat_tan());
            std::vector<std::string> peers;
            { std::lock_guard<std::mutex> lock(nougatState->mutex); peers=nougatState->peers; }
            const int visible=std::max(1,((int)nougatPeerListBox.h-16)/22);
            nougatPeerScroll=std::max(0,std::min(nougatPeerScroll,std::max(0,(int)peers.size()-visible)));
            int y=nougatPeerListBox.y+22;
            for (int i=nougatPeerScroll; i<(int)peers.size() && i<nougatPeerScroll+visible; ++i) {
                if (i==nougatPeerSelected) fill(target,{nougatPeerListBox.x+5,y-16,(int)nougatPeerListBox.w-10,21},nougat_tan());
                text(target,nougatPeerListBox.x+9,y,peers[(size_t)i],i==nougatPeerSelected?nougat_dark():nougat_cream()); y+=22;
            }
            text(target,28,H-20,status,nougat_light());
            return;
        }
        if (nougatPanel == NougatPanel::Search) {
            draw_nougat_input(target,nougatSearchRect,nougatSearchQuery,NougatInputFocus::Search);
            nougat_button(target,nougatRawBtn,nougatRaw?"RAW [x]":"RAW",nougatRaw);
            nougat_button(target,nougatSearchBtn,search_busy?"SEARCHING":"SEARCH");
            text(target,28,160,status,nougat_light());
            fill(target,nougatResultsBox,nougat_chocolate()); outline(target,nougatResultsBox,nougat_tan());
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
                fill(target,card,col(0x3d3d,0x2828,0x2020)); outline(target,card,nougat_tan());
                text(target,card.x+8,card.y+18,head_to_width(std::to_string(nougatSearchOffset+i+1)+". "+r.title,card.w-170),nougat_cream());
                text(target,card.x+8,card.y+38,head_to_width(r.url,card.w-16),nougat_tan());
                text(target,card.x+8,card.y+58,head_to_width(r.snippet,card.w-16),nougat_light());
                text(target,card.x+8,card.y+78,head_to_width(r.source_network+" | Node "+r.source_node,card.w-300),nougat_light());
                Rect copy={card.x+card.w-278,card.y+63,82,24};
                Rect tor={card.x+card.w-190,card.y+63,94,24};
                Rect open={card.x+card.w-90,card.y+63,82,24};
                nougat_button(target,copy,"Copy URL"); nougat_button(target,tor,"Open Tor"); nougat_button(target,open,"Open");
                nougatResultHitboxes.push_back({card,open,tor,copy,i});
                y+=card_h;
            }
            return;
        }
        text(target,28,102,"Seed URL",nougat_cream());
        draw_nougat_input(target,nougatCrawlSeedRect,nougatCrawlSeed,NougatInputFocus::CrawlSeed);
        nougat_button(target,nougatCrawlMinusBtn,"-"); nougat_button(target,nougatCrawlPlusBtn,"+");
        text(target,nougatCrawlMinusBtn.x-96,130,"Max pages: "+std::to_string(nougatMaxPages),nougat_cream());
        nougat_button(target,nougatSameDomainBtn,nougatSameDomain?"Stay on domain [x]":"Stay on domain",nougatSameDomain);
        nougat_button(target,nougatStartCrawlBtn,crawl_busy?"CRAWLING":"START CRAWL");
        fill(target,nougatCrawlLogBox,nougat_chocolate()); outline(target,nougatCrawlLogBox,nougat_tan());
        std::vector<std::string> lines;
        { std::lock_guard<std::mutex> lock(nougatState->mutex); lines=nougatState->crawl_log; }
        const int visible=std::max(1,((int)nougatCrawlLogBox.h-16)/18);
        const int max_scroll=std::max(0,(int)lines.size()-visible);
        nougatCrawlScroll=std::max(0,std::min(nougatCrawlScroll,max_scroll));
        if (crawl_busy && max_scroll>0) nougatCrawlScroll=max_scroll;
        int y=nougatCrawlLogBox.y+20;
        for (int i=nougatCrawlScroll; i<(int)lines.size() && i<nougatCrawlScroll+visible; ++i) {
            const bool selected=nougatOutputSelectionStart>=0 && i>=std::min(nougatOutputSelectionStart,nougatOutputSelectionEnd) && i<=std::max(nougatOutputSelectionStart,nougatOutputSelectionEnd);
            if (selected) fill(target,{nougatCrawlLogBox.x+5,y-14,(int)nougatCrawlLogBox.w-10,18},nougat_tan());
            text(target,nougatCrawlLogBox.x+9,y,head_to_width(lines[(size_t)i],nougatCrawlLogBox.w-18),selected?nougat_dark():nougat_cream());
            y+=18;
        }
        text(target,28,194,status,nougat_light());
    }

    void poll_nougat_workers() {
        bool needs_redraw=false;
        {
            std::lock_guard<std::mutex> lock(nougatState->mutex);
            if (nougatState->updated) { nougatState->updated=false; needs_redraw=true; }
        }
        if (needs_redraw && !fullscreen && currentView==ViewMode::Nougat) redraw();
    }

    void draw_debug_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Debug);
        const unsigned long dark = palette.text;
        const unsigned long border = palette.border;
        fill(target, {0,26,W,H-26}, palette.background);
        text(target, 28, 58, "DEBUG AND SYSTEM HEALTH", dark);
        button_on(target, debugRunBtn, "Run Checks");
        button_on(target, debugRetryBtn, "Retry");
        button_on(target, debugMetadataBtn, "Refresh Metadata");
        button_on(target, debugTmdbBtn, "Test TMDb");
        button_on(target, debugServerBtn, "Refresh Server");
        button_on(target, debugLogsBtn, "Open Logs");
        button_on(target, debugCopyBtn, "Copy Report");

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
        fill(target, debugListBox, palette.panel);
        outline(target, debugListBox, border);
        debugIssueRows.clear();
        if (!has_report) {
            text(target, debugListBox.x + 14, debugListBox.y + 30,
                 "Run Checks to inspect the server, metadata, TMDb, AI runtime, and local paths.",
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
                discoverState->status = "This is an External recommendation and is not on your server.";
            }
            redraw();
            return;
        }
        if (!exists_file(result.item.local_path)) {
            {
                std::lock_guard<std::mutex> lock(discoverState->mutex);
                discoverState->status = "That Local media file is unavailable. Refresh the Library.";
            }
            redraw();
            return;
        }
        std::string history_error;
        recommendationEngine->record_started(result.item, history_error);
        cancel_tv_autoplay();
        switch_view(ViewMode::VideoPlayer);
        open_media(result.item.local_path, 0);
    }

    void draw_discover_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Discover);
        const unsigned long dark = palette.text;
        const unsigned long border = palette.border;
        fill(target, {0,26,W,H-26}, palette.background);
        if (discoverServiceSettings) {
            text(target, 28, 58, "MY STREAMING SERVICES - UNITED STATES", dark);
            button_on(target, discoverServicesBackBtn, "Back to Discover");
            text(target, 224, 97,
                 "Select services you use. Availability still shows every verified listing.",
                 palette.muted);
            const Rect services_box = {28, 120, std::max(240, W - 56), std::max(150, H - 148)};
            fill(target, services_box, palette.panel);
            outline(target, services_box, border);
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
                    fill(target, row, palette.selection);
                }
                outline(target, row, palette.border);
                const std::string label =
                    std::string(watchPreferences.is_selected(provider.id) ? "[x] " : "[ ] ") +
                    provider.name;
                text(target, row.x + 8, row.y + 18,
                     head_to_width(label, row.w - 16), dark);
                discoverProviderRows.push_back({row, provider.id});
                row_y += row_height;
            }
            return;
        }
        text(target, 28, 58,
             discoverMode == reddmedia::RecommendationMode::Usual
                 ? "DISCOVER USUAL" : "DISCOVER RANDOM",
             dark);
        button_on(target, discoverUsualTab, "Usual");
        button_on(target, discoverRandomTab, "Random");
        button_on(target, discoverLocalMovieBtn, "Local Movie");
        button_on(target, discoverLocalTvBtn, "Local TV");
        button_on(target, discoverExternalMovieBtn, "External Movie");
        button_on(target, discoverExternalTvBtn, "External TV");
        button_on(target, discoverTmdbTestBtn, "Test TMDb");
        button_on(target, discoverTmdbReplaceBtn, "Save / Replace");
        button_on(target, discoverTmdbClearBtn, "Clear TMDb");
        button_on(target, discoverMyServicesBtn, "My Services");
        text(target, 28, 122, head_to_width(recommendationEngine->external_credential_label(), W - 56), dark);

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
        fill(target, discoverResultBox, palette.panel);
        outline(target, discoverResultBox, border);
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
             result.item.local_path.empty() ? "External" : "Local", col(0xc7c7,0x9f9f,0xd2d2));
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
            if (p2p.pause_transfer(error)) p2pUiStatus="P2P download stopped. Partial data and resume state preserved.";
            else p2pUiStatus=error;
        }
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
        if (currentView == ViewMode::Library) draw_library_screen(buffer);
        if (currentView == ViewMode::Discover) draw_discover_screen(buffer);
        if (currentView == ViewMode::Nougat) draw_nougat_screen(buffer);
        if (currentView == ViewMode::Debug) draw_debug_screen(buffer);
        if (currentView == ViewMode::Stream) draw_stream_screen(buffer);
        draw_loading_bar(buffer);
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

    void switch_view(ViewMode v) {
        if (currentView == v) return;
        currentView = v;
        urlFocused = false;
        urlSelectAll = false;
        p2pMagnetFocused = false;
        p2pMagnetSelectAll = false;
        close_context_menu();
        apply_video_layout();
        redraw();
    }
    void scroll_button_row(int& offset, int button_count, int delta, int viewport_width = -1) {
        if (viewport_width < 0) viewport_width = std::max(kCompactButtonW, W - 56);
        offset = clamp_button_scroll(offset + delta, button_count, viewport_width);
        layout();
        redraw();
    }
    void scroll_bottom_controls(int delta) {
        scroll_button_row(controlsScrollX, 6, delta, std::max(kCompactButtonW, W - 20));
    }
    void scroll_top_navigation(int delta) {
        scroll_button_row(topNavScrollX, 7, delta, topNavViewportW);
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

    bool handle_wheel(Window target, int x, int y, unsigned int button) {
        int delta = (button == Button4) ? -40 : 40;
        if (target == win && y < 26) { scroll_top_navigation(delta); return true; }
        if (target == win && y >= 70 && y < 110) {
            if (currentView == ViewMode::Library) { scroll_button_row(libraryButtonsScrollX,9,delta); return true; }
            if (currentView == ViewMode::Discover && !discoverServiceSettings) { scroll_button_row(discoverButtonsScrollX,10,delta); return true; }
            if (currentView == ViewMode::Debug) { scroll_button_row(debugButtonsScrollX,7,delta); return true; }
        }
        if (target == win && currentView == ViewMode::Stream && y >= 198 && y < 234) { scroll_button_row(ytdlpButtonsScrollX,5,delta); return true; }
        if (target == win && currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && y >= 224 && y < 266) { scroll_button_row(p2pButtonsScrollX,4,delta); return true; }
        if (currentView == ViewMode::Stream && target == win && y >= 54 && y < 82) {
            scroll_button_row(streamSourceScrollX,5,delta);
            return true;
        }
        if (currentView == ViewMode::Library && target == win && libraryListBox.contains(x,y)) {
            std::size_t count = 0;
            {
                std::lock_guard<std::mutex> lock(libraryState->mutex);
                count = libraryState->nodes.size();
            }
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
            if (needResumePrompt && videoResumeBtn.contains(x,y)) { pendingVideoSingleClick=false; open_media(sessionPath, sessionTime); return; }
            if (needResumePrompt && videoLoadBtn.contains(x,y)) { pendingVideoSingleClick=false; needResumePrompt=false; redraw(); do_open(); return; }
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
        if (y < 26 && videoPlayerTab.contains(x,y)) {
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
            show_menu(win, 8, 26, items);
            return;
        }
        if (y < 26 && libraryTab.contains(x,y)) {
            if (currentView != ViewMode::Library) {
                switch_view(ViewMode::Library);
                if (!libraryTypeChosen) start_library_task(0);
            }
            return;
        }
        if (y < 26 && discoverTab.contains(x,y)) {
            if (currentView != ViewMode::Discover) switch_view(ViewMode::Discover);
            return;
        }
        if (y < 26 && nougatTab.contains(x,y)) {
            if (currentView != ViewMode::Nougat) switch_view(ViewMode::Nougat);
            return;
        }
        if (y < 26 && ytdlpTab.contains(x,y)) {
            if (currentView != ViewMode::Stream) { switch_view(ViewMode::Stream); return; }
            show_ytdlp_menu(ytdlpTab.x, 26);
            return;
        }
        if (y < 26 && debugTab.contains(x,y)) {
            if (currentView != ViewMode::Debug) switch_view(ViewMode::Debug);
            return;
        }
        if (currentView == ViewMode::Nougat) {
            handle_nougat_click(x, y);
            return;
        }
        if (currentView == ViewMode::Library) {
            if (libraryMoviesBtn.contains(x,y)) {
                select_library_type(reddmedia::LibraryMediaType::Movies);
                return;
            }
            if (libraryTvBtn.contains(x,y)) {
                select_library_type(reddmedia::LibraryMediaType::Television);
                return;
            }
            if (libraryGridBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::Grid); return; }
            if (libraryListViewBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::List); return; }
            if (libraryAddFolderBtn.contains(x,y)) { add_library_folder(); return; }
            if (libraryUnlinkFolderBtn.contains(x,y)) { unlink_library_folder(); return; }
            if (libraryRefreshBtn.contains(x,y)) {
                if (libraryTypeChosen) start_library_task(2);
                return;
            }
            if (libraryBackBtn.contains(x,y)) { library_back(); return; }
            if (serverStartBtn.contains(x,y)) { start_server_task(1); return; }
            if (serverStopBtn.contains(x,y)) { start_server_task(2); return; }
            if (serverRefreshBtn.contains(x,y)) { start_server_task(3); return; }
            for (std::size_t row = 0; row < libraryRows.size(); ++row) {
                if (libraryRows[row].contains(x,y)) {
                    librarySelected = libraryScroll + static_cast<int>(row);
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
                discoverMode = reddmedia::RecommendationMode::Usual;
                redraw();
                return;
            }
            if (discoverRandomTab.contains(x,y)) {
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
                start_provider_catalog_task();
                return;
            }
            if (discoverOpenBtn.contains(x,y)) { open_discover_result(); return; }
            if (discoverWatchBtn.contains(x,y)) { open_watch_options(); return; }
            return;
        }
        if (currentView == ViewMode::Debug) {
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
            if (debugServerBtn.contains(x,y)) { start_server_task(3); return; }
            if (debugLogsBtn.contains(x,y)) { open_debug_logs(); return; }
            if (debugCopyBtn.contains(x,y)) { copy_debug_report(); return; }
            return;
        }
        if (currentView == ViewMode::Stream) {
            if (streamYoutubeTab.contains(x,y)) { select_stream_platform(StreamPlatform::YouTube); return; }
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
            if (ytdlpPlayBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; start_ytdlp_play(); return; }
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
        if (playBtn.contains(x,y)) { toggle_play(); return; }
        if (stopBtn.contains(x,y)) { stop_media(); return; }
        if (forwardBtn.contains(x,y)) { seek_relative(10000); return; }
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
                    } else if (contextMenuOpen && e.xexpose.window == contextMenu) {
                        draw_context_menu();
                        XFlush(d);
                    }
                }
                else if (e.type == ConfigureNotify && e.xconfigure.window == win) resize(e.xconfigure.width, e.xconfigure.height);
                else if (e.type == ClientMessage) { shuttingDown=true; running=false; break; }
                else if (e.type == ButtonPress) {
                    if (e.xbutton.button == Button4 || e.xbutton.button == Button5) handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
                else if (e.type == ButtonRelease) {
                    if (currentView == ViewMode::Nougat) nougatOutputSelecting = false;
                    volumeDragging = false;
                }
                else if (e.type == SelectionRequest) handle_clipboard_selection_request(e.xselectionrequest);
                else if (e.type == SelectionClear && e.xselectionclear.selection == clipboardAtom) ownedClipboardText.clear();
                else if (e.type == MotionNotify) {
                    lastMouse=time(nullptr); show_pointer(); handle_nougat_motion(e.xmotion);
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
                        std::size_t count = 0;
                        {
                            std::lock_guard<std::mutex> lock(libraryState->mutex);
                            count = libraryState->nodes.size();
                        }
                        if (ks == XK_Return || ks == XK_KP_Enter || ks == XK_space) {
                            open_selected_library_item();
                        } else if ((ks == XK_Up || ks == XK_Left) && count > 0U) {
                            const LibraryGridMetrics grid = library_grid_metrics();
                            const int amount = ks == XK_Up ? grid.columns : 1;
                            librarySelected = std::max(0, librarySelected - amount);
                            if (librarySelected < libraryScroll) libraryScroll = librarySelected;
                            redraw();
                        } else if ((ks == XK_Down || ks == XK_Right) && count > 0U) {
                            const LibraryGridMetrics grid = library_grid_metrics();
                            const int amount = ks == XK_Down ? grid.columns : 1;
                            librarySelected = std::min(static_cast<int>(count) - 1,
                                                       std::max(0, librarySelected) + amount);
                            if (librarySelected >= libraryScroll + grid.visibleItems) {
                                libraryScroll = librarySelected - grid.visibleItems + grid.columns;
                            }
                            redraw();
                        } else if (ks == XK_r || ks == XK_R) {
                            if (libraryTypeChosen) start_library_task(2);
                        } else if (ks == XK_a || ks == XK_A) {
                            add_library_folder();
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
            poll_ytdlp_process();
            poll_nougat_workers();
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
                if (!fullscreen) redraw();
            }
            double loading_progress = 0.0;
            if (!fullscreen && loading_state(loading_progress) &&
                now_ms() - lastLoadingRedrawMs >= 80) {
                lastLoadingRedrawMs = now_ms();
                redraw();
            }
            if (currentMediaIsYtDlpStream && mp) playback_length_ms();
            if (!fullscreen && currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && now_ms()-lastP2PRedrawMs >= 500) { lastP2PRedrawMs=now_ms(); redraw(); }
            if (pointerInVideo && time(nullptr) - lastMouse >= 3) hide_pointer();
            static time_t lastRedraw=0; time_t now=time(nullptr); if (!fullscreen && currentView == ViewMode::VideoPlayer && now != lastRedraw) { draw_seek_time_only(); lastRedraw=now; }
            fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds); timeval tv; tv.tv_sec=0; tv.tv_usec=100000; select(xfd+1, &fds, nullptr, nullptr, &tv);
        }
    }
    void shutdown() {
        shuttingDown = true;
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
        if (nougatSearchWorker.joinable()) {
            bool busy = false;
            { std::lock_guard<std::mutex> lock(nougatState->mutex); busy = nougatState->search_busy; }
            if (busy) nougatSearchWorker.detach(); else nougatSearchWorker.join();
        }
        if (nougatCrawlWorker.joinable()) {
            bool busy = false;
            { std::lock_guard<std::mutex> lock(nougatState->mutex); busy = nougatState->crawl_busy; }
            if (busy) nougatCrawlWorker.detach(); else nougatCrawlWorker.join();
        }
        nougat.stop_node();
        mediaServer.stop();
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
        if (d) XCloseDisplay(d);
        d=nullptr;
    }
};

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        printf("Nougat Media Suite v0.0.21\n");
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
        if (server.state() != reddmedia::MediaServerState::Stopped) {
            std::fprintf(stderr, "Nougat Media Suite integrated server lifecycle FAIL: stop state.\n");
            return 1;
        }
        std::printf("Nougat Media Suite integrated server graceful shutdown PASS.\n");
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
