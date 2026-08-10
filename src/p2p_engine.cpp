#include "p2p_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace {
[[maybe_unused]] std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

[[maybe_unused]] bool is_video_path(const std::string& path) {
    const std::string lower = lower_copy(path);
    const char* extensions[] = {
        ".mp4", ".mkv", ".webm", ".mov", ".avi", ".m4v", ".ts", ".m2ts",
        ".mpg", ".mpeg", ".ogv", ".flv", ".wmv", nullptr
    };
    for (int i = 0; extensions[i]; ++i) {
        const std::string ext(extensions[i]);
        if (lower.size() >= ext.size() && lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] fs::path config_dir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return fs::path(xdg) / "reddmedia" / "p2p";
    const char* home = std::getenv("HOME");
    return fs::path(home && *home ? home : ".") / ".config" / "reddmedia" / "p2p";
}

[[maybe_unused]] fs::path resume_file() { return config_dir() / "active.fastresume"; }
[[maybe_unused]] fs::path selected_file_state() { return config_dir() / "selected_file.txt"; }

[[maybe_unused]] void ensure_parent(const fs::path& path) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
}
}

#ifdef REDDMEDIA_P2P_STUB

struct P2PEngine::Impl {
    mutable std::mutex mutex;
    int selected = -1;
    std::string error;
    bool paused = false;
};

P2PEngine::P2PEngine() : impl_(std::make_unique<Impl>()) {}
P2PEngine::~P2PEngine() = default;

bool P2PEngine::start_magnet(const std::string&, const std::string&, std::string& error) {
    error = "P2P stub build does not start transfers.";
    return false;
}
bool P2PEngine::start_torrent_file(const std::string&, const std::string&, std::string& error) {
    error = "P2P stub build does not start transfers.";
    return false;
}
bool P2PEngine::restore_last(std::string&) { return false; }
bool P2PEngine::pause_transfer(std::string& error) { std::lock_guard<std::mutex> lock(impl_->mutex); if (impl_->paused) { error="P2P download is already stopped."; return false; } impl_->paused=true; return true; }
bool P2PEngine::resume_transfer(std::string& error) { std::lock_guard<std::mutex> lock(impl_->mutex); if (!impl_->paused) { error="P2P download is already running."; return false; } impl_->paused=false; return true; }
bool P2PEngine::is_paused() const { std::lock_guard<std::mutex> lock(impl_->mutex); return impl_->paused; }
void P2PEngine::shutdown() {}
P2PStatus P2PEngine::status() const { return {}; }
std::vector<P2PFileInfo> P2PEngine::files() const { return {}; }
bool P2PEngine::select_file(int, std::string& error) { error = "No P2P metadata."; return false; }
int P2PEngine::selected_file() const { return -1; }
std::uint64_t P2PEngine::selected_file_size() const { return 0; }
std::string P2PEngine::selected_file_name() const { return {}; }
void P2PEngine::prioritize_range(std::uint64_t, std::uint64_t) {}
bool P2PEngine::wait_for_range(std::uint64_t, std::uint64_t, int) { return false; }
bool P2PEngine::read_selected_range(std::uint64_t, char*, std::size_t, std::size_t& bytes_read, std::string& error) const {
    bytes_read = 0; error = "P2P stub build has no transfer data."; return false;
}
void P2PEngine::clear_stream_priority() {}
std::string P2PEngine::libtorrent_version() const { return "libtorrent stub"; }

#else

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/version.hpp>
#include <libtorrent/write_resume_data.hpp>

namespace lt = libtorrent;

struct P2PEngine::Impl {
    mutable std::mutex mutex;
    lt::session session;
    lt::torrent_handle handle;
    int selected = -1;
    std::string last_error;
    bool stopped = false;

    Impl() {
        lt::settings_pack settings;
        settings.set_bool(lt::settings_pack::enable_dht, true);
        settings.set_bool(lt::settings_pack::enable_lsd, true);
        settings.set_int(lt::settings_pack::connections_limit, 500);
        session.apply_settings(settings);
    }

