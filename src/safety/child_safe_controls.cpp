#include "child_safe_controls.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>

namespace reddmedia::safety {
namespace {

std::string home_dir() {
    const char* home = std::getenv("HOME");
    return home && *home ? std::string(home) : std::string(".");
}

std::uint64_t rotate_left(std::uint64_t value, unsigned amount) {
    return (value << amount) | (value >> (64U - amount));
}

// A deliberately self-contained salted iterative digest. This is not used as
// encryption and does not expose the parent password. The high iteration count
// makes offline guessing materially more expensive while keeping Nougat free of
// an extra crypto-library runtime dependency.
std::string iterative_digest(const std::string& password, const std::string& salt) {
    std::array<std::uint64_t, 4> state = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL
    };
    std::string material = salt + "\x1f" + password;
    for (int round = 0; round < 120000; ++round) {
        for (unsigned char c : material) {
            state[0] ^= static_cast<std::uint64_t>(c) + 0x9e3779b97f4a7c15ULL;
            state[0] *= 0xbf58476d1ce4e5b9ULL;
            state[1] ^= rotate_left(state[0], 13U) + c;
            state[1] *= 0x94d049bb133111ebULL;
            state[2] += rotate_left(state[1] ^ state[0], 29U);
            state[2] ^= state[2] >> 31U;
            state[3] ^= rotate_left(state[2] + state[1], 41U);
            state[3] *= 0xd6e8feb86659fd93ULL;
        }
        state[0] ^= static_cast<std::uint64_t>(round) * 0x9e3779b97f4a7c15ULL;
        state[1] += rotate_left(state[3], 17U);
        state[2] ^= rotate_left(state[0], 37U);
        state[3] += state[1] ^ 0xa5a5a5a5a5a5a5a5ULL;
        std::ostringstream next;
        next << std::hex << state[0] << state[1] << state[2] << state[3] << salt << password;
        material = next.str();
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::uint64_t word : state) out << std::setw(16) << word;
    return out.str();
}

bool constant_time_equal(const std::string& a, const std::string& b) {
    const std::size_t n = a.size() > b.size() ? a.size() : b.size();
    unsigned diff = static_cast<unsigned>(a.size() ^ b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned char ac = i < a.size() ? static_cast<unsigned char>(a[i]) : 0U;
        const unsigned char bc = i < b.size() ? static_cast<unsigned char>(b[i]) : 0U;
        diff |= static_cast<unsigned>(ac ^ bc);
    }
    return diff == 0U;
}

} // namespace

ChildSafeControls::ChildSafeControls()
    : config_path_(home_dir() + "/.config/reddmedia/child_safe.conf") {}

bool ChildSafeControls::load(std::string& status) {
    enabled_ = false;
    salt_.clear();
    password_hash_.clear();
    std::ifstream in(config_path_);
    if (!in) {
        status = "Child Safe Controls are off.";
        return true;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("enabled=", 0U) == 0U) enabled_ = line.substr(8U) == "1";
        else if (line.rfind("salt=", 0U) == 0U) salt_ = line.substr(5U);
        else if (line.rfind("hash=", 0U) == 0U) password_hash_ = line.substr(5U);
    }
    if (enabled_ && !password_configured()) {
        enabled_ = false;
        status = "Child Safe configuration was incomplete and was kept off.";
        return false;
    }
    status = enabled_ ? "Child Safe Controls are on." : "Child Safe Controls are off.";
    return true;
}

bool ChildSafeControls::save(std::string& status) const {
    const std::string dir = home_dir() + "/.config";
    const std::string app = dir + "/reddmedia";
    mkdir(dir.c_str(), 0700);
    mkdir(app.c_str(), 0700);
    const std::string temporary = config_path_ + ".tmp";
    {
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) {
            status = "Could not write Child Safe configuration.";
            return false;
        }
        out << "enabled=" << (enabled_ ? "1" : "0") << "\n";
        out << "salt=" << salt_ << "\n";
        out << "hash=" << password_hash_ << "\n";
        out.flush();
        if (!out) {
            status = "Could not finish Child Safe configuration.";
            return false;
        }
    }
    chmod(temporary.c_str(), 0600);
    if (std::rename(temporary.c_str(), config_path_.c_str()) != 0) {
        std::remove(temporary.c_str());
        status = "Could not promote Child Safe configuration.";
        return false;
    }
    chmod(config_path_.c_str(), 0600);
    return true;
}

std::string ChildSafeControls::random_salt() {
    std::array<unsigned char, 24> bytes {};
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom) urandom.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!urandom) {
        std::random_device rd;
        for (auto& value : bytes) value = static_cast<unsigned char>(rd());
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char value : bytes) out << std::setw(2) << static_cast<unsigned>(value);
    return out.str();
}

std::string ChildSafeControls::derive_hash(const std::string& password, const std::string& salt) {
    return iterative_digest(password, salt);
}

bool ChildSafeControls::enable_with_password(const std::string& password, std::string& status) {
    if (password.size() < 6U) {
        status = "Parent password must contain at least 6 characters.";
        return false;
    }
    salt_ = random_salt();
    password_hash_ = derive_hash(password, salt_);
    enabled_ = true;
    if (!save(status)) {
        enabled_ = false;
        salt_.clear();
        password_hash_.clear();
        return false;
    }
    status = "Child Safe Controls enabled. System/settings now require the parent password.";
    return true;
}

bool ChildSafeControls::verify_password(const std::string& password) const {
    return password_configured() &&
           constant_time_equal(derive_hash(password, salt_), password_hash_);
}

bool ChildSafeControls::disable_with_password(const std::string& password, std::string& status) {
    if (!verify_password(password)) {
        status = "Parent password did not match.";
        return false;
    }
    enabled_ = false;
    if (!save(status)) {
        enabled_ = true;
        return false;
    }
    status = "Child Safe Controls disabled.";
    return true;
}

} // namespace reddmedia::safety
