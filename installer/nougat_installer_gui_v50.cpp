#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "nougat_media_suite_icon_data.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr int kWidth = 820;
constexpr int kHeight = 650;
constexpr int kPluginRowHeight = 58;
constexpr const char* kInstallerVersion = "Nougat Media Suite Installer v0.0.50";
constexpr const char* kInstalledCore = "/opt/nougat-media-suite/Nougat_Media_Suite_v50";

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

struct Plugin {
    std::string id;
    std::string name;
    std::string description;
    bool recommended = false;
    bool selected = false;
};

enum class InstallMode { Default, Custom, Advanced };
enum class Page { Selection, Installing, Complete, Failed };

std::string read_text(const fs::path& path) {
    std::ifstream in(path);
    if (!in) return {};
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string json_string(const std::string& text, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    return std::regex_search(text, match, pattern) && match.size() >= 2 ? match[1].str() : std::string();
}

bool json_bool(const std::string& text, const std::string& key, bool fallback = false) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) return fallback;
    return match[1].str() == "true";
}

fs::path executable_path() {
    std::vector<char> buffer(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length <= 0) return fs::current_path();
    return fs::path(std::string(buffer.data(), static_cast<std::size_t>(length)));
}

std::vector<Plugin> load_plugins(const fs::path& payload_root) {
    std::vector<Plugin> plugins;
    const fs::path root = payload_root / "plugins";
    std::error_code ec;
    if (!fs::is_directory(root, ec) || ec) return plugins;

    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (ec || !entry.is_directory()) continue;
        const fs::path manifest = entry.path() / "plugin.json";
        if (!fs::is_regular_file(manifest, ec) || ec) continue;
        const std::string raw = read_text(manifest);
        if (json_string(raw, "format") != "NOUGAT_PLUGIN") continue;
        const std::string id = json_string(raw, "id");
        if (id.empty() || id == "." || id == ".." || id.find('/') != std::string::npos || id.find('\\') != std::string::npos) continue;
        Plugin plugin;
        plugin.id = id;
        plugin.name = json_string(raw, "display_name");
        if (plugin.name.empty()) plugin.name = id;
        plugin.description = json_string(raw, "description");
        plugin.recommended = json_bool(raw, "recommended_by_default", false);
        plugin.selected = plugin.recommended;
        plugins.push_back(std::move(plugin));
    }
    std::sort(plugins.begin(), plugins.end(), [](const Plugin& left, const Plugin& right) {
        return left.name < right.name;
    });
    return plugins;
}

std::string selected_ids(const std::vector<Plugin>& plugins) {
    std::ostringstream out;
    bool first = true;
    for (const auto& plugin : plugins) {
        if (!plugin.selected) continue;
        if (!first) out << ',';
        out << plugin.id;
        first = false;
    }
    return out.str();
}

std::string mode_name(InstallMode mode) {
    if (mode == InstallMode::Default) return "default";
    if (mode == InstallMode::Advanced) return "advanced";
    return "custom";
}

unsigned long color(Display* display, Colormap colormap, const char* value, unsigned long fallback) {
    XColor exact{};
    XColor screen{};
    return XAllocNamedColor(display, colormap, value, &screen, &exact) != 0 ? screen.pixel : fallback;
}

void append_icon(std::vector<unsigned long>& data, int size, const std::uint32_t* pixels) {
    data.push_back(static_cast<unsigned long>(size));
    data.push_back(static_cast<unsigned long>(size));
    for (int index = 0; index < size * size; ++index) data.push_back(static_cast<unsigned long>(pixels[index]));
}

class Installer {
public:
    Installer(fs::path root, fs::path staging, bool automated)
        : root_(std::move(root)), payload_(root_ / "payload"), staging_(std::move(staging)),
          automated_(automated), plugins_(load_plugins(payload_)) {}

    ~Installer() {
        if (pipe_fd_ >= 0) close(pipe_fd_);
        if (display_ && gc_) XFreeGC(display_, gc_);
        if (display_) XCloseDisplay(display_);
    }