    bool valid() const { return handle.is_valid(); }

    void save_resume() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!valid()) return;
        try {
            ensure_parent(resume_file());
            lt::add_torrent_params params = handle.get_resume_data(lt::torrent_handle::save_info_dict);
            std::vector<char> encoded = lt::write_resume_data_buf(params);
            std::ofstream out(resume_file(), std::ios::binary | std::ios::trunc);
            if (out) out.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
            std::ofstream selected_out(selected_file_state(), std::ios::trunc);
            if (selected_out) selected_out << selected << "\n";
        } catch (const std::exception& e) {
            last_error = std::string("Could not save P2P resume data: ") + e.what();
        }
    }

    void stop_current_for_replace() {
        if (!valid()) return;
        save_resume();
        std::lock_guard<std::mutex> lock(mutex);
        if (valid()) session.remove_torrent(handle);
        handle = lt::torrent_handle();
        selected = -1;
    }
};

P2PEngine::P2PEngine() : impl_(std::make_unique<Impl>()) {}
P2PEngine::~P2PEngine() { shutdown(); }

bool P2PEngine::start_magnet(const std::string& uri, const std::string& save_path, std::string& error) {
    if (uri.empty()) { error = "Enter a magnet link first."; return false; }
    if (save_path.empty()) { error = "Choose a download folder first."; return false; }
    impl_->stop_current_for_replace();
    lt::error_code ec;
    lt::add_torrent_params params = lt::parse_magnet_uri(uri, ec);
    if (ec) { error = "Magnet link error: " + ec.message(); return false; }
    params.save_path = save_path;
    lt::torrent_handle handle = impl_->session.add_torrent(std::move(params), ec);
    if (ec || !handle.is_valid()) { error = "Could not add magnet: " + ec.message(); return false; }
    handle.unset_flags(lt::torrent_flags::auto_managed);
    handle.resume();
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->handle = handle;
        impl_->selected = -1;
        impl_->last_error.clear();
        impl_->stopped = false;
    }
    return true;
}

bool P2PEngine::start_torrent_file(const std::string& torrent_path, const std::string& save_path, std::string& error) {
    if (torrent_path.empty()) { error = "Choose a .torrent file first."; return false; }
    if (save_path.empty()) { error = "Choose a download folder first."; return false; }
    impl_->stop_current_for_replace();
    try {
        lt::add_torrent_params params = lt::load_torrent_file(torrent_path);
        params.save_path = save_path;
        lt::error_code ec;
        lt::torrent_handle handle = impl_->session.add_torrent(std::move(params), ec);
        if (ec || !handle.is_valid()) { error = "Could not add P2P metadata file: " + ec.message(); return false; }
        handle.unset_flags(lt::torrent_flags::auto_managed);
        handle.resume();
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->handle = handle;
            impl_->selected = -1;
            impl_->last_error.clear();
            impl_->stopped = false;
        }
        return true;
    } catch (const std::exception& e) {
        error = std::string("P2P metadata file error: ") + e.what();
        return false;
    }
}

bool P2PEngine::restore_last(std::string& error) {
    std::ifstream in(resume_file(), std::ios::binary);
    if (!in) return false;
    std::vector<char> buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (buffer.empty()) return false;
    lt::error_code ec;
    lt::add_torrent_params params = lt::read_resume_data(buffer, ec);
    if (ec) { error = "P2P resume data error: " + ec.message(); return false; }
    if (params.save_path.empty()) { error = "P2P resume data has no save path."; return false; }
    lt::torrent_handle handle = impl_->session.add_torrent(std::move(params), ec);
    if (ec || !handle.is_valid()) { error = "Could not restore P2P download: " + ec.message(); return false; }
    handle.unset_flags(lt::torrent_flags::auto_managed);
    int selected = -1;
    std::ifstream selected_in(selected_file_state());
    if (selected_in) selected_in >> selected;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->handle = handle;
        impl_->selected = selected;
        impl_->last_error.clear();
        impl_->stopped = false;
    }
    return true;
}

