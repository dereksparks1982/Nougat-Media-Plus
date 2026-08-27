#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "games/emulator_host.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::atomic_bool g_running{true};

void handle_signal(int) {
    g_running.store(false);
}

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

enum class Panel { Library, Systems, Controllers, Settings };
enum class DisplayMode { Grid, List };

struct GameEntry {
    std::string title;
    std::string path;
    std::string system;
    bool bundled = false;
    bool archived = false;
    std::string archive_entry;
    std::string artwork_path;
    std::string bundled_artwork;
    std::string prepared_artwork;
    std::string entry_point;
    bool directory_game = false;
};

struct BmpImage {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgb;
};

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ends_with_lower(const std::string& value, const std::string& suffix) {
    const std::string a = lower_copy(value);
    const std::string b = lower_copy(suffix);
    return a.size() >= b.size() && a.compare(a.size() - b.size(), b.size(), b) == 0;
}

std::string basename_only(const std::string& value) {
    return fs::path(value).filename().string();
}

std::string stem_only(const std::string& value) {
    return fs::path(value).stem().string();
}

std::string display_title(const std::string& value) {
    std::string title = stem_only(value);
    for (char& c : title) if (c == '_' || c == '-') c = ' ';
    while (title.find("  ") != std::string::npos) title.replace(title.find("  "), 2, " ");
    return title.empty() ? basename_only(value) : title;
}

bool regular_file(const fs::path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec) && !ec;
}

bool directory_exists(const fs::path& path) {
    std::error_code ec;
    return fs::is_directory(path, ec) && !ec;
}

fs::path env_path(const char* name) {
    const char* value = std::getenv(name);
    return value && *value ? fs::path(value) : fs::path();
}

std::string safe_clip(std::string value, std::size_t max) {
    if (value.size() <= max) return value;
    if (max <= 3) return value.substr(0, max);
    return value.substr(0, max - 3) + "...";
}

bool safe_zip_member(std::string name) {
    if (name.empty()) return false;
    std::replace(name.begin(), name.end(), '\\', '/');
    if (name.front() == '/' || name.find('\0') != std::string::npos) return false;
    fs::path path(name);
    if (path.is_absolute()) return false;
    for (const auto& part : path) {
        if (part == "..") return false;
    }
    return true;
}

std::vector<std::string> capture_command(const std::vector<std::string>& args, int* exit_code = nullptr) {
    std::vector<std::string> lines;
    if (args.empty()) return lines;
    int pipefd[2] = {-1, -1};
    if (::pipe(pipefd) != 0) return lines;
    const pid_t child = ::fork();
    if (child < 0) {
        ::close(pipefd[0]); ::close(pipefd[1]);
        return lines;
    }
    if (child == 0) {
        (void)::dup2(pipefd[1], STDOUT_FILENO);
        const int nullfd = ::open("/dev/null", O_WRONLY);
        if (nullfd >= 0) {
            (void)::dup2(nullfd, STDERR_FILENO);
            if (nullfd > STDERR_FILENO) ::close(nullfd);
        }
        ::close(pipefd[0]); ::close(pipefd[1]);
        std::vector<char*> argv;
        argv.reserve(args.size() + 1U);
        for (const std::string& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        ::execvp(argv[0], argv.data());
        _exit(127);
    }
    ::close(pipefd[1]);
    std::string output;
    char buffer[4096]{};
    for (;;) {
        const ssize_t n = ::read(pipefd[0], buffer, sizeof(buffer));
        if (n > 0) output.append(buffer, static_cast<std::size_t>(n));
        else if (n < 0 && errno == EINTR) continue;
        else break;
    }
    ::close(pipefd[0]);
    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    if (exit_code != nullptr) *exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 126;
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

std::string executable_on_path(const std::string& name) {
    if (name.empty()) return {};
    if (name.find('/') != std::string::npos) {
        return regular_file(name) && ::access(name.c_str(), X_OK) == 0 ? name : std::string();
    }
    const char* path_env = std::getenv("PATH");
    if (!path_env) return {};
    std::istringstream stream(path_env);
    std::string part;
    while (std::getline(stream, part, ':')) {
        if (part.empty()) part = ".";
        const fs::path candidate = fs::path(part) / name;
        if (regular_file(candidate) && ::access(candidate.c_str(), X_OK) == 0) return candidate.string();
    }
    return {};
}

std::string first_executable(const std::vector<std::string>& candidates) {
    for (const std::string& candidate : candidates) {
        const std::string found = executable_on_path(candidate);
        if (!found.empty()) return found;
    }
    return {};
}

std::string system_for_path(const std::string& path, const std::string& container = {}) {
    const std::string lower = lower_copy(path);
    const std::string context = lower_copy(container + "/" + path);
    if (ends_with_lower(lower, ".nes")) return "NES";
    if (ends_with_lower(lower, ".sfc") || ends_with_lower(lower, ".smc")) return "SNES";
    if (ends_with_lower(lower, ".gb")) return "Game Boy";
    if (ends_with_lower(lower, ".gbc")) return "Game Boy Color";
    if (ends_with_lower(lower, ".gba")) return "Game Boy Advance";
    if (ends_with_lower(lower, ".n64") || ends_with_lower(lower, ".z64") || ends_with_lower(lower, ".v64")) return "Nintendo 64";
    if (ends_with_lower(lower, ".md") || ends_with_lower(lower, ".gen") || ends_with_lower(lower, ".smd")) return "Sega Genesis";
    if (ends_with_lower(lower, ".sms")) return "Sega Master System";
    if (ends_with_lower(lower, ".gg")) return "Sega Game Gear";
    if (ends_with_lower(lower, ".a26")) return "Atari 2600";
    if (ends_with_lower(lower, ".a52")) return "Atari 5200";
    if (ends_with_lower(lower, ".a78")) return "Atari 7800";
    if (ends_with_lower(lower, ".lnx")) return "Atari Lynx";
    if (ends_with_lower(lower, ".atr") || ends_with_lower(lower, ".xfd") || ends_with_lower(lower, ".atx")) return "Atari 8-bit";
    if (ends_with_lower(lower, ".xex")) return lower_copy(basename_only(path)) == "default.xex" ? "Xbox 360" : "Atari 8-bit";
    if (ends_with_lower(lower, ".iso")) return "Xbox 360";
    if (ends_with_lower(lower, ".bin")) {
        if (context.find("sega") != std::string::npos || context.find("genesis") != std::string::npos || context.find("mega drive") != std::string::npos)
            return "Sega Genesis";
        return "Atari 2600";
    }
    if ((ends_with_lower(lower, ".car") || ends_with_lower(lower, ".rom") || ends_with_lower(lower, ".cas")) &&
        context.find("atari") != std::string::npos) return "Atari 8-bit";
    return {};
}

std::string sidecar_artwork(const std::string& source) {
    const fs::path path(source);
    const fs::path folder = path.parent_path();
    const std::string stem = path.stem().string();
    const fs::path candidates[] = {
        folder / (stem + ".png"), folder / (stem + ".jpg"), folder / (stem + ".jpeg"), folder / (stem + ".bmp"),
        folder / "cover.png", folder / "cover.jpg", folder / "cover.jpeg", folder / "cover.bmp"
    };
    for (const auto& candidate : candidates) if (regular_file(candidate)) return candidate.string();
    return {};
}

int dos_launcher_score(const fs::path& file, const std::string& directory_lower) {
    const std::string ext = lower_copy(file.extension().string());
    if (ext != ".exe" && ext != ".com" && ext != ".bat") return -1;
    const std::string stem = lower_copy(file.stem().string());
    static const std::set<std::string> reject = {"setup","install","installer","uninstall","config","configure","setsound","sound","readme","help"};
    if (reject.count(stem) != 0U || stem.rfind("unins", 0U) == 0U) return -1;
    int score = 10;
    if (stem == "start" || stem == "play" || stem == "run" || stem == "game") score += 100;
    std::string compact_dir;
    std::string compact_stem;
    for (unsigned char c : directory_lower) if (std::isalnum(c)) compact_dir.push_back(static_cast<char>(c));
    for (unsigned char c : stem) if (std::isalnum(c)) compact_stem.push_back(static_cast<char>(c));
    if (!compact_dir.empty() && !compact_stem.empty()) {
        if (compact_dir == compact_stem) score += 140;
        else if (compact_dir.find(compact_stem) != std::string::npos || compact_stem.find(compact_dir) != std::string::npos) score += 75;
    }
    return score;
}

std::string dos_entrypoint_for_directory(const fs::path& folder) {
    for (const char* name : {"NOU_LAUNCH.BAT", "NOU_LAUNCH.COM", "NOU_LAUNCH.EXE"}) {
        if (regular_file(folder / name)) return name;
    }
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) return {};
    const std::string folder_name = lower_copy(folder.filename().string());
    int best_score = -1;
    std::string best;
    for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        if (!it->is_regular_file(ec)) { ec.clear(); continue; }
        const int score = dos_launcher_score(it->path(), folder_name);
        if (score > best_score) { best_score = score; best = it->path().filename().string(); }
    }
    return best_score >= 10 ? best : std::string();
}

