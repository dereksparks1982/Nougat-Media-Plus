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
#include <algorithm>
#include <vector>
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
typedef void (*fn_libvlc_media_player_pause)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_stop)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_xwindow)(libvlc_media_player_t*, unsigned int);
typedef long long (*fn_libvlc_media_player_get_time)(libvlc_media_player_t*);
typedef void (*fn_libvlc_media_player_set_time)(libvlc_media_player_t*, long long);
typedef long long (*fn_libvlc_media_player_get_length)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_get_volume)(libvlc_media_player_t*);
typedef int (*fn_libvlc_audio_set_volume)(libvlc_media_player_t*, int);

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
    fn_libvlc_media_player_pause pause = nullptr;
    fn_libvlc_media_player_stop stop = nullptr;
    fn_libvlc_media_player_set_xwindow set_xwindow = nullptr;
    fn_libvlc_media_player_get_time get_time = nullptr;
    fn_libvlc_media_player_set_time set_time = nullptr;
    fn_libvlc_media_player_get_length get_length = nullptr;
    fn_libvlc_audio_get_volume get_volume = nullptr;
    fn_libvlc_audio_set_volume set_volume = nullptr;
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
        if (!handle) { err = "VLC/libVLC was not found. Install VLC, then reopen ReddMedia."; return false; }
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
        LOAD_SYM(pause, "libvlc_media_player_pause");
        LOAD_SYM(stop, "libvlc_media_player_stop");
        LOAD_SYM(set_xwindow, "libvlc_media_player_set_xwindow");
        LOAD_SYM(get_time, "libvlc_media_player_get_time");
        LOAD_SYM(set_time, "libvlc_media_player_set_time");
        LOAD_SYM(get_length, "libvlc_media_player_get_length");
        LOAD_SYM(get_volume, "libvlc_audio_get_volume");
        LOAD_SYM(set_volume, "libvlc_audio_set_volume");