bool P2PEngine::pause_transfer(std::string& error) {
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->valid()) { error="No active P2P download."; return false; }
        try {
            impl_->handle.unset_flags(lt::torrent_flags::auto_managed);
            impl_->handle.clear_piece_deadlines();
            impl_->handle.pause();
        } catch (const std::exception& e) { error=e.what(); return false; }
    }
    impl_->save_resume();
    return true;
}

bool P2PEngine::resume_transfer(std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error="No active P2P download."; return false; }
    try {
        impl_->handle.unset_flags(lt::torrent_flags::auto_managed);
        impl_->handle.resume();
        return true;
    } catch (const std::exception& e) { error=e.what(); return false; }
}

bool P2PEngine::is_paused() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return false;
    try { return bool(impl_->handle.flags() & lt::torrent_flags::paused); } catch (const std::exception&) { return false; }
}

void P2PEngine::shutdown() {
    if (!impl_ || impl_->stopped) return;
    impl_->save_resume();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stopped = true;
}

P2PStatus P2PEngine::status() const {
    P2PStatus out;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { out.error = impl_->last_error; return out; }
    try {
        lt::torrent_status st = impl_->handle.status();
        out.active = true;
        out.metadata_ready = st.has_metadata;
        out.seeding = st.is_seeding;
        out.paused = bool(st.flags & lt::torrent_flags::paused);
        out.progress = st.progress;
        out.downloaded = st.total_done;
        out.total = st.total;
        out.download_rate = st.download_payload_rate;
        out.upload_rate = st.upload_payload_rate;
        out.peers = st.num_peers;
        out.seeds = st.num_seeds;
        out.name = st.name;
        out.save_path = st.save_path;
        if (st.errc) out.error = st.errc.message();
        else out.error = impl_->last_error;
        if (out.paused) out.state = "Stopped";
        else switch (st.state) {
            case lt::torrent_status::checking_files: out.state = "Checking files"; break;
            case lt::torrent_status::downloading_metadata: out.state = "Getting metadata"; break;
            case lt::torrent_status::downloading: out.state = "Downloading"; break;
            case lt::torrent_status::finished: out.state = "Finishing"; break;
            case lt::torrent_status::seeding: out.state = "Seeding"; break;
            case lt::torrent_status::checking_resume_data: out.state = "Checking resume data"; break;
            default: out.state = "Starting"; break;
        }
    } catch (const std::exception& e) {
        out.error = e.what();
    }
    return out;
}

std::vector<P2PFileInfo> P2PEngine::files() const {
    std::vector<P2PFileInfo> out;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return out;
    try {
        std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
        if (!info) return out;
        const lt::file_storage& storage = info->files();
        const int count = storage.num_files();
        out.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            P2PFileInfo item;
            item.index = i;
            item.path = storage.file_path(lt::file_index_t(i));
            item.size = static_cast<std::uint64_t>(storage.file_size(lt::file_index_t(i)));
            item.video = is_video_path(item.path);
            out.push_back(item);
        }
    } catch (const std::exception&) {}
    return out;
}

bool P2PEngine::select_file(int index, std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
    if (!info) { error = "P2P metadata is still loading."; return false; }
    if (index < 0 || index >= info->files().num_files()) { error = "Invalid P2P file selection."; return false; }
    impl_->selected = index;
    return true;
}

int P2PEngine::selected_file() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->selected;
}

std::uint64_t P2PEngine::selected_file_size() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid() || impl_->selected < 0) return 0;
    std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
    if (!info || impl_->selected >= info->files().num_files()) return 0;
    return static_cast<std::uint64_t>(info->files().file_size(lt::file_index_t(impl_->selected)));
}

std::string P2PEngine::selected_file_name() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid() || impl_->selected < 0) return {};
    std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
    if (!info || impl_->selected >= info->files().num_files()) return {};
    return info->files().file_path(lt::file_index_t(impl_->selected));
}