std::string pick_dos_entry_from_zip(const std::vector<std::string>& entries, const std::string& archive) {
    for (const std::string& entry : entries) {
        if (!safe_zip_member(entry) || entry.back() == '/') continue;
        if (!system_for_path(entry, archive).empty()) return {};
    }
    int best_score = -1;
    std::string best;
    const std::string archive_name = lower_copy(stem_only(archive));
    for (const std::string& entry : entries) {
        if (!safe_zip_member(entry) || entry.back() == '/') continue;
        int score = dos_launcher_score(fs::path(entry), archive_name);
        if (score < 0) continue;
        const std::string stem = lower_copy(fs::path(entry).stem().string());
        if (stem == "nou_launch") score += 2000;
        if (lower_copy(entry).find("original_dos/") != std::string::npos) score += 500;
        score -= static_cast<int>(std::count(entry.begin(), entry.end(), '/')) * 3;
        if (score > best_score) { best_score = score; best = entry; }
    }
    return best_score >= 10 ? best : std::string();
}

std::string choose_folder() {
    int code = 0;
    const std::vector<std::string> lines = capture_command({"zenity", "--file-selection", "--directory", "--title=Add Game Folder"}, &code);
    return code == 0 && !lines.empty() ? lines.front() : std::string();
}

std::uint32_t read_le32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8U) |
           (static_cast<std::uint32_t>(p[2]) << 16U) |
           (static_cast<std::uint32_t>(p[3]) << 24U);
}

std::uint16_t read_le16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8U);
}

bool load_bmp(const fs::path& path, BmpImage& image) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (bytes.size() < 54U || bytes[0] != 'B' || bytes[1] != 'M') return false;
    const std::uint32_t offset = read_le32(bytes.data() + 10);
    const int width = static_cast<int>(read_le32(bytes.data() + 18));
    const int raw_height = static_cast<int>(read_le32(bytes.data() + 22));
    const std::uint16_t bpp = read_le16(bytes.data() + 28);
    const std::uint32_t compression = read_le32(bytes.data() + 30);
    if (width <= 0 || raw_height == 0 || bpp != 24 || compression != 0) return false;
    const int height = std::abs(raw_height);
    const std::size_t stride = (static_cast<std::size_t>(width) * 3U + 3U) & ~3U;
    if (offset + stride * static_cast<std::size_t>(height) > bytes.size()) return false;
    image.width = width;
    image.height = height;
    image.rgb.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3U, 0U);
    const bool bottom_up = raw_height > 0;
    for (int y = 0; y < height; ++y) {
        const int source_y = bottom_up ? (height - 1 - y) : y;
        const unsigned char* row = bytes.data() + offset + stride * static_cast<std::size_t>(source_y);
        for (int x = 0; x < width; ++x) {
            const std::size_t src = static_cast<std::size_t>(x) * 3U;
            const std::size_t dst = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 3U;
            image.rgb[dst + 0] = row[src + 2];
            image.rgb[dst + 1] = row[src + 1];
            image.rgb[dst + 2] = row[src + 0];
        }
    }
    return true;
}

unsigned long channel_to_mask(unsigned char value, unsigned long mask) {
    if (mask == 0UL) return 0UL;
    unsigned long scaled = mask;
    int shift = 0;
    while ((scaled & 1UL) == 0UL) { scaled >>= 1U; ++shift; }
    const unsigned long converted = (static_cast<unsigned long>(value) * scaled + 127UL) / 255UL;
    return (converted << shift) & mask;
}

class GamesApp {
public:
    GamesApp(Display* display, Window parent, int width, int height, fs::path plugin_root,
             fs::path config_dir, fs::path cache_dir, fs::path state_dir, fs::path data_dir)
        : display_(display), parent_(parent), width_(width), height_(height), plugin_root_(std::move(plugin_root)),
          config_dir_(std::move(config_dir)), cache_dir_(std::move(cache_dir)), state_dir_(std::move(state_dir)),
          data_dir_(std::move(data_dir)) {}

    ~GamesApp() {
        g_running.store(false);
        emulator_.stop();
        if (scan_worker_.joinable()) scan_worker_.join();
        stop_artwork_worker();
        if (gc_ != 0) XFreeGC(display_, gc_);
        if (game_surface_ != 0) XDestroyWindow(display_, game_surface_);
        if (window_ != 0) XDestroyWindow(display_, window_);
    }