#undef LOAD_SYM
#define LOAD_OPTIONAL(field, name) do { field = (decltype(field))dlsym(handle, name); } while(0)
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
    P2pUrlCut, P2pUrlCopy, P2pUrlPaste
};
enum class ViewMode { VideoPlayer, YtDlp, P2P };
enum class YtDlpJob { Idle, Download };
struct MenuItem {
    std::string label;
    MenuAction action = MenuAction::NoAction;
    int value = 0;
};
struct TrackChoice { int id = -1; std::string name; };
class App {
public:
    Display* d=nullptr; int screen=0; Window win=0, video=0; GC gc=0; XFontStruct* fontInfo=nullptr;
    int W=1000,H=650;
    int videoW=980, videoH=530;
    Rect openBtn, rewindBtn, playBtn, stopBtn, forwardBtn, fsBtn, seekRect, volRect, resumeBtn, loadBtn;
    Rect videoResumeBtn, videoLoadBtn;
    Rect videoPlayerTab, ytdlpTab, p2pTab;
    Rect ytdlpUrlRect, ytdlpOutputRect, ytdlpDownloadBtn, ytdlpPlayBtn, ytdlpClearBtn, ytdlpFolderBtn;
    Rect p2pMagnetRect, p2pOutputRect, p2pLoadMagnetBtn, p2pOpenTorrentBtn, p2pPlayBtn, p2pStopResumeBtn;
    VlcApi api; std::string vlcErr;
    libvlc_instance_t* inst=nullptr; libvlc_media_player_t* mp=nullptr;
    bool running=true, paused=false, fullscreen=false, hasMedia=false, needResumePrompt=false;
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
    long long lastChapterScanMs=0;
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
    long long ytdlpCachedThroughMs = 0;
    int controlsScrollX = 0;
    P2PEngine p2p;
    P2PStreamServer p2pStream{p2p};
    bool p2pMagnetFocused=false;
    bool p2pMagnetSelectAll=false;
    std::string p2pMagnet;
    std::string p2pOutputFolder = home_dir() + "/Downloads";
    std::string p2pUiStatus = "Ready.";
    std::vector<Rect> p2pFileRows;
    long long lastP2PRedrawMs=0;

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
    void fill(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XFillRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void outline(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XDrawRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void line(Drawable target, int x1, int y1, int x2, int y2, unsigned long c) { XSetForeground(d,gc,c); XDrawLine(d,target,gc,x1,y1,x2,y2); }
    void button(const Rect& r, const std::string& label) {
        button_on(win, r, label);
    }
    void button_on(Drawable target, const Rect& r, const std::string& label) {
        unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        unsigned long darkRed = col(0x6600,0x0000,0x0000);
        unsigned long lightRed = col(0xffff,0x2222,0x2222);
        unsigned long white = col(0xffff,0xffff,0xffff);
        fill(target, r, companyRed);
        outline(target, r, darkRed);
        line(target, r.x + 1, r.y + 1, r.x + (int)r.w - 2, r.y + 1, lightRed);
        text(target, r.x+10, r.y+20, label, white);
    }

    unsigned long icon_pixel(int x, int y, int size) {
        constexpr double pi = 3.14159265358979323846;
        const double cx = (size - 1) / 2.0;
        const double cy = (size - 1) / 2.0;
        const double outer = size * 0.43;
        const double inner = size * 0.19;
        double vx[10]{};
        double vy[10]{};
        for (int i = 0; i < 10; ++i) {
            const double radius = (i % 2 == 0) ? outer : inner;
            const double angle = -pi / 2.0 + i * pi / 5.0;
            vx[i] = cx + std::cos(angle) * radius;
            vy[i] = cy + std::sin(angle) * radius;
        }
        bool inside = false;
        for (int i = 0, j = 9; i < 10; j = i++) {
            const bool crosses = ((vy[i] > y) != (vy[j] > y));
            if (!crosses) continue;
            const double edgeX = (vx[j] - vx[i]) * (y - vy[i]) / (vy[j] - vy[i]) + vx[i];
            if (x < edgeX) inside = !inside;
        }
        return inside ? 0xffbb0000UL : 0x00000000UL;
    }

    void set_net_wm_icon() {
        std::vector<unsigned long> data;
        const int sizes[] = {16, 32, 64};
        for (int size : sizes) {
            data.push_back((unsigned long)size);
            data.push_back((unsigned long)size);
            for (int y=0; y<size; ++y) {
                for (int x=0; x<size; ++x) data.push_back(icon_pixel(x, y, size));
            }
        }
        Atom netWmIcon = XInternAtom(d, "_NET_WM_ICON", False);
        Atom cardinal = XInternAtom(d, "CARDINAL", False);
        XChangeProperty(d, win, netWmIcon, cardinal, 32, PropModeReplace,
                        reinterpret_cast<unsigned char*>(data.data()), (int)data.size());
    }

    void set_window_identity() {
        XClassHint classHint;
        classHint.res_name = const_cast<char*>("reddmedia");
        classHint.res_class = const_cast<char*>("ReddMedia");
        XSetClassHint(d, win, &classHint);
        set_net_wm_icon();
    }

    void set_window_title() {
        const char* title = "\xE2\x98\x85 ReddMedia";
        XStoreName(d, win, title);
        Atom netWmName = XInternAtom(d, "_NET_WM_NAME", False);
        Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
        XChangeProperty(d, win, netWmName, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(title),
                        (int)std::strlen(title));
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
        XSelectInput(d, win, ExposureMask|StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask);
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
        std::string p2pRestoreError;
        if (p2p.restore_last(p2pRestoreError)) p2pUiStatus = "Previous P2P download restored.";
        else if (!p2pRestoreError.empty()) p2pUiStatus = p2pRestoreError;
        return true;
    }
    void layout() {
        videoPlayerTab = {0,0,118,26};
        ytdlpTab = {118,0,82,26};
        p2pTab = {200,0,64,26};
        int bottomY = H - 36;
        int seekY = H - 72;
        const int gap = 8;
        int totalButtonsW = 78 + gap + 116 + gap + 104 + gap + 70 + gap + 148 + gap + 104;
        int availableButtonW = std::max(160, W - 200);
        int maxScroll = std::max(0, totalButtonsW - availableButtonW);
        controlsScrollX = std::max(0, std::min(controlsScrollX, maxScroll));
        int x = 10 - controlsScrollX;
        openBtn = {x, bottomY, 78, 28}; x += openBtn.w + gap;
        rewindBtn = {x, bottomY, 116, 28}; x += rewindBtn.w + gap;
        playBtn = {x, bottomY, 104, 28}; x += playBtn.w + gap;
        stopBtn = {x, bottomY, 70, 28}; x += stopBtn.w + gap;
        forwardBtn = {x, bottomY, 148, 28}; x += forwardBtn.w + gap;
        fsBtn = {x, bottomY, 104, 28};

        int currentTimeWidth = 64;
        int totalTimeWidth = 74;
        int seekX = 10 + currentTimeWidth + 10;
        int seekRightPad = totalTimeWidth + 20;
        seekRect = {seekX, seekY, std::max(220, W-seekX-seekRightPad), 18};

        volRect = {std::max(10, W-170), bottomY+5, 150, 18};
        resumeBtn = {W/2-155, H/2+40, 130, 34};
        loadBtn = {W/2+25, H/2+40, 170, 34};
        ytdlpUrlRect = {28, 118, std::max(240, W-56), 28};
        ytdlpOutputRect = {28, 158, std::max(240, W-56), 28};
        ytdlpDownloadBtn = {28, 204, 130, 32};
        ytdlpPlayBtn = {170, 204, 110, 32};
        ytdlpFolderBtn = {0, 0, 0, 0};
        ytdlpClearBtn = {292, 204, 130, 32};
        p2pMagnetRect = {28, 112, std::max(240, W-56), 28};
        p2pOutputRect = {28, 152, std::max(240, W-56), 28};
        p2pLoadMagnetBtn = {28, 198, 138, 32};
        p2pOpenTorrentBtn = {178, 198, 138, 32};
        p2pPlayBtn = {328, 198, 100, 32};
        p2pStopResumeBtn = {440, 198, 160, 32};
        update_video_prompt_layout();
    }
    void update_video_prompt_layout() {
        videoResumeBtn = {std::max(20, videoW/2-165), std::max(88, videoH/2+36), 130, 34};
        videoLoadBtn = {std::max(20, videoW/2+15), std::max(88, videoH/2+36), 170, 34};
    }
    void apply_video_layout() {
        if (!video) return;
        if (currentView == ViewMode::YtDlp || currentView == ViewMode::P2P) {
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
            videoH = std::max(100, H-156);
            XMoveResizeWindow(d, video, 10, 42, videoW, videoH);
        }
        update_video_prompt_layout();
    }
    void adjust_volume(int delta) {
        if (!mp) return;
        int vol = api.get_volume(mp);
        if (vol < 0) vol = 80;
        vol = std::max(0, std::min(100, vol + delta));
        api.set_volume(mp, vol);
        if (!fullscreen) draw_volume_only();
    }
    long long playback_time_ms() {
        if (!mp) return 0;
        long long t = api.get_time(mp);
        if (t < 0) t = 0;
        if (currentMediaIsYtDlpStream) return ytdlpStreamBaseMs + t;
        return t;
    }
    long long playback_length_ms() {
        if (!mp) return 0;
        long long l = api.get_length(mp);
        if (!currentMediaIsYtDlpStream) return l;
        if (l > 0 && ytdlpTotalDurationMs <= 0) {
            ytdlpTotalDurationMs = ytdlpStreamBaseMs > 0 ? ytdlpStreamBaseMs + l : l;
        }
        return ytdlpTotalDurationMs > 0 ? ytdlpTotalDurationMs : (l > 0 ? ytdlpStreamBaseMs + l : 0);
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

    bool start_ytdlp_cache_playback_at(long long targetMs) {
        if (ytdlpUrl.empty()) return false;
        std::string engine = ytdlp_engine_path();
        std::string error;
        cleanup_player();
        if (!ytdlpStream.start(engine, ytdlpUrl, targetMs, error)) {
            ytdlpStatus = error.empty() ? "Could not start YouTube cache bridge." : error;
            switch_view(ViewMode::YtDlp);
            redraw();
            return false;
        }
        if (!ytdlpStream.wait_for_initial_cache(256 * 1024, 15000, error)) {
            append_ytdlp_log(ytdlpStream.take_log());
            ytdlpStream.stop();
            ytdlpStatus = error.empty() ? "YouTube cache did not buffer enough media." : error;
            switch_view(ViewMode::YtDlp);
            redraw();
            return false;
        }
        append_ytdlp_log(ytdlpStream.take_log());
        ytdlpStreamBaseMs = std::max<long long>(0, targetMs);
        ytdlpCachedThroughMs = ytdlpStreamBaseMs;
        if (!open_ytdlp_cache_location(ytdlpStream.url(), ytdlpStreamBaseMs)) {
            ytdlpStream.stop();
            ytdlpStatus = "VLC could not start the YouTube cache stream. See log.";
            switch_view(ViewMode::YtDlp);
            redraw();
            return false;
        }
        ytdlpStatus = targetMs > 0 ? "Seek buffered. Playing through local cache bridge."
                                   : "Playing through local cache bridge at up to 1080p.";
        return true;
    }
    void seek_to_ms(long long targetMs) {
        if (!mp) return;
        long long l = playback_length_ms();
        if (targetMs < 0) targetMs = 0;
        if (l > 0 && targetMs > l) targetMs = l;
        if (currentMediaIsYtDlpStream) {
            if (targetMs >= ytdlpStreamBaseMs && targetMs <= ytdlpCachedThroughMs) {
                api.set_time(mp, targetMs - ytdlpStreamBaseMs);
                if (!fullscreen) draw_seek_time_only();
                return;
            }
            start_ytdlp_cache_playback_at(targetMs);
            return;
        }
        api.set_time(mp, targetMs);
        if (!fullscreen) draw_seek_time_only();
    }
    void seek_relative(long long deltaMs) {
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
        long long t = mp ? api.get_time(mp) : 0;
        std::ofstream f(session_file());
        f << "{\n";
        f << "  \"path\": \"" << json_escape(currentPath) << "\",\n";
        f << "  \"title\": \"" << json_escape(basename_only(currentPath)) << "\",\n";
        f << "  \"time_ms\": " << t << ",\n";
        f << "  \"saved_at\": " << (long long)time(nullptr) << "\n";
        f << "}\n";
    }
    void stop_ytdlp_stream_process() {
        ytdlpStream.stop();
        ytdlpStreamBaseMs = 0;
        ytdlpCachedThroughMs = 0;
    }
    void cleanup_player() {
        if (mp) { api.stop(mp); api.player_release(mp); mp=nullptr; }
        if (currentMediaIsYtDlpStream) stop_ytdlp_stream_process();
        currentMediaIsYtDlpStream=false;
        hasMedia=false; paused=false; pendingSeek=false; subtitlesOn=false; chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false;
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
        api.set_volume(mp, 80);
        currentPath = path; currentMediaIsP2P=false; currentMediaIsNetwork=false; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; lastChapterScanMs=0;
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
        if (!inst || !api.media_new_location || url.empty()) return false;
        cleanup_player();
        libvlc_media_t* media = api.media_new_location(inst, url.c_str());
        if (!media) return false;
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, 80);
        currentPath.clear(); currentMediaIsP2P=true; currentMediaIsNetwork=true; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; lastChapterScanMs=0;
        api.play(mp);
        redraw();
        return true;
    }
    bool open_ytdlp_cache_location(const std::string& url, long long baseMs) {
        if (!inst || !api.media_new_location || url.empty()) return false;
        p2pStream.stop();
        libvlc_media_t* media = api.media_new_location(inst, url.c_str());
        if (!media) return false;
        if (api.media_add_option) api.media_add_option(media, ":network-caching=2500");
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, 80);
        ytdlpStreamBaseMs = std::max<long long>(0, baseMs);
        currentPath.clear(); currentMediaIsP2P=false; currentMediaIsNetwork=true; currentMediaIsYtDlpStream=true; hasMedia=true; paused=false; needResumePrompt=false;
        subtitlePath.clear(); subtitlesOn=false; subtitleDelayUs=0;
        chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; lastChapterScanMs=0;
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
    void do_open() {
        std::string p = choose_file_dialog();
        if (!p.empty()) open_media(p, 0);
    }
    void toggle_play() {
        if (!mp) return;
        api.pause(mp); paused=!paused; redraw();
    }
    void stop_media() {
        if (currentMediaIsYtDlpStream) {
            cleanup_player();
            currentMediaIsNetwork=false;
            ytdlpStatus="Play stream stopped.";
            redraw();
            return;
        }
        if (mp) { api.stop(mp); paused=false; redraw(); }
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
    void draw_top_bar(Drawable target) {
        unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        unsigned long activeRed = col(0x9900,0x0000,0x0000);
        unsigned long topText = col(0xffff,0xffff,0xffff);
        fill(target, {0,0,W,26}, companyRed);
        if (currentView == ViewMode::VideoPlayer) fill(target, videoPlayerTab, activeRed);
        if (currentView == ViewMode::YtDlp) fill(target, ytdlpTab, activeRed);
        if (currentView == ViewMode::P2P) fill(target, p2pTab, activeRed);
        outline(target, {0,24,W,1}, col(0x6600,0x0000,0x0000));
        line(target, videoPlayerTab.x + videoPlayerTab.w, 0, videoPlayerTab.x + videoPlayerTab.w, 25, col(0x6600,0x0000,0x0000));
        line(target, ytdlpTab.x + ytdlpTab.w, 0, ytdlpTab.x + ytdlpTab.w, 25, col(0x6600,0x0000,0x0000));
        line(target, p2pTab.x + p2pTab.w, 0, p2pTab.x + p2pTab.w, 25, col(0x6600,0x0000,0x0000));
        text(target, 10, 17, "Video Player", topText);
        text(target, 132, 17, "YouTube", topText);
        text(target, 216, 17, "P2P", topText);
        text(target, W-66, 17, "v0.0.12", topText);
    }

    void update_chapter_marks(bool force=false) {
        if (!mp) { chapterMarksMs.clear(); chapterNames.clear(); chapterMarksAreReal=false; return; }
        long long l = api.get_length(mp);
        if (l <= 0) return;
        long long n = now_ms();
        if (!force && n - lastChapterScanMs < 2500 && !chapterMarksMs.empty()) return;
        lastChapterScanMs = n;

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
                api.chapter_descriptions_release(chapters, (unsigned)count);
            }
        }
        if (realMarks.size() > 1) {
            chapterMarksMs = realMarks;
            chapterNames = realNames;
            chapterMarksAreReal = true;
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
    }
    void jump_to_chapter_index(int idx) {
        if (!mp || !chapterMarksAreReal || idx < 0 || idx >= (int)chapterMarksMs.size()) return;
        api.set_time(mp, chapterMarksMs[(size_t)idx]);
        draw_seek_time_only();
    }
    int current_chapter_index() {
        if (!mp || !chapterMarksAreReal || chapterMarksMs.empty()) return -1;
        long long t = api.get_time(mp);
        int idx = 0;
        for (size_t i=0; i<chapterMarksMs.size(); ++i) if (chapterMarksMs[i] <= t) idx = (int)i;
        return idx;
    }
    void previous_chapter() {
        if (!chapterMarksAreReal) return;
        int idx = current_chapter_index();
        long long t = mp ? api.get_time(mp) : 0;
        if (idx > 0 && idx < (int)chapterMarksMs.size() && t - chapterMarksMs[(size_t)idx] < 3000) idx--;
        jump_to_chapter_index(std::max(0, idx));
    }
    void next_chapter() {
        if (!chapterMarksAreReal) return;
        int idx = current_chapter_index();
        jump_to_chapter_index(std::min((int)chapterMarksMs.size()-1, idx+1));
    }

    void draw_seek_time_row(Drawable target) {
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        unsigned long markDark = col(0x3333,0x3333,0x3333);
        unsigned long markReal = col(0x0000,0x0000,0x0000);
        unsigned long bg = col(0xdede,0xdede,0xdede);
        fill(target, {0, std::max(0, seekRect.y-6), W, seekRect.h+16}, bg);
        fill(target, seekRect, col(0xeeee,0xeeee,0xeeee));
        outline(target, seekRect, col(0x8888,0x8888,0x8888));
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
        text(target, 10, seekRect.y+14, format_time(t), dark);
        int totalX = seekRect.x + seekRect.w + 10;
        text(target, totalX, seekRect.y+14, format_time(l), dark);
    }

    void draw_volume_bar(Drawable target) {
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        unsigned long bg = col(0xdede,0xdede,0xdede);
        fill(target, {std::max(0, volRect.x-62), std::max(0, volRect.y-6), volRect.w+72, volRect.h+16}, bg);
        fill(target, volRect, col(0xeeee,0xeeee,0xeeee));
        outline(target, volRect, col(0x8888,0x8888,0x8888));
        int vol = mp ? api.get_volume(mp) : 80;
        vol = std::max(0,std::min(100,vol));
        fill(target, {volRect.x, volRect.y, volRect.w*vol/100, volRect.h}, companyRed);
        text(target, volRect.x-56, volRect.y+14, "Volume", dark);
    }

    void draw_controls(Drawable target) {
        draw_top_bar(target);
        if (currentView != ViewMode::VideoPlayer) return;
        button_on(target, openBtn, "Open");
        button_on(target, rewindBtn, "Rewind 10s");
        button_on(target, playBtn, "Play/Pause");
        button_on(target, stopBtn, "Stop");
        button_on(target, forwardBtn, "Fast Forward 10s");
        button_on(target, fsBtn, "Fullscreen");
        draw_seek_time_row(target);
        draw_volume_bar(target);
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

    void draw_yt_dlp_screen(Drawable target) {
        unsigned long bg = col(0xdede,0xdede,0xdede);
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long border = urlFocused ? col(0xbbbb,0x0000,0x0000) : col(0x7777,0x7777,0x7777);
        fill(target, {0,26,W,H-26}, bg);
        text(target, 28, 68, "YouTube", dark);
        text(target, 28, 92, "Download or play YouTube videos.", dark);
        fill(target, ytdlpUrlRect, col(0xffff,0xffff,0xffff));
        outline(target, ytdlpUrlRect, border);
        text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y-8, "URL", dark);
        int urlTextMax = std::max(24, ytdlpUrlRect.w - 18);
        std::string visibleUrl = ytdlpUrl.empty() ? std::string("") : tail_to_width(ytdlpUrl, urlTextMax);
        XRectangle urlClip;
        urlClip.x = (short)(ytdlpUrlRect.x + 5);
        urlClip.y = (short)(ytdlpUrlRect.y + 2);
        urlClip.width = (unsigned short)std::max(1, ytdlpUrlRect.w - 10);
        urlClip.height = (unsigned short)std::max(1, ytdlpUrlRect.h - 4);
        XSetClipRectangles(d, gc, 0, 0, &urlClip, 1, Unsorted);
        if (visibleUrl.empty() && !urlFocused) {
            std::string hint = "click here, then Ctrl+V or right-click";
            text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y+18, hint, col(0x5555,0x5555,0x5555));
        } else if (urlSelectAll && !visibleUrl.empty()) {
            int selectedW = std::min(text_width(visibleUrl) + 4, std::max(1, ytdlpUrlRect.w - 12));
            fill(target, {ytdlpUrlRect.x+6, ytdlpUrlRect.y+4, selectedW, ytdlpUrlRect.h-8}, col(0x3333,0x6666,0xaaaa));
            text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y+18, visibleUrl, col(0xffff,0xffff,0xffff));
        } else {
            text(target, ytdlpUrlRect.x+8, ytdlpUrlRect.y+18, visibleUrl, dark);
        }
        if (urlFocused && !urlSelectAll) {
            int cx = ytdlpUrlRect.x + 8 + text_width(visibleUrl);
            int rightEdge = ytdlpUrlRect.x + ytdlpUrlRect.w - 8;
            if (cx > rightEdge) cx = rightEdge;
            line(target, cx, ytdlpUrlRect.y+5, cx, ytdlpUrlRect.y+23, dark);
        }
        XSetClipMask(d, gc, None);
        fill(target, ytdlpOutputRect, col(0xffff,0xffff,0xffff));
        outline(target, ytdlpOutputRect, col(0x7777,0x7777,0x7777));
        std::string outLine = "Output folder: " + ytdlpOutputFolder;
        if ((int)outLine.size() > std::max(12, (ytdlpOutputRect.w-14)/8)) outLine = outLine.substr(0, std::max(12, (ytdlpOutputRect.w-14)/8));
        text(target, ytdlpOutputRect.x+8, ytdlpOutputRect.y+18, outLine, dark);
        button_on(target, ytdlpDownloadBtn, "Download");
        button_on(target, ytdlpPlayBtn, "Play");
        button_on(target, ytdlpClearBtn, "Clear Log");
        text(target, 28, 258, "Status: " + ytdlpStatus, dark);
        Rect logBox = {28, 280, std::max(240, W-56), std::max(100, H-305)};
        fill(target, logBox, col(0xf7f7,0xf7f7,0xf7f7));
        outline(target, logBox, col(0x7777,0x7777,0x7777));
        text(target, logBox.x+8, logBox.y+20, "YouTube activity log", dark);
        int lineY = logBox.y + 44;
        std::istringstream iss(ytdlpLog);
        std::string lineText;
        std::vector<std::string> lines;
        while (std::getline(iss, lineText)) lines.push_back(lineText);
        int maxLines = std::max(1, (logBox.h - 52) / 18);
        int start = std::max(0, (int)lines.size() - maxLines);
        for (int i=start; i<(int)lines.size() && lineY < logBox.y + logBox.h - 8; ++i) {
            std::string ln = lines[(size_t)i];
            if ((int)ln.size() > std::max(10, (logBox.w-18)/8)) ln = ln.substr(0, std::max(10, (logBox.w-18)/8));
            text(target, logBox.x+8, lineY, ln, dark);
            lineY += 18;
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
        unsigned long bg = col(0xdede,0xdede,0xdede);
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long border = p2pMagnetFocused ? col(0xbbbb,0x0000,0x0000) : col(0x7777,0x7777,0x7777);
        fill(target, {0,26,W,H-26}, bg);
        text(target, 28, 62, "P2P Streaming", dark);
        text(target, 28, 84, "Open a P2P source, choose a video, then Play.", dark);
        fill(target, p2pMagnetRect, col(0xffff,0xffff,0xffff));
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
        } else text(target,p2pMagnetRect.x+8,p2pMagnetRect.y+18,visible,dark);
        if (p2pMagnetFocused && !p2pMagnetSelectAll) {
            int cx=p2pMagnetRect.x+8+text_width(visible);
            cx=std::min(cx,p2pMagnetRect.x+p2pMagnetRect.w-8);
            line(target,cx,p2pMagnetRect.y+5,cx,p2pMagnetRect.y+23,dark);
        }
        XSetClipMask(d,gc,None);
        fill(target,p2pOutputRect,col(0xffff,0xffff,0xffff));
        outline(target,p2pOutputRect,col(0x7777,0x7777,0x7777));
        text(target,p2pOutputRect.x+8,p2pOutputRect.y+18,tail_to_width("Download folder: "+p2pOutputFolder,p2pOutputRect.w-16),dark);
        button_on(target,p2pLoadMagnetBtn,"Load Magnet");
        button_on(target,p2pOpenTorrentBtn,"Open .torrent");
        button_on(target,p2pPlayBtn,"Play");
        button_on(target,p2pStopResumeBtn,p2p.is_paused()?"Resume Download":"Stop Download");

        auto_select_single_video();
        P2PStatus st=p2p.status();
        int y=254;
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
        fill(target,fileBox,col(0xf7f7,0xf7f7,0xf7f7)); outline(target,fileBox,col(0x7777,0x7777,0x7777));
        text(target,fileBox.x+8,fileBox.y+19,"P2P files",dark);
        int rowY=fileBox.y+30;
        const int selected=p2p.selected_file();
        for (const P2PFileInfo& f:fs) {
            if (rowY+24>fileBox.y+fileBox.h) break;
            Rect row={fileBox.x+6,rowY,fileBox.w-12,24};
            if (f.index==selected) fill(target,row,col(0xdddd,0xeeee,0xffff));
            outline(target,row,col(0xcccc,0xcccc,0xcccc));
            std::string label=(f.video?"[video] ":"[file] ")+f.path+"  ("+format_bytes((std::int64_t)f.size)+")";
            text(target,row.x+6,row.y+17,tail_to_width(label,row.w-12),dark);
            p2pFileRows.push_back(row);
            rowY+=26;
        }
        if (fs.empty()) text(target,fileBox.x+8,fileBox.y+46,st.active?"Waiting for P2P metadata...":"Load a magnet or P2P metadata file.",col(0x5555,0x5555,0x5555));
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
        p2pUiStatus="Streaming. ReddMedia will prioritize pieces around playback and seeks.";
        switch_view(ViewMode::VideoPlayer);
        if (!open_p2p_stream_location(p2pStream.url())) {
            p2pStream.stop();
            p2pUiStatus="VLC could not open the local P2P stream.";
            switch_view(ViewMode::P2P);
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
        if (currentView == ViewMode::YtDlp) draw_yt_dlp_screen(buffer);
        if (currentView == ViewMode::P2P) draw_p2p_screen(buffer);
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
                if (currentView == ViewMode::YtDlp) redraw();
                ytdlpProcessOutput.clear();
            }
        }
        if (ytdlpStream.running()) {
            ytdlpStream.poll();
            append_ytdlp_log(ytdlpStream.take_log());
            if (ytdlpStream.failed()) {
                ytdlpStatus="YouTube cache feeder ended with an error. See log.";
                if (currentView==ViewMode::YtDlp) redraw();
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
        if (!exists_file(engine)) { ytdlpStatus = "YouTube engine is missing."; redraw(); return; }
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
        if (pid < 0) { close(pipefd[0]); ytdlpStatus = "Could not start YouTube download."; redraw(); return; }
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
        if (ytdlpPid > 0) { ytdlpStatus = "YouTube download is already running."; redraw(); return; }
        if (ytdlpUrl.empty()) { ytdlpStatus = "Paste or type a URL first."; redraw(); return; }
        std::string engine = ytdlp_engine_path();
        if (!exists_file(engine)) { ytdlpStatus = "YouTube engine is missing."; redraw(); return; }

        ytdlpTotalDurationMs = 0;
        ytdlpLog.clear();
        ytdlpStatus = "Reading YouTube duration...";
        redraw();
        ytdlpTotalDurationMs = resolve_ytdlp_duration_ms(engine, ytdlpUrl);
        ytdlpStatus = "Buffering YouTube cache at up to 1080p...";
        redraw();
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
    void scroll_bottom_controls(int delta) {
        int totalButtonsW = 78 + 8 + 116 + 8 + 104 + 8 + 70 + 8 + 148 + 8 + 104;
        int availableButtonW = std::max(160, W - 200);
        int maxScroll = std::max(0, totalButtonsW - availableButtonW);
        controlsScrollX = std::max(0, std::min(maxScroll, controlsScrollX + delta));
        layout();
        redraw();
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
        items.push_back({"Exit ReddMedia", MenuAction::ExitApp, 0});
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
            case MenuAction::ExitApp: running=false; break;
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
        if (currentView == ViewMode::VideoPlayer && target == win && y >= H-42 && !volRect.contains(x,y)) {
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
        if (currentView == ViewMode::YtDlp && target == win && ytdlpUrlRect.contains(x,y) && button == Button3) {
            urlFocused = true;
            XSetInputFocus(d, win, RevertToParent, CurrentTime);
            redraw();
            show_url_context_menu(x, y);
            return;
        }
        if (currentView == ViewMode::P2P && target == win && p2pMagnetRect.contains(x,y) && button == Button3) {
            p2pMagnetFocused=true;
            XSetInputFocus(d,win,RevertToParent,CurrentTime);
            redraw(); show_p2p_url_context_menu(x,y); return;
        }
        if (target == video) {
            if (button == Button3) { show_context_menu(target, x, y); return; }
            if (button != Button1 && !(currentView == ViewMode::YtDlp && ytdlpUrlRect.contains(x,y) && button == Button3)) return;
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
            items.push_back({"Exit ReddMedia", MenuAction::ExitApp, 0});
            show_menu(win, 8, 26, items);
            return;
        }
        if (y < 26 && ytdlpTab.contains(x,y)) {
            if (currentView != ViewMode::YtDlp) { switch_view(ViewMode::YtDlp); return; }
            show_ytdlp_menu(ytdlpTab.x, 26);
            return;
        }
        if (y < 26 && p2pTab.contains(x,y)) {
            if (currentView != ViewMode::P2P) { switch_view(ViewMode::P2P); return; }
            return;
        }
        if (currentView == ViewMode::P2P) {
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
        if (currentView == ViewMode::YtDlp) {
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
            if (ytdlpClearBtn.contains(x,y)) { urlFocused=false; urlSelectAll=false; ytdlpLog = "No download output yet."; ytdlpStatus = "Ready."; redraw(); return; }
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
            int v = std::max(0, std::min(100, (x-volRect.x)*100/volRect.w)); api.set_volume(mp, v); draw_volume_only(); return;
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
                else if (e.type == ClientMessage) running=false;
                else if (e.type == ButtonPress) {
                    if (e.xbutton.button == Button4 || e.xbutton.button == Button5) handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
                else if (e.type == SelectionRequest) handle_clipboard_selection_request(e.xselectionrequest);
                else if (e.type == SelectionClear && e.xselectionclear.selection == clipboardAtom) ownedClipboardText.clear();
                else if (e.type == MotionNotify) { lastMouse=time(nullptr); show_pointer(); }
                else if (e.type == EnterNotify && e.xcrossing.window == video) { pointerInVideo=true; lastMouse=time(nullptr); show_pointer(); }
                else if (e.type == LeaveNotify && e.xcrossing.window == video) { pointerInVideo=false; show_pointer(); }
                else if (e.type == KeyPress) {
                    KeySym ks = XLookupKeysym(&e.xkey, 0);
                    if (currentView == ViewMode::YtDlp && urlFocused) {
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
                    } else if (currentView == ViewMode::P2P && p2pMagnetFocused) {
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
            poll_ytdlp_process();
            if (currentMediaIsYtDlpStream && mp) {
                const long long current = playback_time_ms();
                if (current > ytdlpCachedThroughMs) ytdlpCachedThroughMs = current;
                playback_length_ms();
            }
            if (!fullscreen && currentView == ViewMode::P2P && now_ms()-lastP2PRedrawMs >= 500) { lastP2PRedrawMs=now_ms(); redraw(); }
            if (pointerInVideo && time(nullptr) - lastMouse >= 3) hide_pointer();
            static time_t lastRedraw=0; time_t now=time(nullptr); if (!fullscreen && currentView == ViewMode::VideoPlayer && now != lastRedraw) { draw_seek_time_only(); lastRedraw=now; }
            fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds); timeval tv; tv.tv_sec=0; tv.tv_usec=100000; select(xfd+1, &fds, nullptr, nullptr, &tv);
        }
    }
    void shutdown() {
        stop_ytdlp_process(); stop_ytdlp_stream_process(); close_context_menu(); save_session(); p2pStream.stop(); p2p.shutdown(); cleanup_player(); if (inst) api.release(inst); inst=nullptr;
        if (d) { XCloseDisplay(d); }
        d=nullptr;
    }
};

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        printf("ReddMedia v0.0.12\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--p2p-engine-info") {
        P2PEngine engine;
        printf("%s\n", engine.libtorrent_version().c_str());
        engine.shutdown();
        return 0;
    }
    App app;
    if (!app.init()) return 1;
    app.event_loop();
    app.shutdown();
    return 0;
}
