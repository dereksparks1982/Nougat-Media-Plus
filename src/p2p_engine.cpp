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
[[maybe_unused]] fs::path plus_settings_state() { return config_dir() / "plus_settings.txt"; }

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
    P2PPlusSettings plus;
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
bool P2PEngine::remove_transfer(std::string& error) { std::lock_guard<std::mutex> lock(impl_->mutex); if (impl_->selected < 0 && !impl_->paused) { error="No active P2P transfer."; return false; } impl_->selected=-1; impl_->paused=false; return true; }
bool P2PEngine::is_paused() const { std::lock_guard<std::mutex> lock(impl_->mutex); return impl_->paused; }
bool P2PEngine::set_speed_limits(int download_kib, int upload_kib, std::string&) { std::lock_guard<std::mutex> lock(impl_->mutex); impl_->plus.download_limit_kib=std::max(0,download_kib); impl_->plus.upload_limit_kib=std::max(0,upload_kib); return true; }
bool P2PEngine::set_seed_rules(double ratio_limit, int time_limit_minutes, std::string&) { std::lock_guard<std::mutex> lock(impl_->mutex); impl_->plus.seed_ratio_limit=std::max(0.0,ratio_limit); impl_->plus.seed_time_limit_minutes=std::max(0,time_limit_minutes); return true; }
P2PPlusSettings P2PEngine::plus_settings() const { std::lock_guard<std::mutex> lock(impl_->mutex); return impl_->plus; }
bool P2PEngine::queue_up(std::string& error) { error="P2P stub has no queue."; return false; }
bool P2PEngine::queue_down(std::string& error) { error="P2P stub has no queue."; return false; }
bool P2PEngine::force_reannounce(std::string& error) { error="P2P stub has no trackers."; return false; }
bool P2PEngine::force_recheck(std::string& error) { error="P2P stub has no transfer."; return false; }
bool P2PEngine::set_file_priority(int, int, std::string& error) { error="P2P stub has no metadata."; return false; }
std::vector<P2PTrackerInfo> P2PEngine::trackers() const { return {}; }
void P2PEngine::enforce_seed_rules() {}
void P2PEngine::shutdown() {}
P2PStatus P2PEngine::status() const { return {}; }
std::vector<P2PFileInfo> P2PEngine::files() const { return {}; }
bool P2PEngine::select_file(int, std::string& error) { error = "No P2P metadata."; return false; }
int P2PEngine::selected_file() const { return -1; }
std::uint64_t P2PEngine::selected_file_size() const { return 0; }
std::string P2PEngine::selected_file_name() const { return {}; }
void P2PEngine::prioritize_range(std::uint64_t, std::uint64_t) {}
void P2PEngine::prioritize_playback_window(std::uint64_t) {}
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
    P2PPlusSettings plus;

    Impl() {
        lt::settings_pack settings;
        settings.set_bool(lt::settings_pack::enable_dht, true);
        settings.set_bool(lt::settings_pack::enable_lsd, true);
        settings.set_int(lt::settings_pack::connections_limit, 500);
        session.apply_settings(settings);
        std::ifstream in(plus_settings_state());
        if (in) in >> plus.download_limit_kib >> plus.upload_limit_kib >> plus.seed_ratio_limit >> plus.seed_time_limit_minutes;
    }

    void persist_plus() const {
        ensure_parent(plus_settings_state());
        std::ofstream out(plus_settings_state(), std::ios::trunc);
        if (out) out << plus.download_limit_kib << " " << plus.upload_limit_kib << " "
                     << plus.seed_ratio_limit << " " << plus.seed_time_limit_minutes << "\n";
    }

    void apply_plus_limits() {
        if (!valid()) return;
        handle.set_download_limit(plus.download_limit_kib > 0 ? plus.download_limit_kib * 1024 : 0);
        handle.set_upload_limit(plus.upload_limit_kib > 0 ? plus.upload_limit_kib * 1024 : 0);
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
        impl_->apply_plus_limits();
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
            impl_->apply_plus_limits();
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
        impl_->apply_plus_limits();
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

bool P2PEngine::remove_transfer(std::string& error) {
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
        try {
            impl_->handle.clear_piece_deadlines();
            impl_->session.remove_torrent(impl_->handle);
            impl_->handle = lt::torrent_handle();
            impl_->selected = -1;
            impl_->last_error.clear();
            impl_->stopped = false;
        } catch (const std::exception& e) { error = e.what(); return false; }
    }
    std::error_code ec;
    fs::remove(resume_file(), ec);
    ec.clear();
    fs::remove(selected_file_state(), ec);
    return true;
}

