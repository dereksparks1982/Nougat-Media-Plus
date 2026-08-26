#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "nougat_media_suite_icon_data.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr int kWidth = 820;
constexpr int kHeight = 650;
constexpr int kPluginRowHeight = 58;
constexpr const char* kInstallerVersion = "Nougat Media Suite Installer v0.0.50";
constexpr const char* kCoreVersion = "Nougat Media Suite v0.0.50";
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
        const std::string text = read_text(manifest);
        if (json_string(text, "format") != "NOUGAT_PLUGIN") continue;
        const std::string id = json_string(text, "id");
        if (id.empty() || id == "." || id == ".." || id.find('/') != std::string::npos || id.find('\\') != std::string::npos) continue;
        Plugin plugin;
        plugin.id = id;
        plugin.name = json_string(text, "display_name");
        if (plugin.name.empty()) plugin.name = id;
        plugin.description = json_string(text, "description");
        plugin.recommended = json_bool(text, "recommended_by_default", false);
        plugin.selected = plugin.recommended;
        plugins.push_back(std::move(plugin));
    }
    std::sort(plugins.begin(), plugins.end(), [](const Plugin& a, const Plugin& b) {
        return a.name < b.name;
    });
    return plugins;
}

std::string join_selected(const std::vector<Plugin>& plugins) {
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
    switch (mode) {
        case InstallMode::Default: return "default";
        case InstallMode::Custom: return "custom";
        case InstallMode::Advanced: return "advanced";
    }
    return "custom";
}

unsigned long named_color(Display* display, Colormap colormap, const char* value, unsigned long fallback) {
    XColor exact{};
    XColor screen{};
    if (XAllocNamedColor(display, colormap, value, &screen, &exact) != 0) return screen.pixel;
    return fallback;
}

void append_icon(std::vector<unsigned long>& data, int size, const std::uint32_t* pixels) {
    data.push_back(static_cast<unsigned long>(size));
    data.push_back(static_cast<unsigned long>(size));
    for (int index = 0; index < size * size; ++index) {
        data.push_back(static_cast<unsigned long>(pixels[index]));
    }
}

class InstallerApp {
public:
    InstallerApp(fs::path package_root, fs::path staging_root, bool automated)
        : package_root_(std::move(package_root)),
          payload_root_(package_root_ / "payload"),
          staging_root_(std::move(staging_root)),
          automated_(automated),
          plugins_(load_plugins(payload_root_)) {}

    ~InstallerApp() {
        if (pipe_fd_ >= 0) close(pipe_fd_);
        if (display_) XCloseDisplay(display_);
    }

    bool package_valid(std::string& error) const {
        const fs::path required[] = {
            payload_root_ / "Nougat_Media_Suite_v50",
            payload_root_ / "installer" / "nougat_v50_installer.py",
            payload_root_ / "assets" / "icons" / "nougat-media-suite-concept-sheet-v24.png",
            payload_root_ / "NougatMediaSuite.desktop",
        };
        for (const auto& path : required) {
            if (!fs::is_regular_file(path)) {
                error = "Required installer payload is missing: " + path.string();
                return false;
            }
        }
        return true;
    }

    int run() {
        std::string error;
        if (!package_valid(error)) {
            std::cerr << "INSTALLER FAIL: " << error << '\n';
            return 1;
        }

        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            std::cerr << "INSTALLER FAIL: no graphical X11/XWayland display is available\n";
            return 1;
        }