void P2PEngine::prioritize_range(std::uint64_t offset, std::uint64_t length) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid() || impl_->selected < 0 || length == 0) return;
    try {
        std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
        if (!info) return;
        const lt::file_storage& storage = info->files();
        const lt::file_index_t file_index(impl_->selected);
        const std::uint64_t file_size = static_cast<std::uint64_t>(storage.file_size(file_index));
        if (offset >= file_size) return;
        const std::uint64_t bounded_length = std::min(length, file_size - offset);
        const std::uint64_t absolute_start = static_cast<std::uint64_t>(storage.file_offset(file_index)) + offset;
        const std::uint64_t absolute_end = absolute_start + bounded_length - 1;
        const std::uint64_t piece_length = static_cast<std::uint64_t>(storage.piece_length());
        const int first = static_cast<int>(absolute_start / piece_length);
        const int last = static_cast<int>(absolute_end / piece_length);
        int deadline = 0;
        for (int piece = first; piece <= last; ++piece) {
            impl_->handle.set_piece_deadline(lt::piece_index_t(piece), deadline);
            deadline += 80;
        }
    } catch (const std::exception& e) {
        impl_->last_error = e.what();
    }
}

bool P2PEngine::wait_for_range(std::uint64_t offset, std::uint64_t length, int timeout_ms) {
    if (length == 0) return true;
    const auto deadline_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, timeout_ms));
    while (true) {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            if (impl_->stopped || !impl_->valid() || impl_->selected < 0) return false;
            try {
                std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
                if (!info) return false;
                const lt::file_storage& storage = info->files();
                const lt::file_index_t file_index(impl_->selected);
                const std::uint64_t file_size = static_cast<std::uint64_t>(storage.file_size(file_index));
                if (offset >= file_size) return false;
                const std::uint64_t bounded_length = std::min(length, file_size - offset);
                const std::uint64_t absolute_start = static_cast<std::uint64_t>(storage.file_offset(file_index)) + offset;
                const std::uint64_t absolute_end = absolute_start + bounded_length - 1;
                const std::uint64_t piece_length = static_cast<std::uint64_t>(storage.piece_length());
                const int first = static_cast<int>(absolute_start / piece_length);
                const int last = static_cast<int>(absolute_end / piece_length);
                bool ready = true;
                for (int piece = first; piece <= last; ++piece) {
                    if (!impl_->handle.have_piece(lt::piece_index_t(piece))) { ready = false; break; }
                }
                if (ready) return true;
                lt::torrent_status st = impl_->handle.status();
                if (st.errc) { impl_->last_error = st.errc.message(); return false; }
            } catch (const std::exception& e) {
                impl_->last_error = e.what();
                return false;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline_time) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool P2PEngine::read_selected_range(std::uint64_t offset, char* destination, std::size_t length,
                                    std::size_t& bytes_read, std::string& error) const {
    bytes_read = 0;
    std::string path;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->valid() || impl_->selected < 0) { error = "No video selected."; return false; }
        try {
            std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
            if (!info) { error = "P2P metadata is not ready."; return false; }
            const lt::file_storage& storage = info->files();
            const lt::file_index_t file_index(impl_->selected);
            const std::uint64_t file_size = static_cast<std::uint64_t>(storage.file_size(file_index));
            if (offset >= file_size) { error = "Requested range is outside the selected file."; return false; }
            path = storage.file_path(file_index, impl_->handle.status().save_path);
        } catch (const std::exception& e) {
            error = e.what();
            return false;
        }
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) { error = "Could not open downloaded P2P data: " + path; return false; }
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!input) { error = "Could not seek downloaded P2P data."; return false; }
    input.read(destination, static_cast<std::streamsize>(length));
    bytes_read = static_cast<std::size_t>(input.gcount());
    if (bytes_read == 0 && length != 0) { error = "P2P data was not readable yet."; return false; }
    return true;
}

void P2PEngine::clear_stream_priority() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return;
    try { impl_->handle.clear_piece_deadlines(); } catch (const std::exception&) {}
}

std::string P2PEngine::libtorrent_version() const {
    return std::string("libtorrent ") + LIBTORRENT_VERSION;
}

#endif