    bool initialize(std::string& error) {
        std::error_code ec;
        fs::create_directories(config_dir_, ec);
        ec.clear(); fs::create_directories(cache_dir_, ec);
        ec.clear(); fs::create_directories(state_dir_, ec);
        if (!directory_exists(plugin_root_ / "bundled")) {
            error = "Games plugin bundled starter library is missing.";
            return false;
        }
        const int screen = DefaultScreen(display_);
        Colormap cmap = DefaultColormap(display_, screen);
        navy_ = named_color(cmap, "#233758", BlackPixel(display_, screen));
        panel_ = named_color(cmap, "#2d4870", BlackPixel(display_, screen));
        field_ = named_color(cmap, "#f4e8cd", WhitePixel(display_, screen));
        ink_ = named_color(cmap, "#f4e8cd", WhitePixel(display_, screen));
        dark_ = named_color(cmap, "#18263e", BlackPixel(display_, screen));
        muted_ = named_color(cmap, "#cdd9e8", WhitePixel(display_, screen));
        select_ = named_color(cmap, "#43608d", WhitePixel(display_, screen));
        gold_ = named_color(cmap, "#d39037", WhitePixel(display_, screen));

        window_ = XCreateSimpleWindow(display_, parent_, 0, 0,
                                      static_cast<unsigned int>(std::max(1, width_)),
                                      static_cast<unsigned int>(std::max(1, height_)), 0, dark_, navy_);
        if (window_ == 0) { error = "Games could not create its Nougat plugin window."; return false; }
        XSelectInput(display_, window_, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        game_surface_ = XCreateSimpleWindow(display_, window_, 0, 48,
                                             static_cast<unsigned int>(std::max(1, width_)),
                                             static_cast<unsigned int>(std::max(1, height_ - 48)), 0, dark_, BlackPixel(display_, screen));
        XMapWindow(display_, window_);
        XFlush(display_);
        start_scan();
        return true;
    }

    int run() {
        while (g_running.load()) {
            resize_to_parent();
            poll_scan();
            poll_artwork_worker();
            poll_emulator();
            while (XPending(display_) > 0) {
                XEvent event{};
                XNextEvent(display_, &event);
                if (event.xany.window == window_) {
                    if (event.type == Expose && event.xexpose.count == 0) draw();
                    else if (event.type == ConfigureNotify) {
                        width_ = std::max(1, event.xconfigure.width);
                        height_ = std::max(1, event.xconfigure.height);
                        layout();
                        if (playing_) emulator_.resize(width_, std::max(1, height_ - 48));
                        draw();
                    } else if (event.type == ButtonPress) {
                        if (event.xbutton.button == Button1) handle_click(event.xbutton.x, event.xbutton.y, event.xbutton.time);
                        else if (event.xbutton.button == Button4) scroll_by(-1);
                        else if (event.xbutton.button == Button5) scroll_by(1);
                    } else if (event.type == KeyPress) {
                        KeySym sym = XLookupKeysym(&event.xkey, 0);
                        if (sym == XK_Escape && playing_) stop_game();
                        else if (sym == XK_Return && !playing_) launch_selected();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }
        return 0;
    }

private:
    Display* display_ = nullptr;
    Window parent_ = 0;
    Window window_ = 0;
    Window game_surface_ = 0;
    GC gc_ = 0;
    int width_ = 1;
    int height_ = 1;
    unsigned long navy_ = 0;
    unsigned long panel_ = 0;
    unsigned long field_ = 0;
    unsigned long ink_ = 0;
    unsigned long dark_ = 0;
    unsigned long muted_ = 0;
    unsigned long select_ = 0;
    unsigned long gold_ = 0;

    fs::path plugin_root_;
    fs::path config_dir_;
    fs::path cache_dir_;
    fs::path state_dir_;
    fs::path data_dir_;
    Panel panel_mode_ = Panel::Library;
    DisplayMode display_mode_ = DisplayMode::Grid;
    std::vector<GameEntry> games_;
    std::vector<Rect> game_rows_;
    std::vector<std::string> linked_folders_;
    std::thread scan_worker_;
    std::mutex games_mutex_;
    bool scanning_ = false;
    bool scan_updated_ = false;
    int selected_ = -1;
    int scroll_ = 0;
    Time last_click_time_ = 0;
    int last_click_index_ = -1;
    std::string status_ = "Game library ready.";
    nougat::games::EmulatorHost emulator_;
    bool playing_ = false;
    std::string playing_title_;
    std::string playing_system_;
    bool stella_options_armed_ = true;
    long long last_stella_options_ms_ = 0;
    pid_t artwork_pid_ = -1;
    std::map<std::string, BmpImage> bmp_cache_;
    std::set<std::string> bmp_failed_;

    Rect library_btn_{}, systems_btn_{}, add_btn_{}, controllers_btn_{}, settings_btn_{};
    Rect play_btn_{}, refresh_btn_{}, grid_btn_{}, list_btn_{}, body_{}, stop_game_btn_{};

    unsigned long named_color(Colormap cmap, const char* name, unsigned long fallback) {
        XColor exact{}; XColor screen{};
        return XAllocNamedColor(display_, cmap, name, &screen, &exact) != 0 ? screen.pixel : fallback;
    }

    fs::path data_root() const {
        fs::path root = data_dir_;
        if (!root.empty()) root = root.parent_path().parent_path();
        return root;
    }

    fs::path runtime_root(const std::string& component) const {
        return data_root() / "runtime" / component;
    }

    fs::path folders_file() const { return config_dir_ / "rom-folders.txt"; }

    void load_folders() {
        std::vector<std::string> folders;
        std::ifstream input(folders_file());
        std::string line;
        std::set<std::string> seen;
        while (std::getline(input, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            if (!line.empty() && seen.insert(line).second) folders.push_back(line);
        }
        linked_folders_ = std::move(folders);
    }

    bool save_folders() {
        std::error_code ec;
        fs::create_directories(config_dir_, ec);
        if (ec) return false;
        const fs::path temporary = folders_file().string() + ".tmp";
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return false;
        for (const std::string& folder : linked_folders_) output << folder << '\n';
        output.close();
        if (!output) return false;
        ::chmod(temporary.c_str(), 0600);
        fs::rename(temporary, folders_file(), ec);
        if (ec) return false;
        ::chmod(folders_file().c_str(), 0600);
        return true;
    }

    void add_folder() {
        const std::string folder = choose_folder();
        if (folder.empty()) return;
        load_folders();
        if (std::find(linked_folders_.begin(), linked_folders_.end(), folder) == linked_folders_.end()) linked_folders_.push_back(folder);
        if (!save_folders()) {
            status_ = "Nougat Games could not save that ROM folder.";
            draw();
            return;
        }
        start_scan();
    }

    void add_game(std::vector<GameEntry>& out, std::set<std::string>& seen, GameEntry game) {
        const std::string key = game.archived ? game.path + "::" + game.archive_entry : game.path;
        if (!seen.insert(key).second) return;
        if (game.title.empty()) game.title = display_title(game.archived ? game.archive_entry : game.path);
        if (game.artwork_path.empty() && !game.archived) game.artwork_path = sidecar_artwork(game.path);
        const std::string art_key = std::to_string(std::hash<std::string>{}(key));
        game.prepared_artwork = (cache_dir_ / "prepared" / (art_key + ".bmp")).string();
        out.push_back(std::move(game));
    }

    void scan_directory(const fs::path& root, bool bundled, std::vector<GameEntry>& out, std::set<std::string>& seen) {
        std::error_code ec;
        if (!fs::is_directory(root, ec)) return;

        const std::string direct_launcher = bundled ? std::string() : dos_entrypoint_for_directory(root);
        if (!direct_launcher.empty()) {
            GameEntry game;
            game.path = root.string(); game.title = display_title(root.filename().string()); game.system = "DOS";
            game.directory_game = true; game.entry_point = direct_launcher; game.bundled = bundled;
            add_game(out, seen, std::move(game));
            return;
        }

        for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (it->is_symlink(ec)) { ec.clear(); continue; }
            if (it->is_directory(ec)) {
                ec.clear();
                if (!bundled) {
                    const std::string launcher = dos_entrypoint_for_directory(it->path());
                    if (!launcher.empty()) {
                        GameEntry game;
                        game.path = it->path().string(); game.title = display_title(it->path().filename().string()); game.system = "DOS";
                        game.directory_game = true; game.entry_point = launcher; game.bundled = false;
                        add_game(out, seen, std::move(game));
                        it.disable_recursion_pending();
                    }
                }
                continue;
            }
            if (!it->is_regular_file(ec)) { ec.clear(); continue; }
            const std::string path = it->path().string();
            const std::string system = system_for_path(path, root.string());
            if (!system.empty()) {
                GameEntry game;
                game.path = path; game.title = display_title(path); game.system = system; game.bundled = bundled;
                if (bundled) {
                    const fs::path artwork = plugin_root_ / "bundled" / "artwork" / (it->path().stem().string() + ".png");
                    if (regular_file(artwork)) game.bundled_artwork = artwork.string();
                }
                add_game(out, seen, std::move(game));
                continue;
            }
            if (!ends_with_lower(path, ".zip")) continue;
            int code = 0;
            const std::vector<std::string> entries = capture_command({"unzip", "-Z1", path}, &code);
            if (code != 0 || entries.empty()) continue;
            const std::string dos_entry = pick_dos_entry_from_zip(entries, path);
            if (!dos_entry.empty()) {
                GameEntry game;
                game.path = path; game.title = display_title(path); game.system = "DOS"; game.bundled = bundled;
                game.archived = true; game.archive_entry = dos_entry; game.entry_point = dos_entry; game.directory_game = true;
                add_game(out, seen, std::move(game));
                continue;
            }
            for (const std::string& entry : entries) {
                if (!safe_zip_member(entry) || (!entry.empty() && entry.back() == '/')) continue;
                const std::string archived_system = system_for_path(entry, path);
                if (archived_system.empty()) continue;
                GameEntry game;
                game.path = path; game.title = display_title(entry); game.system = archived_system; game.bundled = bundled;
                game.archived = true; game.archive_entry = entry;
                add_game(out, seen, std::move(game));
            }
        }
    }

    void start_scan() {
        if (scanning_) return;
        if (scan_worker_.joinable()) scan_worker_.join();
        load_folders();
        const std::vector<std::string> folders = linked_folders_;
        scanning_ = true;
        status_ = "Scanning bundled and linked game folders...";
        scan_worker_ = std::thread([this, folders]() {
            std::vector<GameEntry> found;
            std::set<std::string> seen;
            scan_directory(plugin_root_ / "bundled", true, found, seen);
            for (const std::string& folder : folders) scan_directory(folder, false, found, seen);
            std::stable_sort(found.begin(), found.end(), [](const GameEntry& a, const GameEntry& b) {
                if (a.system != b.system) return a.system < b.system;
                return lower_copy(a.title) < lower_copy(b.title);
            });
            {
                std::lock_guard<std::mutex> lock(games_mutex_);
                games_ = std::move(found);
                scanning_ = false;
                scan_updated_ = true;
                status_ = games_.empty() ? "No games found. Add a ROM folder." : std::to_string(games_.size()) + " games indexed.";
            }
        });
    }

    void poll_scan() {
        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(games_mutex_);
            updated = scan_updated_;
            scan_updated_ = false;
        }
        if (!updated) return;
        if (!scanning_ && scan_worker_.joinable()) scan_worker_.join();
        selected_ = games_.empty() ? -1 : std::max(0, std::min(selected_, static_cast<int>(games_.size()) - 1));
        if (selected_ < 0 && !games_.empty()) selected_ = 0;
        scroll_ = 0;
        start_artwork_worker();
        draw();
    }

    static std::string manifest_escape(const std::string& value) {
        std::string out;
        for (char c : value) {
            if (c == '\\') out += "\\\\";
            else if (c == '\t') out += "\\t";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else out.push_back(c);
        }
        return out;
    }

    void start_artwork_worker() {
        stop_artwork_worker();
        const fs::path worker = plugin_root_ / "artwork_cache_worker.py";
        if (!regular_file(worker)) return;
        const fs::path manifest = state_dir_ / "artwork-manifest.tsv";
        fs::create_directories(manifest.parent_path());
        std::ofstream output(manifest, std::ios::trunc);
        if (!output) return;
        for (const GameEntry& game : games_) {
            const std::string source = game.archived ? game.archive_entry : game.path;
            const std::string stem = stem_only(source);
            const std::string remote = (cache_dir_ / "remote" / (std::to_string(std::hash<std::string>{}(game.system + "|" + source)) + ".png")).string();
            output << manifest_escape(game.system) << '\t'
                   << manifest_escape(game.path + (game.archived ? "::" + game.archive_entry : "")) << '\t'
                   << manifest_escape(stem) << '\t'
                   << manifest_escape(game.title) << '\t'
                   << manifest_escape(game.artwork_path) << '\t'
                   << manifest_escape(game.bundled_artwork) << '\t'
                   << manifest_escape(remote) << '\t'
                   << manifest_escape(game.prepared_artwork) << '\n';
        }
        output.close();
        const pid_t child = ::fork();
        if (child == 0) {
            ::execlp("python3", "python3", worker.c_str(), "--manifest", manifest.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        if (child > 0) artwork_pid_ = child;
    }

    void stop_artwork_worker() {
        if (artwork_pid_ <= 1) { artwork_pid_ = -1; return; }
        int status = 0;
        const pid_t result = ::waitpid(artwork_pid_, &status, WNOHANG);
        if (result == artwork_pid_) { artwork_pid_ = -1; return; }
        (void)::kill(artwork_pid_, SIGTERM);
        for (int i = 0; i < 20; ++i) {
            const pid_t waited = ::waitpid(artwork_pid_, &status, WNOHANG);
            if (waited == artwork_pid_ || waited < 0) { artwork_pid_ = -1; return; }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        (void)::kill(artwork_pid_, SIGKILL);
        while (::waitpid(artwork_pid_, &status, 0) < 0 && errno == EINTR) {}
        artwork_pid_ = -1;
    }

    void poll_artwork_worker() {
        if (artwork_pid_ <= 1) return;
        int status = 0;
        const pid_t result = ::waitpid(artwork_pid_, &status, WNOHANG);
        if (result == 0) return;
        artwork_pid_ = -1;
        bmp_cache_.clear(); bmp_failed_.clear();
        draw();
    }

    std::string emulator_for_system(const std::string& system) const {
        const fs::path mesen = runtime_root("mesen2") / "Mesen";
        const fs::path rmg = runtime_root("rmg") / "AppRun";
        const fs::path atari800 = runtime_root("atari800") / "AppRun";
        const fs::path stella = runtime_root("stella") / "stella";
        const fs::path blastem = runtime_root("blastem") / "blastem";
        if (system == "Atari 2600") return first_executable({stella.string(), "stella"});
        if (system == "Sega Genesis" || system == "Sega Master System" || system == "Sega Game Gear") return first_executable({blastem.string(), "blastem"});
        if (system == "NES" || system == "SNES" || system == "Game Boy" || system == "Game Boy Color" || system == "Game Boy Advance") return first_executable({mesen.string(), "mesen", "fceux", "nestopia", "snes9x", "mgba"});
        if (system == "Nintendo 64") return first_executable({rmg.string(), "RMG", "mupen64plus"});
        if (system == "Atari 5200" || system == "Atari 8-bit") return first_executable({atari800.string(), "atari800"});
        if (system == "Atari 7800") return first_executable({"a7800"});
        if (system == "Atari Lynx") return first_executable({"mednafen", "retroarch"});
        return {};
    }

    fs::path extract_member(const GameEntry& game) {
        if (!game.archived || !safe_zip_member(game.archive_entry)) return game.path;
        const fs::path dir = cache_dir_ / "extracted" / std::to_string(std::hash<std::string>{}(game.path));
        const fs::path target = dir / fs::path(game.archive_entry).filename();
        if (regular_file(target)) return target;
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) return {};
        int pipefd[2] = {-1, -1};
        if (::pipe(pipefd) != 0) return {};
        const pid_t child = ::fork();
        if (child < 0) { ::close(pipefd[0]); ::close(pipefd[1]); return {}; }
        if (child == 0) {
            (void)::dup2(pipefd[1], STDOUT_FILENO);
            ::close(pipefd[0]); ::close(pipefd[1]);
            ::execlp("unzip", "unzip", "-p", game.path.c_str(), game.archive_entry.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        ::close(pipefd[1]);
        std::ofstream output(target, std::ios::binary | std::ios::trunc);
        char buffer[16384]{};
        bool write_ok = static_cast<bool>(output);
        for (;;) {
            const ssize_t n = ::read(pipefd[0], buffer, sizeof(buffer));
            if (n > 0) { if (write_ok) output.write(buffer, n); }
            else if (n < 0 && errno == EINTR) continue;
            else break;
        }
        ::close(pipefd[0]); output.close();
        int status = 0; while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        if (!write_ok || !WIFEXITED(status) || WEXITSTATUS(status) != 0 || !regular_file(target)) { fs::remove(target, ec); return {}; }
        ::chmod(target.c_str(), 0600);
        return target;
    }

    bool extract_dos_archive(const GameEntry& game, fs::path& folder, std::string& entrypoint) {
        if (!game.archived) { folder = game.path; entrypoint = game.entry_point; return directory_exists(folder); }
        int code = 0;
        const std::vector<std::string> entries = capture_command({"unzip", "-Z1", game.path}, &code);
        if (code != 0 || entries.empty()) return false;
        for (const std::string& member : entries) if (!safe_zip_member(member)) return false;
        folder = cache_dir_ / "dos" / std::to_string(std::hash<std::string>{}(game.path));
        std::error_code ec; fs::create_directories(folder, ec); if (ec) return false;
        for (const std::string& member : entries) {
            if (member.empty() || member.back() == '/') continue;
            fs::path relative(member);
            fs::path target = folder / relative;
            fs::create_directories(target.parent_path(), ec); if (ec) return false;
            GameEntry one = game; one.archive_entry = member;
            int pipefd[2] = {-1, -1}; if (::pipe(pipefd) != 0) return false;
            const pid_t child = ::fork(); if (child < 0) { ::close(pipefd[0]); ::close(pipefd[1]); return false; }
            if (child == 0) {
                (void)::dup2(pipefd[1], STDOUT_FILENO); ::close(pipefd[0]); ::close(pipefd[1]);
                ::execlp("unzip", "unzip", "-p", game.path.c_str(), member.c_str(), static_cast<char*>(nullptr)); _exit(127);
            }
            ::close(pipefd[1]); std::ofstream output(target, std::ios::binary | std::ios::trunc);
            char buffer[16384]{}; for (;;) { const ssize_t n = ::read(pipefd[0], buffer, sizeof(buffer)); if (n > 0) output.write(buffer, n); else if (n < 0 && errno == EINTR) continue; else break; }
            ::close(pipefd[0]); output.close(); int status = 0; while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return false;
        }
        fs::path wanted = folder / fs::path(game.entry_point);
        if (regular_file(wanted)) {
            entrypoint = fs::relative(wanted, folder, ec).string();
            if (!ec) return true;
        }
        entrypoint = dos_entrypoint_for_directory(folder);
        return !entrypoint.empty();
    }

    bool make_launch_request(const GameEntry& selected, const fs::path& launch_path,
                             nougat::games::LaunchRequest& request, std::string& error) {
        request.title = selected.title;
        request.log_path = (state_dir_ / "embedded-emulator.log").string();
        request.window_timeout_ms = selected.system == "Xbox 360" ? 90000 : 45000;
        request.environment = {{"SDL_VIDEODRIVER","x11"},{"SDL_VIDEO_DRIVER","x11"},{"QT_QPA_PLATFORM","xcb"},{"GDK_BACKEND","x11"}};

        if (selected.system == "DOS") {
            const char* override_value = std::getenv("NOUGAT_DOSBOX");
            std::vector<std::string> candidates;
            if (override_value && *override_value) candidates.emplace_back(override_value);
            candidates.push_back((runtime_root("dosbox-staging") / "dosbox").string());
            candidates.push_back("dosbox-staging"); candidates.push_back("dosbox");
            const std::string emulator = first_executable(candidates);
            if (emulator.empty()) { error = "DOSBox Staging/DOSBox is unavailable."; return false; }
            if (selected.entry_point.find('"') != std::string::npos || selected.entry_point.find('\n') != std::string::npos || selected.entry_point.find('\r') != std::string::npos) {
                error = "The detected DOS launcher name is unsafe."; return false;
            }
            request.backend = basename_only(emulator);
            request.argv = {emulator, "--noprimaryconf", "--nolocalconf", "--set", "fullscreen=off", "--set", "output=texture",
                            "-c", "mount c \"" + launch_path.string() + "\"", "-c", "c:", "-c", selected.entry_point, "-c", "exit"};
            return true;
        }

        if (selected.system == "Xbox 360") {
            const char* override_value = std::getenv("NOUGAT_XENIA");
            std::vector<std::string> candidates;
            if (override_value && *override_value) candidates.emplace_back(override_value);
            candidates.push_back((runtime_root("xenia") / "xenia_canary").string());
            candidates.push_back((runtime_root("xenia") / "xenia").string());
            candidates.push_back("xenia_canary"); candidates.push_back("xenia-canary"); candidates.push_back("xenia");
            const std::string emulator = first_executable(candidates);
            if (emulator.empty()) { error = "Xenia Canary is unavailable."; return false; }
            request.environment.push_back({"APPIMAGE_EXTRACT_AND_RUN","1"});
            request.backend = basename_only(emulator); request.argv = {emulator, launch_path.string()}; return true;
        }

        const std::string emulator = emulator_for_system(selected.system);
        if (emulator.empty()) { error = "No supported " + selected.system + " emulator is available."; return false; }
        const std::string backend = lower_copy(basename_only(emulator));
        request.backend = basename_only(emulator);
        if (backend == "mesen" || backend == "mesen2") request.argv = {emulator, launch_path.string(), "--fullscreen"};
        else if (backend == "blastem") {
            request.argv = {emulator, "-m", selected.system == "Sega Master System" ? "sms" : (selected.system == "Sega Game Gear" ? "gg" : "gen"), launch_path.string()};
        } else if (backend == "stella") request.argv = {emulator, "-fullscreen", "0", "-center", "0", launch_path.string()};
        else if (selected.system == "Atari 5200" && (backend == "atari800" || emulator.find("/atari800/") != std::string::npos)) request.argv = {emulator, "-5200", launch_path.string()};
        else if (backend == "retroarch" && selected.system == "Atari Lynx") request.argv = {emulator, "-L", "handy_libretro.so", launch_path.string()};
        else request.argv = {emulator, launch_path.string()};
        return true;
    }

    void launch_selected() {
        if (playing_) return;
        GameEntry selected;
        {
            std::lock_guard<std::mutex> lock(games_mutex_);
            if (selected_ < 0 || selected_ >= static_cast<int>(games_.size())) { status_ = "Select a game first."; draw(); return; }
            selected = games_[static_cast<std::size_t>(selected_)];
        }
        fs::path launch_path;
        GameEntry launch_selected = selected;
        if (selected.system == "DOS" && selected.directory_game) {
            std::string entry;
            if (!extract_dos_archive(selected, launch_path, entry)) { status_ = "Nougat could not safely prepare that DOS game package."; draw(); return; }
            launch_selected.entry_point = entry;
        } else {
            launch_path = selected.archived ? extract_member(selected) : fs::path(selected.path);
            if (!regular_file(launch_path)) { status_ = "That game is unavailable. Refresh after reconnecting its folder."; draw(); return; }
        }
        nougat::games::LaunchRequest request;
        std::string error;
        if (!make_launch_request(launch_selected, launch_path, request, error)) { status_ = error; draw(); return; }
        layout();
        XMapRaised(display_, game_surface_);
        XFlush(display_);
        if (!emulator_.start(display_, window_, game_surface_, width_, std::max(1, height_ - 48), request, error)) {
            XUnmapWindow(display_, game_surface_); status_ = error; draw(); return;
        }
        playing_ = true; playing_title_ = selected.title; playing_system_ = selected.system;
        status_ = "Starting " + selected.title + " inside Nougat Games...";
        draw();
    }

    void stop_game() {
        emulator_.stop();
        playing_ = false; playing_title_.clear(); playing_system_.clear(); stella_options_armed_ = true;
        XUnmapWindow(display_, game_surface_);
        status_ = "Game closed. Library ready.";
        draw();
    }

    long long now_ms() const {
        using namespace std::chrono;
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void poll_emulator() {
        if (!playing_ && !emulator_.active()) return;
        const nougat::games::HostEvent event = emulator_.poll();
        if (event.changed && !event.message.empty()) { status_ = event.message; draw(); }
        if (event.state == nougat::games::HostState::Failed || event.state == nougat::games::HostState::Exited) {
            playing_ = false; playing_title_.clear(); playing_system_.clear(); XUnmapWindow(display_, game_surface_); draw(); return;
        }
        if (event.state == nougat::games::HostState::Embedded) {
            emulator_.resize(width_, std::max(1, height_ - 48));
            emulator_.focus();
        }
        if (playing_ && playing_system_ == "Atari 2600" && emulator_.embedded()) {
            int x = 0, y = 0;
            if (emulator_.pointer_position(x, y)) {
                const long long now = now_ms();
                if (y > 48) stella_options_armed_ = true;
                if (x >= 0 && y >= 0 && y <= 6 && stella_options_armed_ && now - last_stella_options_ms_ >= 1000) {
                    if (emulator_.send_key(XK_Tab)) { stella_options_armed_ = false; last_stella_options_ms_ = now; }
                }
            }
        }
    }

    void resize_to_parent() {
        XWindowAttributes attr{};
        if (XGetWindowAttributes(display_, parent_, &attr) == 0) return;
        const int w = std::max(1, attr.width), h = std::max(1, attr.height);
        if (w == width_ && h == height_) return;
        width_ = w; height_ = h;
        XMoveResizeWindow(display_, window_, 0, 0, static_cast<unsigned int>(width_), static_cast<unsigned int>(height_));
        layout();
        if (playing_) {
            XMoveResizeWindow(display_, game_surface_, 0, 48, static_cast<unsigned int>(width_), static_cast<unsigned int>(std::max(1, height_ - 48)));
            emulator_.resize(width_, std::max(1, height_ - 48));
        }
        draw();
    }

    void layout() {
        const int bw = 104, bh = 30, gap = 4;
        int x = 18;
        library_btn_ = {x, 14, bw, bh}; x += bw + gap;
        systems_btn_ = {x, 14, bw, bh}; x += bw + gap;
        add_btn_ = {x, 14, bw, bh}; x += bw + gap;
        controllers_btn_ = {x, 14, bw, bh}; x += bw + gap;
        settings_btn_ = {x, 14, bw, bh};
        play_btn_ = {18, 55, 104, 30}; refresh_btn_ = {126, 55, 104, 30};
        grid_btn_ = {std::max(18, width_ - 230), 55, 104, 30}; list_btn_ = {std::max(126, width_ - 122), 55, 104, 30};
        body_ = {18, 95, std::max(220, width_ - 36), std::max(120, height_ - 113)};
        stop_game_btn_ = {18, 10, 124, 30};
        XMoveResizeWindow(display_, game_surface_, 0, 48, static_cast<unsigned int>(width_), static_cast<unsigned int>(std::max(1, height_ - 48)));
    }

    void fill(Rect r, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XFillRectangle(display_, window_, gc_, r.x, r.y, static_cast<unsigned int>(std::max(1, r.w)), static_cast<unsigned int>(std::max(1, r.h)));
    }

    void outline(Rect r, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XDrawRectangle(display_, window_, gc_, r.x, r.y, static_cast<unsigned int>(std::max(1, r.w - 1)), static_cast<unsigned int>(std::max(1, r.h - 1)));
    }

    void text(int x, int y, const std::string& value, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XDrawString(display_, window_, gc_, x, y, value.c_str(), static_cast<int>(value.size()));
    }

    void button(Rect r, const std::string& label, bool active = false) {
        fill(r, active ? gold_ : panel_); outline(r, dark_); outline({r.x + 2, r.y + 2, std::max(1, r.w - 4), std::max(1, r.h - 4)}, active ? field_ : select_);
        text(r.x + 9, r.y + 20, safe_clip(label, static_cast<std::size_t>(std::max(1, r.w / 7))), active ? dark_ : ink_);
    }

    int visible_list_rows() const { return std::max(1, (body_.h - 12) / 34); }

    void scroll_by(int direction) {
        if (playing_ || panel_mode_ != Panel::Library) return;
        const int visible = display_mode_ == DisplayMode::List ? visible_list_rows() : std::max(1, ((body_.w - 12) / 166) * std::max(1, (body_.h - 12) / 220));
        const int max_scroll = std::max(0, static_cast<int>(games_.size()) - visible);
        const int step = display_mode_ == DisplayMode::List ? 1 : std::max(1, (body_.w - 12) / 166);
        scroll_ = std::max(0, std::min(max_scroll, scroll_ + direction * step));
        draw();
    }

    bool cached_image(const GameEntry& game, BmpImage& image) {
        if (game.prepared_artwork.empty() || !regular_file(game.prepared_artwork)) return false;
        auto found = bmp_cache_.find(game.prepared_artwork);
        if (found != bmp_cache_.end()) { image = found->second; return true; }
        if (bmp_failed_.count(game.prepared_artwork) != 0U) return false;
        BmpImage loaded;
        if (!load_bmp(game.prepared_artwork, loaded)) { bmp_failed_.insert(game.prepared_artwork); return false; }
        bmp_cache_[game.prepared_artwork] = loaded; image = std::move(loaded); return true;
    }

    void draw_image(Rect target, const BmpImage& image) {
        if (image.width <= 0 || image.height <= 0 || image.rgb.empty()) return;
        const int depth = DefaultDepth(display_, DefaultScreen(display_));
        char* data = static_cast<char*>(std::calloc(static_cast<std::size_t>(target.w) * static_cast<std::size_t>(target.h), 4U));
        if (!data) return;
        XImage* ximage = XCreateImage(display_, DefaultVisual(display_, DefaultScreen(display_)), depth, ZPixmap, 0, data,
                                     target.w, target.h, 32, 0);
        if (!ximage) { std::free(data); return; }
        Visual* visual = DefaultVisual(display_, DefaultScreen(display_));
        for (int y = 0; y < target.h; ++y) {
            const int sy = y * image.height / std::max(1, target.h);
            for (int x = 0; x < target.w; ++x) {
                const int sx = x * image.width / std::max(1, target.w);
                const std::size_t offset = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(sx)) * 3U;
                const unsigned long pixel = channel_to_mask(image.rgb[offset], visual->red_mask) |
                                            channel_to_mask(image.rgb[offset + 1], visual->green_mask) |
                                            channel_to_mask(image.rgb[offset + 2], visual->blue_mask);
                XPutPixel(ximage, x, y, pixel);
            }
        }
        XPutImage(display_, window_, gc_, ximage, 0, 0, target.x, target.y, target.w, target.h);
        XDestroyImage(ximage);
    }

    void draw() {
        if (window_ == 0) return;
        layout();
        fill({0,0,width_,height_}, navy_);
        if (playing_) {
            fill({0,0,width_,48}, navy_);
            button(stop_game_btn_, "Stop Game", true);
            text(stop_game_btn_.x + stop_game_btn_.w + 14, 30, safe_clip(playing_title_ + " | " + playing_system_, 90), ink_);
            XMapRaised(display_, game_surface_);
            XFlush(display_);
            return;
        }
        XUnmapWindow(display_, game_surface_);
        button(library_btn_, "Library", panel_mode_ == Panel::Library);
        button(systems_btn_, "Systems", panel_mode_ == Panel::Systems);
        button(add_btn_, "Add Games");
        button(controllers_btn_, "Controllers", panel_mode_ == Panel::Controllers);
        button(settings_btn_, "Settings", panel_mode_ == Panel::Settings);
        button(play_btn_, "Play"); button(refresh_btn_, "Refresh");
        if (panel_mode_ == Panel::Library) { button(grid_btn_, "Grid", display_mode_ == DisplayMode::Grid); button(list_btn_, "List", display_mode_ == DisplayMode::List); }
        outline(body_, dark_); fill({body_.x + 1, body_.y + 1, std::max(1, body_.w - 2), std::max(1, body_.h - 2)}, panel_);
        text(242, 76, safe_clip(std::string(scanning_ ? "Scanning | " : "") + status_, static_cast<std::size_t>(std::max(20, (width_ - 480) / 7))), ink_);
        game_rows_.clear();

        if (panel_mode_ == Panel::Library) {
            if (games_.empty() && !scanning_) text(body_.x + 16, body_.y + 30, "No games indexed. Add a ROM folder; legal starter ROMs are bundled with this plugin.", muted_);
            if (display_mode_ == DisplayMode::List) {
                const int visible = visible_list_rows();
                scroll_ = std::max(0, std::min(scroll_, std::max(0, static_cast<int>(games_.size()) - visible)));
                for (int i = 0; i < visible; ++i) {
                    const int index = scroll_ + i; if (index >= static_cast<int>(games_.size())) break;
                    Rect row{body_.x + 6, body_.y + 6 + i * 34, body_.w - 12, 32};
                    if (index == selected_) fill(row, select_); outline(row, dark_);
                    const GameEntry& game = games_[static_cast<std::size_t>(index)];
                    text(row.x + 8, row.y + 21, safe_clip(game.title, static_cast<std::size_t>(std::max(10, (row.w - 300) / 7))), ink_);
                    const std::string source = game.system + " | " + (game.bundled ? "Bundled" : "Linked") + (game.archived ? " | ZIP" : "");
                    text(std::max(row.x + 140, row.x + row.w - 285), row.y + 21, safe_clip(source, 38), muted_);
                    game_rows_.push_back(row);
                }
            } else {
                const int gap = 8;
                const int columns = std::max(1, (body_.w - 12 + gap) / 166);
                const int tile_w = std::max(110, (body_.w - 12 - (columns - 1) * gap) / columns);
                const int art_h = std::min(190, std::max(110, tile_w * 3 / 2));
                const int tile_h = art_h + 48;
                const int rows = std::max(1, (body_.h - 12 + gap) / (tile_h + gap));
                const int visible = columns * rows;
                scroll_ = std::max(0, std::min(scroll_, std::max(0, static_cast<int>(games_.size()) - visible)));
                for (int i = 0; i < visible; ++i) {
                    const int index = scroll_ + i; if (index >= static_cast<int>(games_.size())) break;
                    const int col = i % columns, row_num = i / columns;
                    Rect card{body_.x + 6 + col * (tile_w + gap), body_.y + 6 + row_num * (tile_h + gap), tile_w, tile_h};
                    if (index == selected_) fill(card, select_); outline(card, dark_);
                    Rect art{card.x + 4, card.y + 4, card.w - 8, art_h}; fill(art, dark_);
                    const GameEntry& game = games_[static_cast<std::size_t>(index)];
                    BmpImage image; if (cached_image(game, image)) draw_image(art, image); else text(art.x + std::max(6, art.w / 2 - 22), art.y + art.h / 2, "NO ART", muted_);
                    text(card.x + 6, card.y + art_h + 21, safe_clip(game.title, static_cast<std::size_t>(std::max(8, (card.w - 12) / 7))), ink_);
                    text(card.x + 6, card.y + card.h - 8, safe_clip(game.system + " | " + (game.bundled ? "Bundled" : "Linked"), static_cast<std::size_t>(std::max(8, (card.w - 12) / 7))), muted_);
                    game_rows_.push_back(card);
                }
            }
        } else if (panel_mode_ == Panel::Systems) {
            int y = body_.y + 28; text(body_.x + 14, y, "EMULATION BACKENDS", ink_); y += 30;
            const std::vector<std::string> systems = {"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Sega Genesis","Sega Master System","Sega Game Gear","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx","DOS","Xbox 360"};
            for (const std::string& system : systems) {
                std::string backend;
                if (system == "DOS") backend = first_executable({(runtime_root("dosbox-staging") / "dosbox").string(), "dosbox-staging", "dosbox"});
                else if (system == "Xbox 360") backend = first_executable({(runtime_root("xenia") / "xenia_canary").string(), (runtime_root("xenia") / "xenia").string(), "xenia_canary", "xenia"});
                else backend = emulator_for_system(system);
                text(body_.x + 14, y, system + ": " + (backend.empty() ? "No supported backend installed" : basename_only(backend) + " (automatic)"), backend.empty() ? muted_ : ink_); y += 22;
            }
        } else if (panel_mode_ == Panel::Controllers) {
            int y = body_.y + 28; text(body_.x + 14, y, "CONTROLLERS", ink_); y += 30;
            std::error_code ec; const bool manta = fs::exists("/dev/input/by-id/usb-081f_USB_gamepad-joystick", ec);
            text(body_.x + 14, y, manta ? "Manta USB gamepad 081f:e401: detected and ready." : "Manta USB gamepad 081f:e401: not currently connected.", manta ? ink_ : muted_); y += 24;
            int detected = 0; if (fs::is_directory("/dev/input", ec)) for (const auto& entry : fs::directory_iterator("/dev/input", ec)) if (entry.path().filename().string().rfind("js", 0U) == 0U) ++detected;
            text(body_.x + 14, y, "Linux joystick devices detected: " + std::to_string(detected) + ". D-pad, A/B, Start and Select use normal Linux input.", ink_);
            text(body_.x + 14, y + 26, "Per-system button mapping remains owned by the selected emulator backend.", muted_);
        } else {
            int y = body_.y + 28; text(body_.x + 14, y, "GAME LIBRARY SETTINGS", ink_); y += 30;
            text(body_.x + 14, y, "Bundled library: " + (plugin_root_ / "bundled").string(), ink_); y += 24;
            load_folders();
            if (linked_folders_.empty()) text(body_.x + 14, y, "No user game folders linked yet.", muted_);
            for (const std::string& folder : linked_folders_) { text(body_.x + 14, y, safe_clip(folder, static_cast<std::size_t>(std::max(20, (body_.w - 28) / 7))), ink_); y += 22; }
            text(body_.x + 14, body_.y + body_.h - 38, "Cartridge ZIPs stay zipped; DOS ZIP packages prepare in the Games plugin cache.", muted_);
            text(body_.x + 14, body_.y + body_.h - 18, "Artwork: local sidecar, bundled cover, then cached Libretro artwork.", muted_);
        }
        XFlush(display_);
    }

    void handle_click(int x, int y, Time when) {
        if (playing_) { if (stop_game_btn_.contains(x, y)) stop_game(); return; }
        if (library_btn_.contains(x,y)) { panel_mode_ = Panel::Library; draw(); return; }
        if (systems_btn_.contains(x,y)) { panel_mode_ = Panel::Systems; draw(); return; }
        if (controllers_btn_.contains(x,y)) { panel_mode_ = Panel::Controllers; draw(); return; }
        if (settings_btn_.contains(x,y)) { panel_mode_ = Panel::Settings; draw(); return; }
        if (add_btn_.contains(x,y)) { add_folder(); return; }
        if (refresh_btn_.contains(x,y)) { start_scan(); draw(); return; }
        if (play_btn_.contains(x,y)) { launch_selected(); return; }
        if (panel_mode_ == Panel::Library && grid_btn_.contains(x,y)) { display_mode_ = DisplayMode::Grid; scroll_ = 0; draw(); return; }
        if (panel_mode_ == Panel::Library && list_btn_.contains(x,y)) { display_mode_ = DisplayMode::List; scroll_ = 0; draw(); return; }
        if (panel_mode_ == Panel::Library) {
            for (std::size_t i = 0; i < game_rows_.size(); ++i) {
                if (!game_rows_[i].contains(x,y)) continue;
                const int index = scroll_ + static_cast<int>(i);
                const bool double_click = index == last_click_index_ && last_click_time_ != 0 && when >= last_click_time_ && when - last_click_time_ <= 450;
                selected_ = index; last_click_index_ = index; last_click_time_ = when; draw();
                if (double_click) { last_click_time_ = 0; launch_selected(); }
                return;
            }
        }
    }
};

bool parse_ulong(const char* text, unsigned long& value) {
    if (!text || !*text) return false;
    try {
        std::size_t used = 0; const std::string input(text); value = std::stoul(input, &used, 10); return used == input.size() && value != 0UL;
    } catch (...) { return false; }
}

int env_dimension(const char* name, int fallback) {
    const char* text = std::getenv(name); if (!text || !*text) return fallback;
    try { return std::max(1, std::stoi(text)); } catch (...) { return fallback; }
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "Nougat Games Plugin v0.0.50\n";
        return 0;
    }
    if (argc <= 1 || std::string(argv[1]) != "--nougat-plugin") {
        std::cerr << "Games is a Nougat plugin. Launch it through Nougat Media Suite.\n";
        return 2;
    }
    const char* api = std::getenv("NOUGAT_PLUGIN_API");
    const char* id = std::getenv("NOUGAT_PLUGIN_ID");
    if (!api || std::string(api) != "1" || !id || std::string(id) != "games") {
        std::cerr << "Games plugin host contract is missing or incompatible.\n";
        return 3;
    }
    unsigned long parent = 0;
    if (!parse_ulong(std::getenv("NOUGAT_PLUGIN_PARENT_XID"), parent)) {
        std::cerr << "Games plugin parent X11 surface is missing.\n";
        return 4;
    }
    const fs::path plugin_root = env_path("NOUGAT_PLUGIN_ROOT");
    const fs::path config_dir = env_path("NOUGAT_PLUGIN_CONFIG_DIR");
    const fs::path cache_dir = env_path("NOUGAT_PLUGIN_CACHE_DIR");
    const fs::path state_dir = env_path("NOUGAT_PLUGIN_STATE_DIR");
    const fs::path data_dir = env_path("NOUGAT_PLUGIN_DATA_DIR");
    if (plugin_root.empty() || config_dir.empty() || cache_dir.empty() || state_dir.empty() || data_dir.empty()) {
        std::cerr << "Games plugin runtime directories are incomplete.\n";
        return 5;
    }
    Display* display = XOpenDisplay(nullptr);
    if (!display) { std::cerr << "Games plugin cannot open the X11/XWayland display.\n"; return 6; }
    std::signal(SIGTERM, handle_signal); std::signal(SIGINT, handle_signal);
    GamesApp app(display, static_cast<Window>(parent), env_dimension("NOUGAT_PLUGIN_WIDTH", 900), env_dimension("NOUGAT_PLUGIN_HEIGHT", 600),
                 plugin_root, config_dir, cache_dir, state_dir, data_dir);
    std::string error;
    if (!app.initialize(error)) { std::cerr << error << '\n'; XCloseDisplay(display); return 7; }
    const int result = app.run();
    XCloseDisplay(display);
    return result;
}
