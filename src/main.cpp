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
#include <ctime>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <X11/keysym.h>

struct libvlc_instance_t;
struct libvlc_media_t;
struct libvlc_media_player_t;

typedef libvlc_instance_t* (*fn_libvlc_new)(int, const char* const*);
typedef void (*fn_libvlc_release)(libvlc_instance_t*);
typedef libvlc_media_t* (*fn_libvlc_media_new_path)(libvlc_instance_t*, const char*);
typedef void (*fn_libvlc_media_release)(libvlc_media_t*);
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

struct VlcApi {
    void* handle = nullptr;
    fn_libvlc_new new_ = nullptr;
    fn_libvlc_release release = nullptr;
    fn_libvlc_media_new_path media_new_path = nullptr;
    fn_libvlc_media_release media_release = nullptr;
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
        LOAD_SYM(media_release, "libvlc_media_release");
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

class App {
public:
    Display* d=nullptr; int screen=0; Window win=0, video=0; GC gc=0;
    int W=1000,H=650;
    int videoW=980, videoH=530;
    Rect openBtn, rewindBtn, playBtn, stopBtn, forwardBtn, fsBtn, seekRect, volRect, resumeBtn, loadBtn;
    Rect videoResumeBtn, videoLoadBtn;
    VlcApi api; std::string vlcErr;
    libvlc_instance_t* inst=nullptr; libvlc_media_player_t* mp=nullptr;
    bool running=true, paused=false, fullscreen=false, hasMedia=false, needResumePrompt=false;
    std::string currentPath, sessionPath;
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
    std::vector<std::string> contextMenuItems;
    bool pendingSeek=false;
    long long pendingSeekMs=0;
    time_t pendingSeekDeadline=0;

