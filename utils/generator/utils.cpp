/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <cstdio>
#include <fstream>
#include <sstream>

#include "generator.h"

namespace mjb {

// Path of a file relative to the repository root.
std::filesystem::path root_path(std::string_view relative) {
    return std::filesystem::path(ROOT) / relative;
}

// Read the whole file. Print an error and return nothing if it cannot be read.
std::optional<std::string> read_file(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);

    if(!file) {
        std::fprintf(stderr, "Cannot read %s\n", path.c_str());

        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

// Write the whole file. Return false and print an error if it cannot be written.
bool write_file(const std::filesystem::path &path, std::string_view content) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if(!file || !file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        std::fprintf(stderr, "Cannot write %s\n", path.c_str());

        return false;
    }

    return true;
}

// Replace the text between the first occurrence of start and the following occurrence of end with
// replacement.
bool substitute_block(std::string &content, std::string_view start,
    std::optional<std::string_view> end, std::string_view replacement) {
    const auto start_index = content.find(start);

    if(start_index == std::string::npos) {
        std::fprintf(stderr, "Block start not found: %.*s\n", static_cast<int>(start.size()),
            start.data());

        return false;
    }

    const auto replace_from = start_index + start.size();

    if(!end) {
        content.replace(replace_from, std::string::npos, replacement);
        content += '\n';

        return true;
    }

    if(const auto end_index = content.find(*end, replace_from); end_index == std::string::npos) {
        std::fprintf(stderr, "Block end not found: %.*s\n", static_cast<int>(end->size()),
            end->data());

        return false;
    } else {
        content.replace(replace_from, end_index - replace_from, replacement);
    }

    return true;
}

// Read the VERSION file. Print an error and return nothing if it is missing or malformed.
std::optional<Version> get_version() {
    const auto content = read_file(root_path("VERSION"));

    if(!content) {
        return std::nullopt;
    }

    Version v{};

    if(std::sscanf(content->c_str(), "%d.%d.%d", &v.major, &v.minor, &v.revision) != 3) {
        std::fprintf(stderr, "Malformed VERSION file: %s\n", content->c_str());

        return std::nullopt;
    }

    char hex[32];
    std::snprintf(hex, sizeof(hex), "0x%X", (v.major << 8) | (v.minor << 4) | v.revision);

    v.version = std::to_string(v.major) + "." + std::to_string(v.minor) + "." +
        std::to_string(v.revision);
    v.hex_number = hex;

    return v;
}

} // namespace mjb