bool P2PEngine::is_paused() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return false;
    try { return bool(impl_->handle.flags() & lt::torrent_flags::paused); } catch (const std::exception&) { return false; }
}

bool P2PEngine::set_speed_limits(int download_kib, int upload_kib, std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->plus.download_limit_kib = std::max(0, download_kib);
    impl_->plus.upload_limit_kib = std::max(0, upload_kib);
    try {
        if (impl_->valid()) impl_->apply_plus_limits();
        impl_->persist_plus();
        return true;
    } catch (const std::exception& e) { error = e.what(); return false; }
}

bool P2PEngine::set_seed_rules(double ratio_limit, int time_limit_minutes, std::string& error) {
    if (ratio_limit < 0.0 || time_limit_minutes < 0) { error = "Seed limits cannot be negative."; return false; }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->plus.seed_ratio_limit = ratio_limit;
    impl_->plus.seed_time_limit_minutes = time_limit_minutes;
    impl_->persist_plus();
    return true;
}

P2PPlusSettings P2PEngine::plus_settings() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->plus;
}

bool P2PEngine::queue_up(std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    try { impl_->handle.queue_position_up(); return true; }
    catch (const std::exception& e) { error=e.what(); return false; }
}

bool P2PEngine::queue_down(std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    try { impl_->handle.queue_position_down(); return true; }
    catch (const std::exception& e) { error=e.what(); return false; }
}

bool P2PEngine::force_reannounce(std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    try { impl_->handle.force_reannounce(); return true; }
    catch (const std::exception& e) { error=e.what(); return false; }
}

bool P2PEngine::force_recheck(std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    try { impl_->handle.force_recheck(); return true; }
    catch (const std::exception& e) { error=e.what(); return false; }
}

bool P2PEngine::set_file_priority(int index, int priority, std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) { error = "No active P2P transfer."; return false; }
    std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
    if (!info || index < 0 || index >= info->files().num_files()) { error = "Invalid P2P file priority selection."; return false; }
    priority = std::max(0, std::min(priority, 7));
    try { impl_->handle.file_priority(lt::file_index_t(index), lt::download_priority_t(priority)); return true; }
    catch (const std::exception& e) { error=e.what(); return false; }
}

std::vector<P2PTrackerInfo> P2PEngine::trackers() const {
    std::vector<P2PTrackerInfo> out;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return out;
    try {
        const auto entries = impl_->handle.trackers();
        out.reserve(entries.size());
        for (const auto& entry : entries) {
            P2PTrackerInfo item;
            item.url = entry.url;
            // libtorrent 2.x stores tracker response text per endpoint/info-hash,
            // not on announce_entry itself. Keep the UI status portable across
            // the 2.0.x API by exposing the stable announce_entry::verified bit.
            item.message = entry.verified ? "verified" : "waiting";
            out.push_back(std::move(item));
        }
    } catch (const std::exception&) {}
    return out;
}