    bool validate_payload(std::string& error) const {
        const fs::path required[] = {
            payload_ / "Nougat_Media_Suite_v50",
            payload_ / "installer" / "nougat_v50_installer.py",
            payload_ / "assets" / "icons" / "nougat-media-suite-concept-sheet-v24.png",
            payload_ / "NougatMediaSuite.desktop",
        };
        for (const auto& path : required) {
            if (!fs::is_regular_file(path)) {
                error = "Required installer payload is missing: " + path.string();
                return false;
            }
        }
        return true;
    }

    int window_self_test() {
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            std::cerr << "FAIL: no X11/XWayland display for installer icon self-test\n";
            return 1;
        }
        screen_ = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 10, 10, 200, 100, 0,
                                      BlackPixel(display_, screen_), WhitePixel(display_, screen_));
        set_identity();
        XSync(display_, False);
        Atom actual = None;
        int format = 0;
        unsigned long count = 0;
        unsigned long after = 0;
        unsigned char* bytes = nullptr;
        const Atom atom = XInternAtom(display_, "_NET_WM_ICON", False);
        const int rc = XGetWindowProperty(display_, window_, atom, 0, 8, False, AnyPropertyType,
                                          &actual, &format, &count, &after, &bytes);
        const bool ok = rc == Success && bytes != nullptr && format == 32 && count >= 2;
        if (bytes) XFree(bytes);
        if (!ok) {
            std::cerr << "FAIL: installer window is missing embedded approved Nougat icon data\n";
            return 1;
        }
        std::cout << "PASS: graphical installer window publishes approved Nougat icon data\n";
        return 0;
    }

    int run() {
        std::string error;
        if (!validate_payload(error)) {
            std::cerr << "INSTALLER FAIL: " << error << '\n';
            return 1;
        }
        if (!open_window()) return 1;
        if (automated_) start_install();
        return event_loop();
    }

