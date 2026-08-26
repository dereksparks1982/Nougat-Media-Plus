#include <X11/Xlib.h>

#include "workshop/split_archive_service.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

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

long long json_number(const std::string& text, const std::string& key, long long fallback = 0) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || match.size() < 2) return fallback;
    try {
        return std::stoll(match[1].str());
    } catch (...) {
        return fallback;
    }
}

std::string json_string(const std::string& text, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    return std::regex_search(text, match, pattern) && match.size() >= 2 ? match[1].str() : std::string();
}

std::string trim_newlines(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
    return value;
}

std::string choose_with_zenity(bool directory, const char* title) {
    int output_pipe[2] = {-1, -1};
    if (::pipe(output_pipe) != 0) return {};
    const pid_t child = ::fork();
    if (child < 0) {
        ::close(output_pipe[0]);
        ::close(output_pipe[1]);
        return {};
    }
    if (child == 0) {
        (void)::dup2(output_pipe[1], STDOUT_FILENO);
        ::close(output_pipe[0]);
        ::close(output_pipe[1]);
        if (directory) {
            ::execlp("zenity", "zenity", "--file-selection", "--directory", "--title", title,
                     static_cast<char*>(nullptr));
        } else {
            ::execlp("zenity", "zenity", "--file-selection", "--title", title,
                     static_cast<char*>(nullptr));
        }
        _exit(127);
    }

    ::close(output_pipe[1]);
    std::string output;
    char buffer[2048]{};
    for (;;) {
        const ssize_t count = ::read(output_pipe[0], buffer, sizeof(buffer));
        if (count > 0) {
            output.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        break;
    }
    ::close(output_pipe[0]);
    int status = 0;
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return {};
    return trim_newlines(output);
}

unsigned long color(Display* display, Colormap cmap, const char* name, unsigned long fallback) {
    XColor exact{};
    XColor screen{};
    if (XAllocNamedColor(display, cmap, name, &screen, &exact) != 0) return screen.pixel;
    return fallback;
}

class WorkshopApp {
public:
    WorkshopApp(Display* display, Window parent, int width, int height, fs::path worker_script)
        : display_(display), parent_(parent), width_(width), height_(height),
          service_(std::move(worker_script)) {}

    ~WorkshopApp() {
        g_running.store(false);
        if (worker_.joinable()) worker_.join();
        if (gc_ != 0) XFreeGC(display_, gc_);
        if (window_ != 0) XDestroyWindow(display_, window_);
    }

    bool initialize(std::string& error) {
        if (!service_.available()) {
            error = "Workshop worker is missing: " + service_.worker_script().string();
            return false;
        }
        const int screen = DefaultScreen(display_);
        const Colormap cmap = DefaultColormap(display_, screen);
        cream_ = color(display_, cmap, "#f2dfb9", WhitePixel(display_, screen));
        pale_ = color(display_, cmap, "#fff4dd", WhitePixel(display_, screen));
        caramel_ = color(display_, cmap, "#c9944f", WhitePixel(display_, screen));
        chocolate_ = color(display_, cmap, "#5c3520", BlackPixel(display_, screen));
        muted_ = color(display_, cmap, "#785f4b", BlackPixel(display_, screen));
        green_ = color(display_, cmap, "#4f713d", BlackPixel(display_, screen));
        red_ = color(display_, cmap, "#9a4537", BlackPixel(display_, screen));

        window_ = XCreateSimpleWindow(display_, parent_, 0, 0,
                                      static_cast<unsigned int>(std::max(1, width_)),
                                      static_cast<unsigned int>(std::max(1, height_)),
                                      0, chocolate_, cream_);
        if (window_ == 0) {
            error = "Workshop could not create its Nougat plugin child window";
            return false;
        }
        XSelectInput(display_, window_, ExposureMask | ButtonPressMask | StructureNotifyMask);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XMapWindow(display_, window_);
        XFlush(display_);
        return true;
    }

    int run() {
        while (g_running.load()) {
            resize_to_parent();
            poll_worker();

            while (XPending(display_) > 0) {
                XEvent event{};
                XNextEvent(display_, &event);
                if (event.xany.window != window_) continue;
                if (event.type == Expose && event.xexpose.count == 0) draw();
                else if (event.type == ConfigureNotify) {
                    width_ = std::max(1, event.xconfigure.width);
                    height_ = std::max(1, event.xconfigure.height);
                    draw();
                } else if (event.type == ButtonPress && event.xbutton.button == Button1) {
                    handle_click(event.xbutton.x, event.xbutton.y);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
        return 0;
    }

private:
    Display* display_ = nullptr;
    Window parent_ = 0;
    Window window_ = 0;
    GC gc_ = 0;
    int width_ = 1;
    int height_ = 1;
    unsigned long cream_ = 0;
    unsigned long pale_ = 0;
    unsigned long caramel_ = 0;
    unsigned long chocolate_ = 0;
    unsigned long muted_ = 0;
    unsigned long green_ = 0;
    unsigned long red_ = 0;

    nougat::workshop::SplitArchiveService service_;
    std::thread worker_;
    mutable std::mutex state_mutex_;
    bool busy_ = false;
    bool updated_ = false;
    bool source_is_folder_ = false;
    bool by_part_count_ = false;
    unsigned part_count_ = 2;
    unsigned long long max_part_mib_ = 450;
    std::string source_;
    std::string output_;
    std::string status_ = "Choose a source file or folder to inspect.";
    long long total_bytes_ = 0;
    long long file_count_ = 0;
    long long directory_count_ = 0;
    long long largest_file_bytes_ = 0;

    Rect file_button_{};
    Rect folder_button_{};
    Rect output_button_{};
    Rect mode_button_{};
    Rect minus_button_{};
    Rect plus_button_{};
    Rect split_button_{};
    Rect reassemble_button_{};

    void resize_to_parent() {
        XWindowAttributes attributes{};
        if (XGetWindowAttributes(display_, parent_, &attributes) == 0) return;
        const int new_width = std::max(1, attributes.width);
        const int new_height = std::max(1, attributes.height);
        if (new_width == width_ && new_height == height_) return;
        width_ = new_width;
        height_ = new_height;
        XMoveResizeWindow(display_, window_, 0, 0,
                          static_cast<unsigned int>(width_),
                          static_cast<unsigned int>(height_));
        draw();
    }

    void fill(Rect rect, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XFillRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w)),
                       static_cast<unsigned int>(std::max(1, rect.h)));
    }

    void outline(Rect rect, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XDrawRectangle(display_, window_, gc_, rect.x, rect.y,
                       static_cast<unsigned int>(std::max(1, rect.w - 1)),
                       static_cast<unsigned int>(std::max(1, rect.h - 1)));
    }

    void text(int x, int y, const std::string& value, unsigned long pixel) {
        XSetForeground(display_, gc_, pixel);
        XDrawString(display_, window_, gc_, x, y, value.c_str(), static_cast<int>(value.size()));
    }

    static std::string clipped(const std::string& value, std::size_t maximum) {
        if (value.size() <= maximum) return value;
        if (maximum <= 3) return value.substr(0, maximum);
        return value.substr(0, maximum - 3) + "...";
    }

    void button(Rect rect, const std::string& label, bool enabled = true) {
        fill(rect, enabled ? caramel_ : cream_);
        outline(rect, chocolate_);
        const int baseline = rect.y + rect.h / 2 + 5;
        text(rect.x + 9, baseline, clipped(label, static_cast<std::size_t>(std::max(1, rect.w / 8))),
             enabled ? chocolate_ : muted_);
    }

    void layout() {
        const int left = 24;
        const int usable = std::max(260, width_ - 48);
        const int gap = 10;
        const int half = std::max(100, (usable - gap) / 2);
        file_button_ = {left, 82, half, 38};
        folder_button_ = {left + half + gap, 82, usable - half - gap, 38};
        output_button_ = {left, 132, usable, 38};
        mode_button_ = {left, 182, std::max(150, usable / 3), 38};
        minus_button_ = {mode_button_.x + mode_button_.w + gap, 182, 42, 38};
        plus_button_ = {left + usable - 42, 182, 42, 38};
        split_button_ = {left, 232, half, 42};
        reassemble_button_ = {left + half + gap, 232, usable - half - gap, 42};
    }

    void draw() {
        layout();
        fill({0, 0, width_, height_}, cream_);
        fill({12, 12, std::max(1, width_ - 24), std::max(1, height_ - 24)}, pale_);
        outline({12, 12, std::max(1, width_ - 24), std::max(1, height_ - 24)}, chocolate_);
        text(24, 38, "WORKSHOP", chocolate_);
        text(24, 60, "File engineering and production workspace", muted_);

        bool busy = false;
        std::string status;
        std::string source;
        std::string output;
        long long total = 0;
        long long files = 0;
        long long directories = 0;
        long long largest = 0;
        bool by_count = false;
        unsigned count = 0;
        unsigned long long max_mib = 0;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            busy = busy_;
            status = status_;
            source = source_;
            output = output_;
            total = total_bytes_;
            files = file_count_;
            directories = directory_count_;
            largest = largest_file_bytes_;
            by_count = by_part_count_;
            count = part_count_;
            max_mib = max_part_mib_;
        }

        button(file_button_, source_is_folder_ ? "Choose File" : "Choose File", !busy);
        button(folder_button_, "Choose Folder", !busy);
        button(output_button_, output.empty() ? "Choose Output Folder" : "Output: " + output, !busy);
        button(mode_button_, by_count ? "Mode: Part Count" : "Mode: Max Size", !busy);
        button(minus_button_, "-", !busy);
        button(plus_button_, "+", !busy);
        const std::string setting = by_count ? std::to_string(count) + " parts"
                                             : std::to_string(max_mib) + " MiB max";
        text(minus_button_.x + minus_button_.w + 12, 207, setting, chocolate_);
        button(split_button_, busy ? "Working..." : "Split + Verify", !busy);
        button(reassemble_button_, "Reassemble", !busy);

        int y = 304;
        text(24, y, "Source: " + clipped(source.empty() ? "none" : source, 90), source.empty() ? muted_ : chocolate_);
        y += 24;
        text(24, y, "Output: " + clipped(output.empty() ? "none" : output, 90), output.empty() ? muted_ : chocolate_);
        y += 30;
        std::ostringstream metrics;
        metrics << "Files " << files << " | Folders " << directories << " | Total " << total
                << " bytes | Largest " << largest << " bytes";
        text(24, y, clipped(metrics.str(), 105), muted_);
        y += 32;
        text(24, y, clipped(status, 105), busy ? chocolate_ : (status.find("failed") != std::string::npos ? red_ : green_));
        y += 28;
        text(24, y, "Format: NOUGAT_SPLIT_ARCHIVE v1 | SHA-256 verified parts and reconstruction", muted_);
        XFlush(display_);
    }

    void set_status(std::string status) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        status_ = std::move(status);
        updated_ = true;
    }

    void begin_operation() {
        std::lock_guard<std::mutex> lock(state_mutex_);
        busy_ = true;
        updated_ = true;
    }

    void finish_operation(std::string status) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        busy_ = false;
        status_ = std::move(status);
        updated_ = true;
    }

    bool busy() const {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return busy_;
    }

    void poll_worker() {
        bool redraw_needed = false;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            redraw_needed = updated_;
            updated_ = false;
        }
        if (redraw_needed) draw();
        if (!busy() && worker_.joinable()) worker_.join();
    }

    void inspect_async(std::string source, bool is_folder) {
        if (busy()) return;
        if (worker_.joinable()) worker_.join();
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            source_ = source;
            source_is_folder_ = is_folder;
            busy_ = true;
            status_ = "Inspecting source...";
            updated_ = true;
        }
        worker_ = std::thread([this, source = std::move(source)]() {
            const auto result = service_.inspect(source);
            if (!result.ok()) {
                finish_operation("Inspect failed: " + trim_newlines(result.standard_error));
                return;
            }
            const long long total = json_number(result.standard_output, "total_bytes");
            const long long files = json_number(result.standard_output, "file_count");
            const long long directories = json_number(result.standard_output, "directory_count");
            const long long largest = json_number(result.standard_output, "largest_file_bytes");
            const long long suggested = std::max<long long>(1, json_number(result.standard_output, "suggested_parts", 1));
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                total_bytes_ = total;
                file_count_ = files;
                directory_count_ = directories;
                largest_file_bytes_ = largest;
                part_count_ = static_cast<unsigned>(std::min<long long>(9999, suggested));
                busy_ = false;
                status_ = "Inspection complete. Choose output and split settings.";
                updated_ = true;
            }
        });
    }

    void split_async() {
        if (busy()) return;
        std::string source;
        std::string output;
        bool by_count = false;
        unsigned count = 0;
        unsigned long long max_mib = 0;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            source = source_;
            output = output_;
            by_count = by_part_count_;
            count = part_count_;
            max_mib = max_part_mib_;
        }
        if (source.empty()) {
            set_status("Choose a source file or folder first.");
            return;
        }
        if (output.empty()) {
            set_status("Choose an output folder first.");
            return;
        }
        if (worker_.joinable()) worker_.join();
        begin_operation();
        set_status("Splitting, hashing, and verifying parts...");
        worker_ = std::thread([this, source, output, by_count, count, max_mib]() {
            const auto result = by_count
                ? service_.split_by_part_count(source, output, std::max(1U, count))
                : service_.split_by_max_size(source, output,
                    std::max<unsigned long long>(25ULL, max_mib) * 1024ULL * 1024ULL);
            if (!result.ok()) {
                finish_operation("Split failed: " + trim_newlines(result.standard_error));
                return;
            }
            const std::string manifest = json_string(result.standard_output, "manifest");
            finish_operation(manifest.empty() ? "Split + verification complete."
                                              : "Split + verification complete: " + manifest);
        });
    }

    void reassemble_async(std::string selected) {
        if (busy()) return;
        std::string output;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            output = output_;
        }
        if (output.empty()) {
            set_status("Choose an output folder before reassembling.");
            return;
        }
        if (worker_.joinable()) worker_.join();
        begin_operation();
        set_status("Verifying parts and reassembling...");
        worker_ = std::thread([this, selected = std::move(selected), output]() {
            const auto result = service_.reassemble(selected, output);
            if (!result.ok()) {
                finish_operation("Reassemble failed: " + trim_newlines(result.standard_error));
                return;
            }
            const std::string restored = json_string(result.standard_output, "output");
            finish_operation(restored.empty() ? "Reassembly and verification complete."
                                              : "Reassembly complete: " + restored);
        });
    }

    void handle_click(int x, int y) {
        if (busy()) return;
        if (file_button_.contains(x, y)) {
            const std::string selected = choose_with_zenity(false, "Choose Workshop source file");
            if (!selected.empty()) inspect_async(selected, false);
            return;
        }
        if (folder_button_.contains(x, y)) {
            const std::string selected = choose_with_zenity(true, "Choose Workshop source folder");
            if (!selected.empty()) inspect_async(selected, true);
            return;
        }
        if (output_button_.contains(x, y)) {
            const std::string selected = choose_with_zenity(true, "Choose Workshop output folder");
            if (!selected.empty()) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                output_ = selected;
                status_ = "Output folder selected.";
                updated_ = true;
            }
            return;
        }
        if (mode_button_.contains(x, y)) {
            std::lock_guard<std::mutex> lock(state_mutex_);
            by_part_count_ = !by_part_count_;
            status_ = by_part_count_ ? "Split mode: exact part count." : "Split mode: maximum part size.";
            updated_ = true;
            return;
        }
        if (minus_button_.contains(x, y)) {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (by_part_count_) part_count_ = std::max(1U, part_count_ - 1U);
            else max_part_mib_ = std::max<unsigned long long>(25ULL, max_part_mib_ >= 25ULL ? max_part_mib_ - 25ULL : 25ULL);
            updated_ = true;
            return;
        }
        if (plus_button_.contains(x, y)) {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (by_part_count_) part_count_ = std::min(9999U, part_count_ + 1U);
            else max_part_mib_ = std::min<unsigned long long>(102400ULL, max_part_mib_ + 25ULL);
            updated_ = true;
            return;
        }
        if (split_button_.contains(x, y)) {
            split_async();
            return;
        }
        if (reassemble_button_.contains(x, y)) {
            const std::string selected = choose_with_zenity(false, "Choose Nougat split manifest or part");
            if (!selected.empty()) reassemble_async(selected);
        }
    }
};