void P2PEngine::enforce_seed_rules() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid()) return;
    if (impl_->plus.seed_ratio_limit <= 0.0 && impl_->plus.seed_time_limit_minutes <= 0) return;
    try {
        const lt::torrent_status st = impl_->handle.status();
        if (!st.is_seeding) return;
        bool ratio_met = false;
        if (impl_->plus.seed_ratio_limit > 0.0 && st.all_time_download > 0) {
            const double ratio = static_cast<double>(st.all_time_upload) / static_cast<double>(st.all_time_download);
            ratio_met = ratio >= impl_->plus.seed_ratio_limit;
        }
        bool time_met = false;
        if (impl_->plus.seed_time_limit_minutes > 0) {
            const auto seeded_minutes = std::chrono::duration_cast<std::chrono::minutes>(st.seeding_duration).count();
            time_met = seeded_minutes >= impl_->plus.seed_time_limit_minutes;
        }
        if (ratio_met || time_met) {
            impl_->handle.unset_flags(lt::torrent_flags::auto_managed);
            impl_->handle.pause();
        }
    } catch (const std::exception&) {}
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
        out.download_limit = impl_->handle.download_limit();
        out.upload_limit = impl_->handle.upload_limit();
        out.total_uploaded = st.total_upload;
        out.total_downloaded = st.total_download;
        out.share_ratio = st.total_download > 0 ? static_cast<double>(st.total_upload) / static_cast<double>(st.total_download) : 0.0;
        out.peers = st.num_peers;
        out.seeds = st.num_seeds;
        out.known_peers = st.list_peers;
        out.known_seeds = st.list_seeds;
        out.tracker_complete = st.num_complete;
        out.tracker_incomplete = st.num_incomplete;
        out.uploading_peers = st.num_uploads;
        out.swarm_availability = st.is_seeding ? -1.0f : st.distributed_copies;
        out.has_incoming = st.has_incoming;
        out.announcing_trackers = st.announcing_to_trackers;
        out.announcing_dht = st.announcing_to_dht;
        out.announcing_lsd = st.announcing_to_lsd;
        out.queue_position = st.queue_position;
        out.tracker_count = static_cast<int>(impl_->handle.trackers().size());
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
        if (impl_->selected >= 0) {
            std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
            if (info && impl_->selected < info->files().num_files()) {
                const lt::file_storage& storage = info->files();
                const lt::file_index_t file_index(impl_->selected);
                const std::uint64_t file_size = static_cast<std::uint64_t>(storage.file_size(file_index));
                out.selected_size = file_size;
                if (file_size > 0) {
                    const std::uint64_t file_start = static_cast<std::uint64_t>(storage.file_offset(file_index));
                    const std::uint64_t file_end = file_start + file_size;
                    const std::uint64_t piece_length = static_cast<std::uint64_t>(storage.piece_length());
                    const int first = static_cast<int>(file_start / piece_length);
                    const int last = static_cast<int>((file_end - 1) / piece_length);
                    std::uint64_t have_bytes = 0;
                    std::uint64_t contiguous = 0;
                    bool gap = false;
                    for (int piece = first; piece <= last; ++piece) {
                        const std::uint64_t piece_start = static_cast<std::uint64_t>(piece) * piece_length;
                        const std::uint64_t piece_end = piece_start + piece_length;
                        const std::uint64_t overlap_start = std::max(file_start, piece_start);
                        const std::uint64_t overlap_end = std::min(file_end, piece_end);
                        const std::uint64_t overlap = overlap_end > overlap_start ? overlap_end - overlap_start : 0;
                        const bool have = impl_->handle.have_piece(lt::piece_index_t(piece));
                        if (have) have_bytes += overlap;
                        if (!gap && have) contiguous += overlap;
                        else if (!have) gap = true;
                    }
                    out.selected_progress = static_cast<float>(static_cast<double>(have_bytes) / static_cast<double>(file_size));
                    out.selected_buffered_bytes = std::min(contiguous, file_size);
                }
            }
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
    ensure_parent(selected_file_state());
    std::ofstream selected_out(selected_file_state(), std::ios::trunc);
    if (selected_out) selected_out << impl_->selected << "\n";
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

void P2PEngine::prioritize_playback_window(std::uint64_t offset) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->valid() || impl_->selected < 0) return;
    try {
        std::shared_ptr<const lt::torrent_info> info = impl_->handle.torrent_file();
        if (!info) return;
        const lt::file_storage& storage = info->files();
        const lt::file_index_t file_index(impl_->selected);
        const std::uint64_t file_size = static_cast<std::uint64_t>(storage.file_size(file_index));
        if (offset >= file_size || file_size == 0) return;
        const std::uint64_t file_start = static_cast<std::uint64_t>(storage.file_offset(file_index));
        const std::uint64_t piece_length = static_cast<std::uint64_t>(storage.piece_length());
        auto schedule = [&](std::uint64_t local_start, std::uint64_t bytes, int first_deadline, int step) {
            if (bytes == 0 || local_start >= file_size) return;
            bytes = std::min(bytes, file_size - local_start);
            const std::uint64_t absolute_start = file_start + local_start;
            const std::uint64_t absolute_end = absolute_start + bytes - 1;
            const int first = static_cast<int>(absolute_start / piece_length);
            const int last = static_cast<int>(absolute_end / piece_length);
            int deadline = first_deadline;
            for (int piece = first; piece <= last; ++piece) {
                if (!impl_->handle.have_piece(lt::piece_index_t(piece))) {
                    impl_->handle.set_piece_deadline(lt::piece_index_t(piece), deadline);
                }
                deadline += step;
            }
        };
        constexpr std::uint64_t immediate = 8ULL * 1024ULL * 1024ULL;
        constexpr std::uint64_t ahead = 48ULL * 1024ULL * 1024ULL;
        constexpr std::uint64_t rewind = 4ULL * 1024ULL * 1024ULL;
        schedule(offset, immediate, 0, 35);
        schedule(offset + immediate, ahead, 500, 70);
        const std::uint64_t rewind_start = offset > rewind ? offset - rewind : 0;
        schedule(rewind_start, offset - rewind_start, 3500, 100);
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
