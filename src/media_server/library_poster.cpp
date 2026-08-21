#include "library_poster.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::uint16_t read_u16(const std::string& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset])) |
        static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset + 1U]) << 8U);
}

std::uint32_t read_u32(const std::string& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1U])) << 8U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2U])) << 16U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3U])) << 24U);
}

} // namespace

bool normalize_library_poster_bmp(const std::string& source_bytes,
                                  std::string& bmp_bytes,
                                  std::string& error) {
    if (source_bytes.size() >= 2U && source_bytes[0] == 'B' && source_bytes[1] == 'M') {
        bmp_bytes = source_bytes;
        return true;
    }
    if (source_bytes.empty()) {
        error = "The poster image is empty.";
        return false;
    }

    char input_template[] = "/tmp/reddmedia-poster-input.XXXXXX";
    char output_template[] = "/tmp/reddmedia-poster-output.XXXXXX";
    const int input_fd = mkstemp(input_template);
    if (input_fd < 0) {
        error = "ReddMedia could not create a temporary poster input.";
        return false;
    }
    const int output_fd = mkstemp(output_template);
    if (output_fd < 0) {
        close(input_fd);
        unlink(input_template);
        error = "ReddMedia could not create a temporary poster output.";
        return false;
    }
    close(output_fd);

    std::size_t written = 0;
    while (written < source_bytes.size()) {
        const ssize_t amount = write(input_fd, source_bytes.data() + written,
                                     source_bytes.size() - written);
        if (amount <= 0) break;
        written += static_cast<std::size_t>(amount);
    }
    const bool input_ok = written == source_bytes.size() && close(input_fd) == 0;
    if (!input_ok) {
        unlink(input_template);
        unlink(output_template);
        error = "ReddMedia could not prepare the poster image.";
        return false;
    }

    const pid_t child = fork();
    if (child < 0) {
        unlink(input_template);
        unlink(output_template);
        error = "ReddMedia could not start the poster decoder.";
        return false;
    }
    if (child == 0) {
        const int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
        }
        execlp("ffmpeg", "ffmpeg", "-v", "error", "-y", "-i", input_template,
               "-frames:v", "1", "-f", "image2", "-vcodec", "bmp",
               output_template, static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    unlink(input_template);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        unlink(output_template);
        error = "ReddMedia could not decode this poster image with FFmpeg.";
        return false;
    }

    std::ifstream input(output_template, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    unlink(output_template);
    bmp_bytes = contents.str();
    if (bmp_bytes.size() < 2U || bmp_bytes[0] != 'B' || bmp_bytes[1] != 'M') {
        error = "The poster decoder did not return a usable image.";
        bmp_bytes.clear();
        return false;
    }
    return true;
}

bool decode_library_poster_bmp(const std::string& bytes,
                               LibraryPoster& poster,
                               std::string& error) {
    if (bytes.size() < 54U || bytes[0] != 'B' || bytes[1] != 'M') {
        error = "The local catalog returned an invalid poster image.";
        return false;
    }
    const std::uint32_t pixel_offset = read_u32(bytes, 10U);
    const std::int32_t width = static_cast<std::int32_t>(read_u32(bytes, 18U));
    const std::int32_t signed_height = static_cast<std::int32_t>(read_u32(bytes, 22U));
    const std::uint16_t planes = read_u16(bytes, 26U);
    const std::uint16_t bits = read_u16(bytes, 28U);
    const std::uint32_t compression = read_u32(bytes, 30U);
    if (width <= 0 || signed_height == 0 || planes != 1U ||
        (bits != 24U && bits != 32U) || compression != 0U) {
        error = "ReddMedia supports uncompressed 24-bit or 32-bit catalog posters.";
        return false;
    }
    const int height = signed_height < 0 ? -signed_height : signed_height;
    if (width > 4096 || height > 4096) {
        error = "The catalog poster is too large to display safely.";
        return false;
    }
    const std::size_t bytes_per_pixel = bits / 8U;
    const std::size_t row_bytes =
        ((static_cast<std::size_t>(width) * bytes_per_pixel + 3U) / 4U) * 4U;
    if (pixel_offset > bytes.size() ||
        row_bytes > (bytes.size() - pixel_offset) / static_cast<std::size_t>(height)) {
        error = "The local catalog returned a truncated poster image.";
        return false;
    }
    LibraryPoster decoded;
    decoded.width = width;
    decoded.height = height;
    decoded.rgb.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3U);
    for (int y = 0; y < height; ++y) {
        const int source_y = signed_height < 0 ? y : height - y - 1;
        const std::size_t source_row = pixel_offset + static_cast<std::size_t>(source_y) * row_bytes;
        for (int x = 0; x < width; ++x) {
            const std::size_t source = source_row + static_cast<std::size_t>(x) * bytes_per_pixel;
            const std::size_t target =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                 static_cast<std::size_t>(x)) * 3U;
            decoded.rgb[target] = static_cast<std::uint8_t>(bytes[source + 2U]);
            decoded.rgb[target + 1U] = static_cast<std::uint8_t>(bytes[source + 1U]);
            decoded.rgb[target + 2U] = static_cast<std::uint8_t>(bytes[source]);
        }
    }
    poster = std::move(decoded);
    return true;
}

} // namespace reddmedia