private:
    fs::path root_;
    fs::path payload_;
    fs::path staging_;
    bool automated_ = false;
    std::vector<Plugin> plugins_;
    InstallMode mode_ = InstallMode::Default;
    Page page_ = Page::Selection;
    int scroll_ = 0;
    std::string status_ = "Choose what you want installed, then click Install.";
    std::string output_;
    pid_t child_ = -1;
    int pipe_fd_ = -1;
    int progress_ = 0;

    Display* display_ = nullptr;
    int screen_ = 0;
    Colormap colormap_ = 0;
    Window window_ = 0;
    GC gc_ = 0;
    Atom wm_delete_ = None;
    unsigned long white_ = 0;
    unsigned long cream_ = 0;
    unsigned long caramel_ = 0;
    unsigned long chocolate_ = 0;
    unsigned long brown_ = 0;
    unsigned long dark_ = 0;
    unsigned long muted_ = 0;
    unsigned long green_ = 0;
    unsigned long red_ = 0;

    const Rect default_mode_{26, 92, 230, 42};
    const Rect custom_mode_{270, 92, 230, 42};
    const Rect advanced_mode_{514, 92, 280, 42};
    const Rect install_button_{625, 584, 169, 42};
    const Rect launch_button_{430, 530, 270, 44};
    const Rect close_button_{430, 584, 270, 44};
    const Rect back_button_{430, 530, 170, 44};

    bool open_window() {
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            std::cerr << "INSTALLER FAIL: no graphical X11/XWayland display is available\n";
            return false;
        }
        screen_ = DefaultScreen(display_);
        colormap_ = DefaultColormap(display_, screen_);
        white_ = color(display_, colormap_, "#fff3dc", WhitePixel(display_, screen_));
        cream_ = color(display_, colormap_, "#f1d7ad", WhitePixel(display_, screen_));
        caramel_ = color(display_, colormap_, "#c98b45", WhitePixel(display_, screen_));
        chocolate_ = color(display_, colormap_, "#4b2b1c", BlackPixel(display_, screen_));
        brown_ = color(display_, colormap_, "#704126", BlackPixel(display_, screen_));
        dark_ = color(display_, colormap_, "#25160f", BlackPixel(display_, screen_));
        muted_ = color(display_, colormap_, "#745f4e", BlackPixel(display_, screen_));
        green_ = color(display_, colormap_, "#54713d", BlackPixel(display_, screen_));
        red_ = color(display_, colormap_, "#9b3f31", BlackPixel(display_, screen_));

        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 100, 80, kWidth, kHeight,
                                      1, chocolate_, cream_);
        if (!window_) return false;
        set_identity();
        wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &wm_delete_, 1);
        XSelectInput(display_, window_, ExposureMask | ButtonPressMask | KeyPressMask);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XMapWindow(display_, window_);
        XFlush(display_);
        return true;
    }

    void set_identity() {
        XStoreName(display_, window_, kInstallerVersion);
        XClassHint hint{};
        hint.res_name = const_cast<char*>("NougatInstaller");
        hint.res_class = const_cast<char*>("NougatMediaSuite");
        XSetClassHint(display_, window_, &hint);
        std::vector<unsigned long> data;
        data.reserve(2 + 16 * 16 + 2 + 32 * 32 + 2 + 64 * 64);
        append_icon(data, nougat_media_suite_icon::kIcon16Size, nougat_media_suite_icon::kIcon16);
        append_icon(data, nougat_media_suite_icon::kIcon32Size, nougat_media_suite_icon::kIcon32);
        append_icon(data, nougat_media_suite_icon::kIcon64Size, nougat_media_suite_icon::kIcon64);
        const Atom icon_atom = XInternAtom(display_, "_NET_WM_ICON", False);
        const Atom cardinal = XInternAtom(display_, "CARDINAL", False);
        XChangeProperty(display_, window_, icon_atom, cardinal, 32, PropModeReplace,
                        reinterpret_cast<unsigned char*>(data.data()), static_cast<int>(data.size()));
    }

    void fill(const Rect& rect, unsigned long value) {
        XSetForeground(display_, gc_, value);
        XFillRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w)),
                       static_cast<unsigned int>(std::max(1, rect.h)));
    }

    void outline(const Rect& rect, unsigned long value) {
        XSetForeground(display_, gc_, value);
        XDrawRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w - 1)),
                       static_cast<unsigned int>(std::max(1, rect.h - 1)));
    }

    void text(int x, int y, const std::string& value, unsigned long shade) {
        XSetForeground(display_, gc_, shade);
        XDrawString(display_, window_, gc_, x, y, value.c_str(), static_cast<int>(value.size()));
    }

    static std::string clip(std::string value, std::size_t maximum) {
        if (value.size() <= maximum) return value;
        if (maximum <= 3) return value.substr(0, maximum);
        return value.substr(0, maximum - 3) + "...";
    }

    void checkbox(const Rect& box, bool checked, bool enabled) {
        fill(box, enabled ? white_ : cream_);
        outline(box, brown_);
        if (!checked) return;
        XSetForeground(display_, gc_, enabled ? chocolate_ : muted_);
        XDrawLine(display_, window_, gc_, box.x + 5, box.y + 12, box.x + 10, box.y + 17);
        XDrawLine(display_, window_, gc_, box.x + 10, box.y + 17, box.x + 19, box.y + 6);
        XDrawLine(display_, window_, gc_, box.x + 5, box.y + 13, box.x + 10, box.y + 18);
        XDrawLine(display_, window_, gc_, box.x + 10, box.y + 18, box.x + 19, box.y + 7);
    }

    void button(const Rect& rect, const std::string& label, bool active = true) {
        fill(rect, active ? caramel_ : cream_);
        outline(rect, chocolate_);
        const int estimate = static_cast<int>(label.size()) * 6;
        text(rect.x + std::max(10, (rect.w - estimate) / 2), rect.y + 26, label, active ? dark_ : muted_);
    }

    void mode_button(const Rect& rect, const std::string& label, bool active) {
        fill(rect, active ? caramel_ : white_);
        outline(rect, chocolate_);
        text(rect.x + 14, rect.y + 25, label, dark_);
    }

    void header() {
        fill({0, 0, kWidth, 70}, chocolate_);
        text(26, 30, "NOUGAT MEDIA SUITE", white_);
        text(26, 52, "GRAPHICAL INSTALLER  v0.0.50", cream_);
    }

    int visible_rows() const { return 6; }

    std::vector<std::pair<Rect, int>> rows() {
        scroll_ = std::max(0, std::min(scroll_, std::max(0, static_cast<int>(plugins_.size()) - visible_rows())));
        std::vector<std::pair<Rect, int>> result;
        int y = 222;
        for (int visible = 0; visible < visible_rows(); ++visible) {
            const int index = scroll_ + visible;
            if (index >= static_cast<int>(plugins_.size())) break;
            result.push_back({{26, y, 768, kPluginRowHeight - 6}, index});
            y += kPluginRowHeight;
        }
        return result;
    }

    void draw_selection() {
        header();
        text(26, 84, "Installation type", chocolate_);
        mode_button(default_mode_, "Default", mode_ == InstallMode::Default);
        mode_button(custom_mode_, "Custom", mode_ == InstallMode::Custom);
        mode_button(advanced_mode_, "Advanced Custom", mode_ == InstallMode::Advanced);

        const Rect core{26, 150, 768, 52};
        fill(core, white_);
        outline(core, brown_);
        checkbox({40, 164, 24, 24}, true, false);
        text(78, 174, "Video Player + Nougat Plugin Core   REQUIRED", chocolate_);
        text(78, 192, "The player and plugin system are always installed.", muted_);

        text(26, 216, "Optional plugin packages", chocolate_);
        const bool editable = mode_ != InstallMode::Default;
        const auto visible = rows();
        if (visible.empty()) text(40, 258, "No valid optional plugin packages are available.", muted_);
        for (const auto& item : visible) {
            const Plugin& plugin = plugins_[static_cast<std::size_t>(item.second)];
            fill(item.first, white_);
            outline(item.first, brown_);
            checkbox({item.first.x + 14, item.first.y + 14, 24, 24}, plugin.selected, editable);
            text(item.first.x + 52, item.first.y + 21, plugin.name + "   [" + plugin.id + "]", chocolate_);
            text(item.first.x + 52, item.first.y + 41, clip(plugin.description, 105), muted_);
        }

        const Rect status_box{26, 574, 570, 52};
        fill(status_box, white_);
        outline(status_box, brown_);
        text(40, 596, clip(status_, 88), muted_);
        text(40, 616, "Nothing changes until you click Install.", muted_);
        button(install_button_, "Install");
    }

    void draw_installing() {
        header();
        text(40, 120, "Installing Nougat Media Suite...", chocolate_);
        text(40, 150, "Video Player + Plugin Core", dark_);
        const std::string selected = selected_ids(plugins_);
        text(40, 174, "Selected plugins: " + (selected.empty() ? std::string("none") : selected), muted_);
        const Rect track{40, 220, 740, 28};
        fill(track, white_);
        outline(track, brown_);
        const int block = 130;
        const int travel = std::max(1, track.w - block - 4);
        const int phase = progress_ % (travel * 2);
        const int position = phase <= travel ? phase : travel * 2 - phase;
        fill({track.x + 2 + position, track.y + 3, block, track.h - 6}, caramel_);
        text(40, 290, clip(status_, 116), dark_);
        text(40, 322, "Verifying the player, plugin files, launcher, and approved Nougat icon.", muted_);
        text(40, 350, "Do not close this window while installation is running.", red_);
    }

    void draw_complete() {
        header();
        text(40, 130, "Installation complete", green_);
        text(40, 168, "Nougat Media Suite v0.0.50 is installed.", chocolate_);
        const std::string selected = selected_ids(plugins_);
        text(40, 196, "Installed plugins: " + (selected.empty() ? std::string("none (player only)") : selected), muted_);
        text(40, 246, "What would you like to do?", dark_);
        button(launch_button_, "Launch Nougat Media Suite");
        button(close_button_, "Close Installer");
    }

    void draw_failed() {
        header();
        text(40, 130, "Installation failed", red_);
        text(40, 168, clip(status_, 116), dark_);
        text(40, 200, "The installer did not mark this candidate accepted or final.", muted_);
        button(back_button_, "Back");
        button(close_button_, "Close Installer");
    }

    void redraw() {
        fill({0, 0, kWidth, kHeight}, cream_);
        if (page_ == Page::Selection) draw_selection();
        else if (page_ == Page::Installing) draw_installing();
        else if (page_ == Page::Complete) draw_complete();
        else draw_failed();
        XFlush(display_);
    }

    void set_mode(InstallMode mode) {
        mode_ = mode;
        if (mode_ == InstallMode::Default) {
            for (auto& plugin : plugins_) plugin.selected = plugin.recommended;
            status_ = "Default selects the recommended plugin set.";
        } else {
            status_ = "Check exactly which optional plugins you want installed.";
        }
        redraw();
    }

    void selection_click(int x, int y) {
        if (default_mode_.contains(x, y)) { set_mode(InstallMode::Default); return; }
        if (custom_mode_.contains(x, y)) { set_mode(InstallMode::Custom); return; }
        if (advanced_mode_.contains(x, y)) { set_mode(InstallMode::Advanced); return; }
        if (install_button_.contains(x, y)) { start_install(); return; }
        if (mode_ == InstallMode::Default) return;
        for (const auto& item : rows()) {
            if (!item.first.contains(x, y)) continue;
            Plugin& plugin = plugins_[static_cast<std::size_t>(item.second)];
            plugin.selected = !plugin.selected;
            const std::string selected = selected_ids(plugins_);
            status_ = "Selected plugins: " + (selected.empty() ? std::string("none") : selected);
            redraw();
            return;
        }
    }

    void click(unsigned int mouse_button, int x, int y) {
        if (page_ == Page::Selection) {
            if (mouse_button == Button4) { --scroll_; redraw(); return; }
            if (mouse_button == Button5) { ++scroll_; redraw(); return; }
            if (mouse_button == Button1) selection_click(x, y);
            return;
        }
        if (mouse_button != Button1) return;
        if (page_ == Page::Complete) {
            if (launch_button_.contains(x, y)) { launch(); std::exit(0); }
            if (close_button_.contains(x, y)) std::exit(0);
        } else if (page_ == Page::Failed) {
            if (back_button_.contains(x, y)) {
                page_ = Page::Selection;
                status_ = "Choose what you want installed, then click Install.";
                redraw();
            } else if (close_button_.contains(x, y)) {
                std::exit(1);
            }
        }
    }

    void start_install() {
        if (child_ > 0 || page_ == Page::Installing) return;
        int fds[2] = {-1, -1};
        if (pipe(fds) != 0) {
            page_ = Page::Failed;
            status_ = "Unable to create the installer process pipe.";
            redraw();
            return;
        }
        child_ = fork();
        if (child_ < 0) {
            close(fds[0]);
            close(fds[1]);
            page_ = Page::Failed;
            status_ = "Unable to start the installer backend.";
            redraw();
            return;
        }
        if (child_ == 0) {
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO);
            dup2(fds[1], STDERR_FILENO);
            close(fds[1]);
            std::vector<std::string> args = {
                "python3",
                (payload_ / "installer" / "nougat_v50_installer.py").string(),
                "--source-root", payload_.string(),
                "--mode", mode_name(mode_),
                "--plugins", selected_ids(plugins_),
            };
            if (!staging_.empty()) {
                args.push_back("--staging-root");
                args.push_back(staging_.string());
            }
            std::vector<char*> argv;
            argv.reserve(args.size() + 1);
            for (auto& arg : args) argv.push_back(arg.data());
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            _exit(127);
        }
        close(fds[1]);
        pipe_fd_ = fds[0];
        const int flags = fcntl(pipe_fd_, F_GETFL, 0);
        if (flags >= 0) fcntl(pipe_fd_, F_SETFL, flags | O_NONBLOCK);
        page_ = Page::Installing;
        status_ = "Starting installation...";
        output_.clear();
        progress_ = 0;
        redraw();
    }

    void read_child() {
        if (pipe_fd_ < 0) return;
        char buffer[2048];
        while (true) {
            const ssize_t count = read(pipe_fd_, buffer, sizeof(buffer));
            if (count > 0) {
                output_.append(buffer, static_cast<std::size_t>(count));
                const std::size_t end = output_.rfind('\n');
                if (end != std::string::npos) {
                    const std::size_t previous = end == 0 ? std::string::npos : output_.rfind('\n', end - 1);
                    const std::size_t begin = previous == std::string::npos ? 0 : previous + 1;
                    const std::string line = output_.substr(begin, end - begin);
                    if (!line.empty()) status_ = line;
                }
                continue;
            }
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            return;
        }
    }

    bool poll_child() {
        if (child_ <= 0) return false;
        read_child();
        int child_status = 0;
        const pid_t result = waitpid(child_, &child_status, WNOHANG);
        if (result == 0) return false;
        if (result > 0 && WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) {
            page_ = Page::Complete;
            status_ = "Installation complete.";
        } else {
            page_ = Page::Failed;
            if (status_.empty()) status_ = "The installer backend reported a failure.";
        }
        child_ = -1;
        read_child();
        if (pipe_fd_ >= 0) {
            close(pipe_fd_);
            pipe_fd_ = -1;
        }
        redraw();
        return true;
    }

    void launch() const {
        if (automated_ || !staging_.empty()) return;
        const pid_t pid = fork();
        if (pid != 0) return;
        setsid();
        execl(kInstalledCore, kInstalledCore, static_cast<char*>(nullptr));
        _exit(127);
    }

    int event_loop() {
        redraw();
        while (true) {
            fd_set inputs;
            FD_ZERO(&inputs);
            const int xfd = ConnectionNumber(display_);
            FD_SET(xfd, &inputs);
            int maximum = xfd;
            if (pipe_fd_ >= 0) {
                FD_SET(pipe_fd_, &inputs);
                maximum = std::max(maximum, pipe_fd_);
            }
            timeval timeout{0, 100000};
            const int result = select(maximum + 1, &inputs, nullptr, nullptr, &timeout);
            if (result < 0 && errno != EINTR) return 1;

            while (XPending(display_) > 0) {
                XEvent event{};
                XNextEvent(display_, &event);
                if (event.type == Expose) redraw();
                else if (event.type == ButtonPress) click(event.xbutton.button, event.xbutton.x, event.xbutton.y);
                else if (event.type == KeyPress) {
                    const KeySym key = XLookupKeysym(&event.xkey, 0);
                    if (key == XK_Escape && page_ != Page::Installing) return page_ == Page::Failed ? 1 : 0;
                } else if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_) {
                    if (page_ == Page::Installing) {
                        status_ = "Installation is still running. Wait for it to finish before closing.";
                        redraw();
                    } else {
                        return page_ == Page::Failed ? 1 : 0;
                    }
                }
            }

            if (page_ == Page::Installing) {
                ++progress_;
                if (!poll_child()) redraw();
            }
            if (automated_ && page_ == Page::Complete) {
                std::cout << "PASS: graphical installer drove the packaged backend to completion\n";
                return 0;
            }
            if (automated_ && page_ == Page::Failed) {
                std::cerr << "FAIL: graphical installer automation failed: " << status_ << '\n';
                return 1;
            }
        }
    }
};

int self_test(const fs::path& package_root) {
    Installer installer(package_root, {}, false);
    std::string error;
    if (!installer.validate_payload(error)) {
        std::cerr << "FAIL: " << error << '\n';
        return 1;
    }
    const auto plugins = load_plugins(package_root / "payload");
    std::cout << "PASS: graphical installer package payload is complete\n";
    std::cout << "PASS: discovered " << plugins.size() << " valid optional plugin package(s)\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--version") {
        std::cout << kInstallerVersion << '\n';
        return 0;
    }

    fs::path package_root = executable_path().parent_path();
    fs::path staging_root;
    bool automated = false;
    bool package_test = false;
    bool window_test = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--package-root" && index + 1 < argc) package_root = fs::path(argv[++index]);
        else if (argument == "--staging-root" && index + 1 < argc) staging_root = fs::path(argv[++index]);
        else if (argument == "--automated-install") automated = true;
        else if (argument == "--self-test") package_test = true;
        else if (argument == "--window-self-test") window_test = true;
    }

    if (package_test) return self_test(package_root);
    Installer installer(package_root, staging_root, automated);
    if (window_test) return installer.window_self_test();
    return installer.run();
}