        screen_ = DefaultScreen(display_);
        colormap_ = DefaultColormap(display_, screen_);
        white_ = named_color(display_, colormap_, "#fff3dc", WhitePixel(display_, screen_));
        cream_ = named_color(display_, colormap_, "#f1d7ad", WhitePixel(display_, screen_));
        caramel_ = named_color(display_, colormap_, "#c98b45", WhitePixel(display_, screen_));
        chocolate_ = named_color(display_, colormap_, "#4b2b1c", BlackPixel(display_, screen_));
        brown_ = named_color(display_, colormap_, "#704126", BlackPixel(display_, screen_));
        dark_ = named_color(display_, colormap_, "#25160f", BlackPixel(display_, screen_));
        muted_ = named_color(display_, colormap_, "#745f4e", BlackPixel(display_, screen_));
        green_ = named_color(display_, colormap_, "#54713d", BlackPixel(display_, screen_));
        red_ = named_color(display_, colormap_, "#9b3f31", BlackPixel(display_, screen_));

        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 100, 80,
                                      kWidth, kHeight, 1, chocolate_, cream_);
        if (!window_) {
            std::cerr << "INSTALLER FAIL: unable to create installer window\n";
            return 1;
        }
        XStoreName(display_, window_, kInstallerVersion);
        XClassHint class_hint{};
        class_hint.res_name = const_cast<char*>("NougatInstaller");
        class_hint.res_class = const_cast<char*>("NougatMediaSuite");
        XSetClassHint(display_, window_, &class_hint);
        set_window_icon();

        wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &wm_delete_, 1);
        XSelectInput(display_, window_, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XMapWindow(display_, window_);
        XFlush(display_);

        if (automated_) {
            start_install();
        }

        return event_loop();
    }

    int window_self_test() {
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            std::cerr << "FAIL: no display for installer window self-test\n";
            return 1;
        }
        screen_ = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 10, 10, 200, 120, 0,
                                      BlackPixel(display_, screen_), WhitePixel(display_, screen_));
        XClassHint class_hint{};
        class_hint.res_name = const_cast<char*>("NougatInstaller");
        class_hint.res_class = const_cast<char*>("NougatMediaSuite");
        XSetClassHint(display_, window_, &class_hint);
        set_window_icon();
        XSync(display_, False);

        Atom actual = None;
        int format = 0;
        unsigned long items = 0;
        unsigned long after = 0;
        unsigned char* bytes = nullptr;
        const Atom icon_atom = XInternAtom(display_, "_NET_WM_ICON", False);
        const int rc = XGetWindowProperty(display_, window_, icon_atom, 0, 8, False, AnyPropertyType,
                                          &actual, &format, &items, &after, &bytes);
        const bool ok = rc == Success && bytes != nullptr && format == 32 && items >= 2;
        if (bytes) XFree(bytes);
        if (!ok) {
            std::cerr << "FAIL: graphical installer did not publish the approved embedded X11 icon data\n";
            return 1;
        }
        std::cout << "PASS: graphical installer window identity and embedded Nougat icon\n";
        return 0;
    }