    unsigned long col(unsigned short r, unsigned short g, unsigned short b) {
        XColor color; color.red=r; color.green=g; color.blue=b; color.flags=DoRed|DoGreen|DoBlue;
        XAllocColor(d, DefaultColormap(d, screen), &color); return color.pixel;
    }
    void text(Drawable target, int x, int y, const std::string& s, unsigned long c) {
        XSetForeground(d, gc, c); XDrawString(d, target, gc, x, y, s.c_str(), (int)s.size());
    }
    void fill(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XFillRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void outline(Drawable target, const Rect& r, unsigned long c) { XSetForeground(d,gc,c); XDrawRectangle(d,target,gc,r.x,r.y,r.w,r.h); }
    void line(Drawable target, int x1, int y1, int x2, int y2, unsigned long c) { XSetForeground(d,gc,c); XDrawLine(d,target,gc,x1,y1,x2,y2); }
    void button(const Rect& r, const std::string& label) {
        button_on(win, r, label);
    }
    void button_on(Drawable target, const Rect& r, const std::string& label) {
        fill(target, r, col(0xeeee,0xeeee,0xeeee)); outline(target, r, col(0x7777,0x7777,0x7777));
        text(target, r.x+10, r.y+20, label, col(0x1111,0x1111,0x1111));
    }

    unsigned long icon_pixel(int x, int y, int size) {
        double cx = (size - 1) / 2.0;
        double top = size * 0.12;
        double bottom = size * 0.86;
        double halfWidthAtBottom = size * 0.38;
        if (y < top || y > bottom) return 0x00000000UL;
        double t = (y - top) / (bottom - top);
        double half = halfWidthAtBottom * t;
        double left = cx - half;
        double right = cx + half;
        if (x >= left && x <= right) return 0xffbb0000UL;
        return 0x00000000UL;
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

    bool init() {
        d = XOpenDisplay(nullptr); if (!d) return false;
        screen = DefaultScreen(d);
        unsigned long bg = col(0xdede,0xdede,0xdede);
        win = XCreateSimpleWindow(d, RootWindow(d,screen), 100, 80, W, H, 1, BlackPixel(d,screen), bg);
        XStoreName(d, win, "ReddMedia v0.0.5");
        set_window_identity();
        XSelectInput(d, win, ExposureMask|StructureNotifyMask|ButtonPressMask|KeyPressMask|PointerMotionMask);
        Atom wmDelete = XInternAtom(d, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(d, win, &wmDelete, 1);
        gc = XCreateGC(d, win, 0, nullptr);
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
        return true;
    }
    void layout() {
        int bottomY = H - 36;
        int seekY = H - 72;
        const int gap = 8;
        int x = 10;
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

        volRect = {std::max(fsBtn.x + fsBtn.w + 74, W-170), bottomY+5, 150, 18};
        if (volRect.x + volRect.w > W - 10) {
            volRect.x = std::max(10, W - volRect.w - 10);
        }
        resumeBtn = {W/2-155, H/2+40, 130, 34};
        loadBtn = {W/2+25, H/2+40, 170, 34};
        update_video_prompt_layout();
    }
    void update_video_prompt_layout() {
        videoResumeBtn = {std::max(20, videoW/2-165), std::max(88, videoH/2+36), 130, 34};
        videoLoadBtn = {std::max(20, videoW/2+15), std::max(88, videoH/2+36), 170, 34};
    }
    void apply_video_layout() {
        if (!video) return;
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
    void seek_relative(long long deltaMs) {
        if (!mp) return;
        long long t = api.get_time(mp);
        long long l = api.get_length(mp);
        if (t < 0) t = 0;
        long long nt = t + deltaMs;
        if (nt < 0) nt = 0;
        if (l > 0 && nt > l) nt = l;
        api.set_time(mp, nt);
        if (!fullscreen) draw_seek_time_only();
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
        if (!hasMedia || currentPath.empty()) return;
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
    void cleanup_player() {
        if (mp) { api.stop(mp); api.player_release(mp); mp=nullptr; }
        hasMedia=false; paused=false; pendingSeek=false;
    }
    void open_media(const std::string& path, long long seek=0) {
        if (!inst || !api.media_new_path) return;
        cleanup_player();
        libvlc_media_t* media = api.media_new_path(inst, path.c_str());
        if (!media) return;
        mp = api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return;
        api.set_xwindow(mp, (unsigned int)video);
        api.set_volume(mp, 80);
        currentPath = path; hasMedia=true; paused=false; needResumePrompt=false;
        api.play(mp);
        if (seek > 0) {
            pendingSeek = true;
            pendingSeekMs = seek;
            pendingSeekDeadline = time(nullptr) + 6;
        }
        redraw();
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
        if (mp) { api.stop(mp); paused=false; redraw(); }
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
        unsigned long topText = col(0xffff,0xffff,0xffff);
        fill(target, {0,0,W,26}, companyRed);
        outline(target, {0,24,W,1}, col(0x6600,0x0000,0x0000));
        text(target, 10, 17, "File", topText);
        text(target, 55, 17, "Audio", topText);
        text(target, 112, 17, "Subtitle", topText);
        text(target, W-155, 17, "ReddMedia v0.0.5", topText);
    }

    void draw_seek_time_row(Drawable target) {
        unsigned long dark = col(0x1111,0x1111,0x1111);
        unsigned long companyRed = col(0xbbbb,0x0000,0x0000);
        unsigned long markDark = col(0x3333,0x3333,0x3333);
        unsigned long bg = col(0xdede,0xdede,0xdede);
        fill(target, {0, std::max(0, seekRect.y-6), W, seekRect.h+16}, bg);
        fill(target, seekRect, col(0xeeee,0xeeee,0xeeee));
        outline(target, seekRect, col(0x8888,0x8888,0x8888));
        long long t=0,l=0;
        if (mp) { t=api.get_time(mp); l=api.get_length(mp); }
        if (l > 0) {
            int pos = (int)((double)t / (double)l * seekRect.w);
            fill(target, {seekRect.x, seekRect.y, std::max(1,pos), seekRect.h}, companyRed);
            long long chapterEveryMs = 300000;
            if (l > 7200000) chapterEveryMs = 900000;
            else if (l > 3600000) chapterEveryMs = 600000;
            for (long long markMs = chapterEveryMs; markMs < l; markMs += chapterEveryMs) {
                int mx = seekRect.x + (int)((double)markMs / (double)l * seekRect.w);
                line(target, mx, seekRect.y, mx, seekRect.y + seekRect.h, markDark);
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
        if (!fullscreen) redraw();
    }

    void draw_volume_only() {
        if (!fullscreen) redraw();
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
        XCopyArea(d, buffer, win, gc, 0, 0, W, H, 0, 0);
        XFreePixmap(d, buffer);
        draw_video_message();
        if (contextMenuOpen) draw_context_menu();
        XFlush(d);
    }
    void close_context_menu() {
        if (contextMenuOpen && contextMenu) {
            XDestroyWindow(d, contextMenu);
        }
        contextMenu=0; contextMenuOpen=false;
    }
    void draw_context_menu() {
        if (!contextMenuOpen || !contextMenu) return;
        unsigned long bg = col(0xf4f4,0xf4f4,0xf4f4);
        unsigned long border = col(0x5555,0x5555,0x5555);
        unsigned long dark = col(0x1111,0x1111,0x1111);
        fill(contextMenu, {0,0,190,152}, bg);
        outline(contextMenu, {0,0,189,151}, border);
        for (size_t i=0; i<contextMenuItems.size(); ++i) {
            int y = 26 + (int)i*28;
            text(contextMenu, 12, y, contextMenuItems[i], dark);
        }
    }
    void show_context_menu(Window target, int x, int y) {
        close_context_menu();
        int wx=x, wy=y; Window child=0;
        if (target != win) XTranslateCoordinates(d, target, win, x, y, &wx, &wy, &child);
        const int menuW=190, menuH=152;
        wx = std::max(0, std::min(wx, std::max(0,W-menuW-4)));
        wy = std::max(0, std::min(wy, std::max(0,H-menuH-4)));
        contextMenuItems.clear();
        contextMenuItems.push_back(paused ? "Play" : "Pause");
        contextMenuItems.push_back(fullscreen ? "Exit Fullscreen" : "Fullscreen");
        contextMenuItems.push_back("Rewind 10 seconds");
        contextMenuItems.push_back("Forward 10 seconds");
        contextMenuItems.push_back("Open File");
        contextMenu = XCreateSimpleWindow(d, win, wx, wy, menuW, menuH, 1, BlackPixel(d,screen), col(0xf4f4,0xf4f4,0xf4f4));
        XSelectInput(d, contextMenu, ExposureMask|ButtonPressMask);
        XMapRaised(d, contextMenu);
        contextMenuOpen=true;
        draw_context_menu();
        XFlush(d);
    }
    void handle_context_menu_click(int, int y) {
        int idx = (y - 8) / 28;
        close_context_menu();
        if (idx == 0) toggle_play();
        else if (idx == 1) { if (fullscreen) exit_fullscreen(); else toggle_fullscreen(); }
        else if (idx == 2) seek_relative(-10000);
        else if (idx == 3) seek_relative(10000);
        else if (idx == 4) do_open();
    }
    void handle_button(Window target, int x, int y, unsigned int button, Time eventTime) {
        if (contextMenuOpen && target == contextMenu) { handle_context_menu_click(x,y); return; }
        if (contextMenuOpen) close_context_menu();
        if (target == video) {
            if (button == Button3) { show_context_menu(target, x, y); return; }
            if (button != Button1) return;
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
        if (x < 45 && y < 24) { do_open(); return; }
        if (openBtn.contains(x,y)) { do_open(); return; }
        if (rewindBtn.contains(x,y)) { seek_relative(-10000); return; }
        if (playBtn.contains(x,y)) { toggle_play(); return; }
        if (stopBtn.contains(x,y)) { stop_media(); return; }
        if (forwardBtn.contains(x,y)) { seek_relative(10000); return; }
        if (fsBtn.contains(x,y)) { toggle_fullscreen(); return; }
        if (needResumePrompt && resumeBtn.contains(x,y)) { open_media(sessionPath, sessionTime); return; }
        if (needResumePrompt && loadBtn.contains(x,y)) { needResumePrompt=false; redraw(); do_open(); return; }
        if (seekRect.contains(x,y) && mp) {
            long long l = api.get_length(mp); if (l > 0) api.set_time(mp, (long long)((double)(x-seekRect.x)/seekRect.w*l)); draw_seek_time_only(); return;
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
                    if (e.xbutton.button == Button4) adjust_volume(5);
                    else if (e.xbutton.button == Button5) adjust_volume(-5);
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
                else if (e.type == MotionNotify) { lastMouse=time(nullptr); show_pointer(); }
                else if (e.type == EnterNotify && e.xcrossing.window == video) { pointerInVideo=true; lastMouse=time(nullptr); show_pointer(); }
                else if (e.type == LeaveNotify && e.xcrossing.window == video) { pointerInVideo=false; show_pointer(); }
                else if (e.type == KeyPress) {
                    KeySym ks = XLookupKeysym(&e.xkey, 0);
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
            run_pending_video_click();
            tick_resume_seek();
            if (pointerInVideo && time(nullptr) - lastMouse >= 3) hide_pointer();
            static time_t lastRedraw=0; time_t now=time(nullptr); if (!fullscreen && now != lastRedraw) { draw_seek_time_only(); lastRedraw=now; }
            fd_set fds; FD_ZERO(&fds); FD_SET(xfd, &fds); timeval tv; tv.tv_sec=0; tv.tv_usec=100000; select(xfd+1, &fds, nullptr, nullptr, &tv);
        }
    }
    void shutdown() {
        close_context_menu(); save_session(); cleanup_player(); if (inst) api.release(inst); inst=nullptr;
        if (d) { XCloseDisplay(d); }
        d=nullptr;
    }
};

int main() {
    App app;
    if (!app.init()) return 1;
    app.event_loop();
    app.shutdown();
    return 0;
}
