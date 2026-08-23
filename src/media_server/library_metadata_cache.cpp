#include "library_metadata_cache.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace reddmedia {
namespace {

std::string default_cache_directory() {
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.cache/reddmedia/library_metadata_v30";
}

bool ensure_directory(const std::string& path) {
    if (path.empty()) return false;
    std::string current;
    if (path.front() == '/') current = "/";
    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '/') current += '/';
            current += part;
            struct stat st {};
            if (stat(current.c_str(), &st) != 0) {
                if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) return false;
            } else if (!S_ISDIR(st.st_mode)) return false;
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    chmod(path.c_str(), 0700);
    return true;
}

std::string hex_encode(const std::string& value) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2U);
    for (unsigned char c : value) {
        out.push_back(digits[(c >> 4U) & 0x0fU]);
        out.push_back(digits[c & 0x0fU]);
    }
    return out;
}

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool hex_decode(const std::string& value, std::string& out) {
    if ((value.size() % 2U) != 0U) return false;
    out.clear();
    out.reserve(value.size() / 2U);
    for (std::size_t i = 0; i < value.size(); i += 2U) {
        const int hi = hex_value(value[i]);
        const int lo = hex_value(value[i + 1U]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

std::string join_genres(const std::vector<std::string>& genres) {
    std::string joined;
    for (std::size_t i = 0; i < genres.size(); ++i) {
        if (i > 0U) joined.push_back('\x1f');
        joined += genres[i];
    }
    return joined;
}

std::vector<std::string> split_genres(const std::string& joined) {
    std::vector<std::string> genres;
    std::size_t start = 0;
    while (start <= joined.size()) {
        const std::size_t pos = joined.find('\x1f', start);
        genres.push_back(joined.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
        if (pos == std::string::npos) break;
        start = pos + 1U;
    }
    if (genres.size() == 1U && genres.front().empty()) genres.clear();
    return genres;
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t pos = line.find('\t', start);
        fields.push_back(line.substr(start, pos == std::string::npos ? std::string::npos : pos - start));
        if (pos == std::string::npos) break;
        start = pos + 1U;
    }
    return fields;
}

std::string deterministic_key_hash(const std::string& key) {
    unsigned long long hash = 1469598103934665603ULL;
    for (unsigned char c : key) {
        hash ^= static_cast<unsigned long long>(c);
        hash *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

bool parse_int(const std::string& value, int& out) {
    try {
        std::size_t used = 0;
        const long parsed = std::stol(value, &used, 10);
        if (used != value.size()) return false;
        out = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

LibraryMetadataCache::LibraryMetadataCache(std::string directory)
    : directory_(directory.empty() ? default_cache_directory() : std::move(directory)) {}

std::string LibraryMetadataCache::path_for_key(const std::string& key) const {
    return directory_ + "/" + deterministic_key_hash(key) + ".cache";
}

bool LibraryMetadataCache::load(const std::string& key,
                                std::vector<LibraryNode>& nodes,
                                std::string& error) const {
    nodes.clear();
    std::ifstream input(path_for_key(key));
    if (!input) {
        error.clear();
        return false;
    }
    std::string header;
    if (!std::getline(input, header) || header != "NOUGAT_LIBRARY_CACHE_V1") {
        error = "Cached library metadata has an unsupported format.";
        return false;
    }
    std::string encoded_key;
    if (!std::getline(input, encoded_key)) {
        error = "Cached library metadata is incomplete.";
        return false;
    }
    std::string decoded_key;
    if (!hex_decode(encoded_key, decoded_key) || decoded_key != key) {
        error = "Cached library metadata key does not match this view.";
        return false;
    }

    std::vector<LibraryNode> loaded;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        const std::vector<std::string> f = split_tabs(line);
        if (f.size() != 24U) {
            error = "Cached library metadata contains a malformed item.";
            return false;
        }
        std::string strings[17];
        for (std::size_t i = 0; i < 17U; ++i) {
            if (!hex_decode(f[i], strings[i])) {
                error = "Cached library metadata contains invalid text encoding.";
                return false;
            }
        }
        LibraryNode node;
        node.id = strings[0];
        node.parent_id = strings[1];
        node.series_id = strings[2];
        node.season_id = strings[3];
        node.name = strings[4];
        node.path = strings[5];
        node.overview = strings[6];
        node.series_name = strings[7];
        node.episode_title = strings[8];
        node.technical_details = strings[9];
        node.genres = split_genres(strings[10]);
        node.primary_image_tag = strings[11];
        node.backdrop_image_tag = strings[12];
        node.poster_item_id = strings[13];
        node.poster_image_tag = strings[14];
        node.tmdb_poster_path = strings[15];
        node.tmdb_id = strings[16];
        std::string series_tmdb;
        if (!hex_decode(f[17], series_tmdb)) {
            error = "Cached library metadata contains invalid series metadata.";
            return false;
        }
        node.series_tmdb_id = series_tmdb;
        int kind = 0;
        if (!parse_int(f[18], kind) || !parse_int(f[19], node.production_year) ||
            !parse_int(f[20], node.child_count) || !parse_int(f[21], node.season_number) ||
            !parse_int(f[22], node.episode_number)) {
            error = "Cached library metadata contains invalid numeric metadata.";
            return false;
        }
        const int first = static_cast<int>(LibraryNodeKind::Movie);
        const int last = static_cast<int>(LibraryNodeKind::Episode);
        if (kind < first || kind > last) {
            error = "Cached library metadata contains an invalid media kind.";
            return false;
        }
        node.kind = static_cast<LibraryNodeKind>(kind);
        std::string reserved;
        if (!hex_decode(f[23], reserved)) {
            error = "Cached library metadata contains invalid reserved metadata.";
            return false;
        }
        loaded.push_back(std::move(node));
    }
    nodes = std::move(loaded);
    error.clear();
    return true;
}

bool LibraryMetadataCache::store(const std::string& key,
                                 const std::vector<LibraryNode>& nodes,
                                 std::string& error) const {
    if (!ensure_directory(directory_)) {
        error = "Nougat could not create its private library metadata cache directory.";
        return false;
    }
    const std::string final_path = path_for_key(key);
    const std::string temp_path = final_path + ".tmp." + std::to_string(static_cast<long long>(getpid()));
    std::ofstream output(temp_path, std::ios::trunc);
    if (!output) {
        error = "Nougat could not open its library metadata cache for writing.";
        return false;
    }
    output << "NOUGAT_LIBRARY_CACHE_V1\n" << hex_encode(key) << '\n';
    for (const LibraryNode& node : nodes) {
        const std::string string_fields[] = {
            node.id, node.parent_id, node.series_id, node.season_id, node.name, node.path,
            node.overview, node.series_name, node.episode_title, node.technical_details,
            join_genres(node.genres), node.primary_image_tag, node.backdrop_image_tag,
            node.poster_item_id, node.poster_image_tag, node.tmdb_poster_path, node.tmdb_id,
            node.series_tmdb_id
        };
        for (const std::string& field : string_fields) output << hex_encode(field) << '\t';
        output << static_cast<int>(node.kind) << '\t'
               << node.production_year << '\t' << node.child_count << '\t'
               << node.season_number << '\t' << node.episode_number << '\t'
               << hex_encode("") << '\n';
    }
    output.flush();
    if (!output.good()) {
        output.close();
        std::remove(temp_path.c_str());
        error = "Nougat could not finish writing its library metadata cache.";
        return false;
    }
    output.close();
    chmod(temp_path.c_str(), 0600);
    if (std::rename(temp_path.c_str(), final_path.c_str()) != 0) {
        std::remove(temp_path.c_str());
        error = "Nougat could not replace its library metadata cache atomically.";
        return false;
    }
    chmod(final_path.c_str(), 0600);
    error.clear();
    return true;
}

bool LibraryMetadataCache::remove(const std::string& key, std::string& error) const {
    const std::string path = path_for_key(key);
    if (std::remove(path.c_str()) == 0 || errno == ENOENT) {
        error.clear();
        return true;
    }
    error = "Nougat could not remove an obsolete library metadata cache entry.";
    return false;
}

} // namespace reddmedia