bool parse_unsigned_long(const char* text, unsigned long& value) {
    if (text == nullptr || *text == '\0') return false;
    try {
        std::size_t used = 0;
        const std::string input(text);
        value = std::stoul(input, &used, 10);
        return used == input.size() && value != 0UL;
    } catch (...) {
        return false;
    }
}

int env_dimension(const char* name, int fallback) {
    const char* text = std::getenv(name);
    if (text == nullptr || *text == '\0') return fallback;
    try {
        const int value = std::stoi(text);
        return std::max(1, value);
    } catch (...) {
        return fallback;
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--version") {
        std::cout << "Nougat Workshop Plugin v0.0.50\n";
        return 0;
    }
    if (argc <= 1 || std::string(argv[1]) != "--nougat-plugin") {
        std::cerr << "Workshop is a Nougat plugin. Launch it through Nougat Media Suite.\n";
        return 2;
    }
    if (std::getenv("NOUGAT_PLUGIN_API") == nullptr || std::string(std::getenv("NOUGAT_PLUGIN_API")) != "1" ||
        std::getenv("NOUGAT_PLUGIN_ID") == nullptr || std::string(std::getenv("NOUGAT_PLUGIN_ID")) != "workshop") {
        std::cerr << "Workshop plugin host contract is missing or incompatible.\n";
        return 3;
    }

    unsigned long parent = 0;
    if (!parse_unsigned_long(std::getenv("NOUGAT_PLUGIN_PARENT_XID"), parent)) {
        std::cerr << "Workshop plugin parent X11 surface is missing.\n";
        return 4;
    }
    const char* root_text = std::getenv("NOUGAT_PLUGIN_ROOT");
    if (root_text == nullptr || *root_text == '\0') {
        std::cerr << "Workshop plugin root is missing.\n";
        return 5;
    }
    const fs::path worker_script = fs::path(root_text) / "nougat_split_archive.py";

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "Workshop plugin cannot open the X11/XWayland display.\n";
        return 6;
    }

    std::signal(SIGTERM, handle_signal);
    std::signal(SIGINT, handle_signal);
    const int width = env_dimension("NOUGAT_PLUGIN_WIDTH", 760);
    const int height = env_dimension("NOUGAT_PLUGIN_HEIGHT", 520);
    WorkshopApp app(display, static_cast<Window>(parent), width, height, worker_script);
    std::string error;
    if (!app.initialize(error)) {
        std::cerr << error << '\n';
        XCloseDisplay(display);
        return 7;
    }
    const int result = app.run();
    XCloseDisplay(display);
    return result;
}