private:
    fs::path package_root_;
    fs::path payload_root_;
    fs::path staging_root_;
    bool automated_ = false;
    std::vector<Plugin> plugins_;
    InstallMode mode_ = InstallMode::Default;
    Page page_ = Page::Selection;
    int plugin_scroll_ = 0;
    std::string status_ = "Choose the optional plugins you want, then click Install.";
    std::string child_output_;
    pid_t child_pid_ = -1;
    int pipe_fd_ = -1;
    int progress_tick_ = 0;

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

    Rect default_mode_{26, 92, 230, 42};
    Rect custom_mode_{270, 92, 230, 42};
    Rect advanced_mode_{514, 92, 280, 42};
    Rect install_button_{625, 584, 169, 42};
    Rect launch_button_{430, 530, 270, 44};
    Rect close_button_{430, 584, 270, 44};
    Rect retry_button_{430, 530, 170, 44};

    void set_window_icon() {
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

    void fill(Rect rect, unsigned long color) {
        XSetForeground(display_, gc_, color);
        XFillRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w)),
                       static_cast<unsigned int>(std::max(1, rect.h)));
    }

    void outline(Rect rect, unsigned long color) {
        XSetForeground(display_, gc_, color);
        XDrawRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w - 1)),
                       static_cast<unsigned int>(std::max(1, rect.h - 1)));
    }

    void text(int x, int y, const std::string& value, unsigned long color = 0) {
        XSetForeground(display_, gc_, color == 0 ? dark_ : color);
        XDrawString(display_, window_, gc_, x, y, value.c_str(), static_cast<int>(value.size()));
    }

    static std::string clipped(std::string value, std::size_t max_chars) {
        if (value.size() <= max_chars) return value;
        if (max_chars <= 3) return value.substr(0, max_chars);
        return value.substr(0, max_chars - 3) + "...";
    }

    void draw_checkbox(const Rect& box, bool checked, bool enabled) {
        fill(box, enabled ? white_ : cream_);
        outline(box, brown_);
        if (!checked) return;
        XSetForeground(display_, gc_, enabled ? chocolate_ : muted_);
        XDrawLine(display_, window_, gc_, box.x + 5, box.y + 12, box.x + 10, box.y + 17);
        XDrawLine(display_, window_, gc_, box.x + 10, box.y + 17, box.x + 19, box.y + 6);
        XDrawLine(display_, window_, gc_, box.x + 5, box.y + 13, box.x + 10, box.y + 18);
        XDrawLine(display_, window_, gc_, box.x + 10, box.y + 18, box.x + 19, box.y + 7);
    }

    void draw_radio(const Rect& rect, const std::string& label, bool active) {
        fill(rect, active ? caramel_ : white_);
        outline(rect, chocolate_);
        text(rect.x + 14, rect.y + 25, label, dark_);
    }

    void draw_button(const Rect& rect, const std::string& label, bool enabled = true) {
        fill(rect, enabled ? caramel_ : cream_);
        outline(rect, chocolate_);
        const int approximate = static_cast<int>(label.size()) * 6;
        text(rect.x + std::max(10, (rect.w - approximate) / 2), rect.y + 26, label,
             enabled ? dark_ : muted_);
    }

    void draw_header() {
        fill({0, 0, kWidth, 70}, chocolate_);
        text(26, 30, "NOUGAT MEDIA SUITE", white_);
        text(26, 52, "GRAPHICAL INSTALLER  v0.0.50", cream_);
    }

    int visible_plugin_rows() const {
        return 6;
    }

    void clamp_scroll() {
        plugin_scroll_ = std::max(0, std::min(plugin_scroll_,
            std::max(0, static_cast<int>(plugins_.size()) - visible_plugin_rows())));
    }

    std::vector<std::pair<Rect, int>> plugin_rows() {
        clamp_scroll();
        std::vector<std::pair<Rect, int>> rows;
        int y = 216;
        for (int visible = 0; visible < visible_plugin_rows(); ++visible) {
            const int index = plugin_scroll_ + visible;
            if (index >= static_cast<int>(plugins_.size())) break;
            rows.push_back({{26, y, 768, kPluginRowHeight - 6}, index});
            y += kPluginRowHeight;
        }
        return rows;
    }

    void draw_selection() {
        draw_header();
        text(26, 84, "Installation type", chocolate_);
        draw_radio(default_mode_, "Default", mode_ == InstallMode::Default);
        draw_radio(custom_mode_, "Custom", mode_ == InstallMode::Custom);
        draw_radio(advanced_mode_, "Advanced Custom", mode_ == InstallMode::Advanced);

        Rect core_row{26, 150, 768, 52};
        fill(core_row, white_);
        outline(core_row, brown_);
        draw_checkbox({40, 164, 24, 24}, true, false);
        text(78, 174, "Video Player + Nougat Plugin Core   REQUIRED", chocolate_);
        text(78, 192, "The player and plugin loader are always installed.", muted_);

        text(26, 212, "Optional plugin packages found beside this installer", chocolate_);
        const bool can_toggle = mode_ != InstallMode::Default;
        const auto rows = plugin_rows();
        if (rows.empty()) {
            text(40, 252, "No valid optional plugin packages are currently available.", muted_);
        }
        for (const auto& row : rows) {
            const Plugin& plugin = plugins_[static_cast<std::size_t>(row.second)];
            fill(row.first, white_);
            outline(row.first, brown_);
            const Rect checkbox{row.first.x + 14, row.first.y + 14, 24, 24};
            draw_checkbox(checkbox, plugin.selected, can_toggle);
            text(row.first.x + 52, row.first.y + 21,
                 plugin.name + "   [" + plugin.id + "]", chocolate_);
            text(row.first.x + 52, row.first.y + 41,
                 clipped(plugin.description, 105), muted_);
        }

        if (static_cast<int>(plugins_.size()) > visible_plugin_rows()) {
            text(748, 570, "Wheel scroll", muted_);
        }
        fill({26, 574, 570, 52}, white_);
        outline({26, 574, 570, 52}, brown_);
        text(40, 596, clipped(status_, 88), muted_);
        text(40, 616, "Nothing installs until you click Install.", muted_);
        draw_button(install_button_, "Install");
    }

    void draw_installing() {
        draw_header();
        text(40, 120, "Installing Nougat Media Suite...", chocolate_);
        text(40, 150, "Video Player + Plugin Core", dark_);
        text(40, 174, "Selected plugins: " + (join_selected(plugins_).empty() ? std::string("none") : join_selected(plugins_)), muted_);
        Rect track{40, 220, 740, 28};
        fill(track, white_);
        outline(track, brown_);
        const int block_w = 130;
        const int travel = std::max(1, track.w - block_w - 4);
        const int x = track.x + 2 + (progress_tick_ % (travel * 2));
        const int reflected = x > track.x + 2 + travel ? (track.x + 2 + travel * 2 - x) : x;
        fill({reflected, track.y + 3, block_w, track.h - 6}, caramel_);
        text(40, 290, clipped(status_, 116), dark_);
        text(40, 322, "The installer is verifying the player, plugins, launcher, and approved Nougat icon.", muted_);
        text(40, 350, "Do not close this window while installation is in progress.", red_);
    }

    void draw_complete() {
        draw_header();
        text(40, 130, "Installation complete", green_);
        text(40, 168, "Nougat Media Suite v0.0.50 is installed.", chocolate_);
        text(40, 196, "Installed plugins: " + (join_selected(plugins_).empty() ? std::string("none (player only)") : join_selected(plugins_)), muted_);
        text(40, 246, "What would you like to do?", dark_);
        draw_button(launch_button_, "Launch Nougat Media Suite");
        draw_button(close_button_, "Close Installer");
    }

    void draw_failed() {
        draw_header();
        text(40, 130, "Installation failed", red_);
        text(40, 168, clipped(status_, 116), dark_);
        text(40, 200, "No acceptance or release state was changed.", muted_);
        draw_button(retry_button_, "Back");
        draw_button(close_button_, "Close Installer");
    }

    void redraw() {
        fill({0, 0, kWidth, kHeight}, cream_);
        switch (page_) {
            case Page::Selection: draw_selection(); break;
            case Page::Installing: draw_installing(); break;
            case Page::Complete: draw_complete(); break;
            case Page::Failed: draw_failed(); break;
        }
        XFlush(display_);
    }

    void apply_default_selection() {
        for (auto& plugin : plugins_) plugin.selected = plugin.recommended;
    }

    void set_mode(InstallMode mode) {
        mode_ = mode;
        if (mode_ == InstallMode::Default) apply_default_selection();
        status_ = mode_ == InstallMode::Default
            ? "Default selects the recommended plugin set."
            : "Choose exactly which optional plugins you want installed.";
        redraw();
    }

    void handle_selection_click(int x, int y) {
        if (default_mode_.contains(x, y)) { set_mode(InstallMode::Default); return; }
        if (custom_mode_.contains(x, y)) { set_mode(InstallMode::Custom); return; }
        if (advanced_mode_.contains(x, y)) { set_mode(InstallMode::Advanced); return; }
        if (install_button_.contains(x, y)) { start_install(); return; }
        if (mode_ == InstallMode::Default) return;
        for (const auto& row : plugin_rows()) {
            if (!row.first.contains(x, y)) continue;
            plugins_[static_cast<std::size_t>(row.second)].selected =
                !plugins_[static_cast<std::size_t>(row.second)].selected;
            status_ = "Selected plugins: " + (join_selected(plugins_).empty() ? std::string("none") : join_selected(plugins_));
            redraw();
            return;
        }
    }

    void handle_click(int button, int x, int y) {
        if (page_ == Page::Selection) {
            if (button == Button4) { --plugin_scroll_; redraw(); return; }
            if (button == Button5) { ++plugin_scroll_; redraw(); return; }
            if (button == Button1) handle_selection_click(x, y);
            return;
        }
        if (button != Button1) return;
        if (page_ == Page::Complete) {
            if (launch_button_.contains(x, y)) {
                launch_application();
                std::exit(0);
            }
            if (close_button_.contains(x, y)) std::exit(0);
            return;
        }
        if (page_ == Page::Failed) {
            if (retry_button_.contains(x, y)) {
                page_ = Page::Selection;
                status_ = "Choose the optional plugins you want, then click Install.";
                redraw();
            } else if (close_button_.contains(x, y)) {
                std::exit(1);
            }
        }
    }

    void start_install() {
        if (child_pid_ > 0 || page_ == Page::Installing) return;
        int descriptors[2] = {-1, -1};
        if (pipe(descriptors) != 0) {
            page_ = Page::Failed;
            status_ = "Unable to create installer process pipe.";
            redraw();
            return;
        }

        const std::string selected = join_selected(plugins_);
        const std::string mode = mode_name(mode_);
        child_pid_ = fork();
        if (child_pid_ < 0) {
            close(descriptors[0]);
            close(descriptors[1]);
            page_ = Page::Failed;
            status_ = "Unable to start installer backend.";
            redraw();
            return;
        }
        if (child_pid_ == 0) {
            close(descriptors[0]);
            dup2(descriptors[1], STDOUT_FILENO);
            dup2(descriptors[1], STDERR_FILENO);
            close(descriptors[1]);

            std::vector<std::string> arguments = {
                "python3",
                (payload_root_ / "installer" / "nougat_v50_installer.py").string(),
                "--source-root", payload_root_.string(),
                "--mode", mode,
                "--plugins", selected,
            };
            if (!staging_root_.empty()) {
                arguments.push_back("--staging-root");
                arguments.push_back(staging_root_.string());
            }
            std::vector<char*> argv;
            argv.reserve(arguments.size() + 1);
            for (auto& argument : arguments) argv.push_back(argument.data());
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            _exit(127);
        }

        close(descriptors[1]);
        pipe_fd_ = descriptors[0];
        const int flags = fcntl(pipe_fd_, F_GETFL, 0);
        if (flags >= 0) fcntl(pipe_fd_, F_SETFL, flags | O_NONBLOCK);
        page_ = Page::Installing;
        status_ = "Starting installation...";
        child_output_.clear();
        progress_tick_ = 0;
        redraw();
    }

    void consume_child_output() {
        if (pipe_fd_ < 0) return;
        char buffer[2048];
        while (true) {
            const ssize_t count = read(pipe_fd_, buffer, sizeof(buffer));
            if (count > 0) {
                child_output_.append(buffer, static_cast<std::size_t>(count));
                std::size_t newline = child_output_.rfind('\n');
                if (newline != std::string::npos) {
                    std::size_t previous = child_output_.rfind('\n', newline > 0 ? newline - 1 : 0);
                    const std::size_t begin = previous == std::string::npos ? 0 : previous + 1;
                    const std::string line = child_output_.substr(begin, newline - begin);
                    if (!line.empty()) status_ = line;
                }
                continue;
            }
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            return;
        }
    }

    bool poll_child() {
        if (child_pid_ <= 0) return false;
        consume_child_output();
        int status = 0;
        const pid_t result = waitpid(child_pid_, &status, WNOHANG);
        if (result == 0) return false;
        if (result < 0) {
            page_ = Page::Failed;
            status_ = "Installer backend status could not be read.";
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            page_ = Page::Complete;
            status_ = "Installation complete.";
        } else {
            page_ = Page::Failed;
            if (status_.empty()) status_ = "Installer backend reported a failure.";
        }
        child_pid_ = -1;
        if (pipe_fd_ >= 0) {
            consume_child_output();
            close(pipe_fd_);
            pipe_fd_ = -1;
        }
        redraw();
        return true;
    }

    void launch_application() const {
        if (automated_ || !staging_root_.empty()) return;
        const pid_t pid = fork();
        if (pid != 0) return;
        setsid();
        execl(kInstalledCore, kInstalledCore, static_cast<char*>(nullptr));
        _exit(127);
    }

    int event_loop() {
        redraw();
        while (true) {
            fd_set read_set;
            FD_ZERO(&read_set);
            const int xfd = ConnectionNumber(display_);
            FD_SET(xfd, &read_set);
            int max_fd = xfd;
            if (pipe_fd_ >= 0) {
                FD_SET(pipe_fd_, &read_set);
                max_fd = std::max(max_fd, pipe_fd_);
            }
            timeval timeout{0, 100000};
            const int ready = select(max_fd + 1, &read_set, nullptr, nullptr, &timeout);
            if (ready < 0 && errno != EINTR) {
                std::cerr << "INSTALLER FAIL: display event loop failed\n";
                return 1;
            }

            while (XPending(display_) > 0) {
                XEvent event{};
                XNextEvent(display_, &event);
                if (event.type == Expose) {
                    redraw();
                } else if (event.type == ButtonPress) {
                    handle_click(event.xbutton.button, event.xbutton.x, event.xbutton.y);
                } else if (event.type == KeyPress) {
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
                ++progress_tick_;
                const bool finished = poll_child();
                if (!finished) redraw();
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

int package_self_test(const fs::path& package_root) {
    InstallerApp app(package_root, {}, false);
    std::string error;
    if (!app.package_valid(error)) {
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
    bool self_test = false;
    bool window_self_test = false;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--package-root" && index + 1 < argc) {
            package_root = fs::path(argv[++index]).expand_filename();
        } else if (argument == "--staging-root" && index + 1 < argc) {
            staging_root = fs::path(argv[++index]);
        } else if (argument == "--automated-install") {
            automated = true;
        } else if (argument == "--self-test") {
            self_test = true;
        } else if (argument == "--window-self-test") {
            window_self_test = true;
        }
    }

    if (self_test) return package_self_test(package_root);
    InstallerApp app(package_root, staging_root, automated);
    if (window_self_test) return app.window_self_test();
    return app.run();
}
