/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#ifndef MJB_GENERATOR_H
#define MJB_GENERATOR_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace mjb {

// Path of the repository root relative to the generator working directory (utils/generate).
constexpr std::string_view ROOT = "../../";

struct Version {
    int major;
    int minor;
    int revision;
    // MAJOR.MINOR.REVISION
    std::string version;
    // MAJOR << 8 | MINOR << 4 | REVISION as an upper-case hex literal, ie: 0x41
    std::string hex_number;
};

// utils.cpp

// Path of a file relative to the repository root.
std::filesystem::path root_path(std::string_view relative);

// Read the whole file. Print an error and return nothing if it cannot be read.
std::optional<std::string> read_file(const std::filesystem::path &path);

// Write the whole file. Return false and print an error if it cannot be written.
bool write_file(const std::filesystem::path &path, std::string_view content);

// Replace the text between the first occurrence of start and the following occurrence of end with
// replacement.
bool substitute_block(std::string &content, std::string_view start,
    std::optional<std::string_view> end, std::string_view replacement);

// Read the VERSION file. Print an error and return nothing if it is missing or malformed.
std::optional<Version> get_version();

// update-version.cpp

// Propagate the VERSION file to every source, package and documentation file that embeds it.
// Return false if any file cannot be updated.
bool update_version();

} // namespace mjb

#endif // MJB_GENERATOR_H
