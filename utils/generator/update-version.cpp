/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 *
 * Port of utils/generate/commands/update-version.ts.
 */

#include <cstdio>
#include <initializer_list>

#include "generator.h"

namespace mjb {

namespace {

struct Substitution {
    std::string_view start;
    // Without end everything after start is replaced.
    std::optional<std::string_view> end;
    std::string replacement;
};

// Apply every substitution to the file in order and write it back.
bool update_file(const std::filesystem::path &path,
    std::initializer_list<Substitution> substitutions) {
    auto content = read_file(path);

    if(!content) {
        return false;
    }

    for(const auto &[start, end, replacement] : substitutions) {
        if(!substitute_block(*content, start, end, replacement)) {
            std::fprintf(stderr, "  in %s\n", path.c_str());

            return false;
        }
    }

    return write_file(path, *content);
}

// Update the version field of every SKILL.md under dir, recursively.
bool update_skill_md_files(const std::filesystem::path &dir, const Version &v) {
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(dir, error);

    if(error) {
        std::fprintf(stderr, "Cannot read %s: %s\n", dir.c_str(), error.message().c_str());

        return false;
    }

    for(const auto &entry : it) {
        if(entry.is_regular_file() && entry.path().filename() == "SKILL.md") {
            if(!update_file(entry.path(), { { "version: ", "\n", v.version } })) {
                return false;
            }
        }
    }

    return true;
}

} // namespace

bool update_version() {
    const auto version = get_version();

    if(!version) {
        return false;
    }

    const Version &v = *version;

    if(!update_file(root_path("src/mojibake.h"),
           {
               { "#define MJB_VERSION_NUMBER", "\n",
                   " " + v.hex_number + " // MAJOR << 8 | MINOR << 4 | REVISION" },
               { "#define MJB_VERSION_MAJOR", "\n", " " + std::to_string(v.major) },
               { "#define MJB_VERSION_MINOR", "\n", " " + std::to_string(v.minor) },
               { "#define MJB_VERSION_REVISION", "\n", " " + std::to_string(v.revision) },
               { "#define MJB_VERSION \"", "-WASM\"\n", v.version },
               { "#else\n    #define MJB_VERSION \"", "\"\n", v.version },
           })) {
        return false;
    }

    if(!update_file(root_path("CMakeLists.txt"), { { "    VERSION ", "\n", v.version } })) {
        return false;
    }

    if(!update_file("package.json", { { "\"version\": \"", "\",\n", v.version } })) {
        return false;
    }

    if(!update_file(root_path("src/api/package.json"),
           {
               { "\"version\": \"", "\",\n", v.version },
           })) {
        return false;
    }

    if(!update_file(root_path("src/api/tests/index.ts"),
           {
               { "mojibake.version(), '", "-WASM", v.version },
               { "versionNumber(), ", ",", v.hex_number },
           })) {
        return false;
    }

    if(!update_file(root_path("src/api/tests/browser.html"),
           {
               { "mojibake.version(), '", "-WASM", v.version },
           })) {
        return false;
    }

    const std::string zip = "mojibake-amalgamation-" + std::to_string(v.major) +
        std::to_string(v.minor) + std::to_string(v.revision);

    if(!update_file(root_path("README.md"),
           {
               { "Download it here [", ".zip)",
                   zip + ".zip](https://github.com/zaerl/mojibake/releases/download/v" + v.version +
                       "/" + zip },
           })) {
        return false;
    }

    if(!update_skill_md_files(root_path(".claude/skills"), v)) {
        return false;
    }

    std::printf("\nVersion updated to %s\n", v.version.c_str());
    std::puts("make wasm");
    std::puts("make amalgamation");
    std::puts("npm i in utils/generate");
    std::puts("npm i in src/api");
    std::printf("Update CHANGELOG.md: add a [%s] section\n", v.version.c_str());
    std::printf("git commit --signoff -am \"Update version to %s\"\n", v.version.c_str());
    std::printf("git tag v%s\n", v.version.c_str());
    std::puts("git push && git push origin --tags");
    std::puts("Deploy");

    return true;
}

} // namespace mjb
