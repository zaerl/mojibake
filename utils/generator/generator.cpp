/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 *
 * Native replacement for utils/generate/commands/generate.ts.
 *
 * Usage: generator [-v|--verbose] [generate-locale|amalgamation|unicode-tables|update-version]
 *
 * With no command the full generation runs.
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "generator.h"

namespace {

bool verbose = false;

// Log a message only in verbose mode.
void log(const char *message) {
    if(verbose) {
        std::puts(message);
    }
}

// Log a message unconditionally.
void ilog(const char *message) {
    std::puts(message);
}

enum class Command : std::uint8_t {
    NONE,
    LOCALE,
    AMALGAMATION,
    UNICODE_TABLES,
    UPDATE_VERSION
};

// generate-locale: refresh the CLDR locale data.
void generate_locale(const std::string &locale) {
    (void)locale;
}

// amalgamation: emit the single-file library and the shell amalgamation.
void generate_amalgamation() {
}

void generate_shell_amalgamation() {
}

// unicode-tables: parse the UCD inputs and emit the Unicode data header.
void build_unicode_table_data() {
}

void generate_unicode_data_header() {
}

// Full run (no command): everything above plus the remaining generated files.
void generate_header() {
}

void generate_wasm() {
}

void generate_api() {
}

void generate_example_c() {
}

void generate_wasm_dts() {
}

void usage(const char *program) {
    std::fprintf(stderr,
        "Usage: %s [-v|--verbose] [command]\n"
        "\n"
        "Commands:\n"
        "  generate-locale  Refresh the locale data\n"
        "  amalgamation     Generate the single-file amalgamation\n"
        "  unicode-tables   Generate the Unicode data tables\n"
        "  update-version   Update the version in every tracked file\n"
        "\n"
        "With no command the full generation runs.\n",
        program);
}

bool parse_args(int argc, char **argv, Command &command) {
    command = Command::NONE;

    for(int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if(std::strcmp(arg, "-v") == 0 || std::strcmp(arg, "--verbose") == 0) {
            verbose = true;
        } else if(std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
            usage(argv[0]);

            return false;
        } else if(std::strcmp(arg, "generate-locale") == 0) {
            command = Command::LOCALE;
        } else if(std::strcmp(arg, "amalgamation") == 0) {
            command = Command::AMALGAMATION;
        } else if(std::strcmp(arg, "unicode-tables") == 0) {
            command = Command::UNICODE_TABLES;
        } else if(std::strcmp(arg, "update-version") == 0) {
            command = Command::UPDATE_VERSION;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n\n", arg);
            usage(argv[0]);

            return false;
        }
    }

    return true;
}

bool generate(Command command) {
    switch(command) {
        case Command::LOCALE:
            ilog("Generate locale");
            generate_locale("it");
            break;
        case Command::AMALGAMATION:
            ilog("Generate amalgamation");
            generate_amalgamation();
            generate_shell_amalgamation();
            break;
        case Command::UNICODE_TABLES:
            ilog("Build Unicode table data");
            build_unicode_table_data();
            generate_unicode_data_header();
            break;
        case Command::UPDATE_VERSION:
            ilog("Update version");

            if(!mjb::update_version()) {
                return false;
            }

            break;
        case Command::NONE:
            ilog("Build Unicode table data");
            build_unicode_table_data();
            generate_unicode_data_header();
            generate_header();
            generate_wasm();
            generate_api();
            generate_example_c();
            generate_wasm_dts();
            break;
    }

    log("Done");

    return true;
}

} // namespace

int main(int argc, char **argv) {
    Command command;

    if(!parse_args(argc, argv, command)) {
        return 1;
    }

    return generate(command) ? 0 : 1;
}
