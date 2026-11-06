/**
 * The Mojibake library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 *
 * Generate the raw `extern "C"` declarations of the mojibake-sys crate.
 *
 * Twin of utils/generate/file-generators/rust-ffi.ts; the two must produce identical output. The
 * TypeScript generator reads utils/generate/functions.ts. This port reads the same metadata from
 * the function declarations that the TypeScript generator writes to src/mojibake.h.
 */

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "generator.h"

namespace {

struct Arg {
    std::string type;
    std::string name;
};

struct Function {
    std::string comment;
    bool nodiscard = false;
    std::string ret;
    std::string name;
    std::vector<Arg> args;
    bool variadic = false;
};

constexpr std::size_t MAX_WIDTH = 100;

constexpr std::string_view BLOCK_START = "// This functions list is automatically generated. Do "
                                         "not edit.\n// clang-format off\n";
constexpr std::string_view BLOCK_END = "// clang-format on";

constexpr std::string_view RUST_KEYWORDS[] = { "abstract", "as", "async", "await", "become", "box",
    "break", "const", "continue", "crate", "do", "dyn", "else", "enum", "extern", "false", "final",
    "fn", "for", "gen", "if", "impl", "in", "let", "loop", "macro", "match", "mod", "move", "mut",
    "override", "priv", "pub", "ref", "return", "self", "Self", "static", "struct", "super",
    "trait", "true", "try", "type", "typeof", "unsafe", "unsized", "use", "virtual", "where",
    "while", "yield" };

// C types that map to core::ffi aliases.
constexpr std::pair<std::string_view, std::string_view> FFI_TYPES[] = {
    { "char", "c_char" },
    { "int", "c_int" },
    { "unsigned int", "c_uint" },
    { "void", "c_void" },
};

// C types that map to Rust primitives.
constexpr std::pair<std::string_view, std::string_view> PRIMITIVE_TYPES[] = {
    { "bool", "bool" },
    { "size_t", "usize" },
    { "int32_t", "i32" },
    { "uint8_t", "u8" },
    { "uint32_t", "u32" },
};

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool is_identifier_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

std::string trim(std::string_view value) {
    std::size_t start = 0;
    std::size_t end = value.size();

    while(start < end && is_space(value[start])) {
        ++start;
    }

    while(end > start && is_space(value[end - 1])) {
        --end;
    }

    return std::string(value.substr(start, end - start));
}

bool starts_with(std::string_view value, std::string_view prefix) {
    return value.substr(0, prefix.size()) == prefix;
}

// Split "const mjb_text *text" into its type and its trailing identifier.
bool split_declarator(std::string_view declaration, std::string &type, std::string &name) {
    const std::string value = trim(declaration);
    std::size_t index = value.size();

    while(index > 0 && is_identifier_char(value[index - 1])) {
        --index;
    }

    if(index == 0 || index == value.size()) {
        return false;
    }

    type = trim(std::string_view(value).substr(0, index));
    name = value.substr(index);

    return true;
}

bool parse_declaration(std::string_view line, Function &function) {
    std::string_view rest = line.substr(std::string_view("MJB_EXPORT ").size());

    // Drop the MJB_* attributes, including the parenthesized MJB_PRINTF_FORMAT(x, y).
    while(starts_with(rest, "MJB_")) {
        std::size_t index = 0;

        while(index < rest.size() && is_identifier_char(rest[index])) {
            ++index;
        }

        const std::string_view attribute = rest.substr(0, index);

        if(attribute == "MJB_NODISCARD") {
            function.nodiscard = true;
        }

        if(index < rest.size() && rest[index] == '(') {
            index = rest.find(')', index);

            if(index == std::string_view::npos) {
                return false;
            }

            ++index;
        }

        while(index < rest.size() && rest[index] == ' ') {
            ++index;
        }

        rest = rest.substr(index);
    }

    const std::size_t open = rest.find('(');
    const std::size_t close = rest.rfind(')');

    if(open == std::string_view::npos || close == std::string_view::npos || close < open) {
        return false;
    }

    if(!split_declarator(rest.substr(0, open), function.ret, function.name)) {
        return false;
    }

    const std::string args = trim(rest.substr(open + 1, close - open - 1));

    if(args == "void") {
        return true;
    }

    std::size_t start = 0;

    while(start <= args.size()) {
        std::size_t comma = args.find(',', start);

        if(comma == std::string::npos) {
            comma = args.size();
        }

        const std::string arg = trim(std::string_view(args).substr(start, comma - start));

        if(arg == "...") {
            function.variadic = true;
        } else {
            Arg parsed;

            if(!split_declarator(arg, parsed.type, parsed.name)) {
                return false;
            }

            function.args.push_back(parsed);
        }

        start = comma + 1;
    }

    return true;
}

bool read_functions(std::vector<Function> &functions) {
    const auto header = mjb::read_file(mjb::root_path("src/mojibake.h"));

    if(!header) {
        return false;
    }

    const std::size_t start = header->find(BLOCK_START);
    const std::size_t end = start == std::string::npos ?
        start :
        header->find(BLOCK_END, start + BLOCK_START.size());

    if(start == std::string::npos || end == std::string::npos) {
        std::fprintf(stderr, "Function list not found in src/mojibake.h\n");

        return false;
    }

    const std::string_view block = std::string_view(*header).substr(start + BLOCK_START.size(),
        end - start - BLOCK_START.size());
    std::string comment;
    std::size_t index = 0;

    while(index < block.size()) {
        std::size_t newline = block.find('\n', index);

        if(newline == std::string_view::npos) {
            newline = block.size();
        }

        const std::string_view line = block.substr(index, newline - index);
        index = newline + 1;

        if(starts_with(line, "// ")) {
            comment = std::string(line.substr(3));
        } else if(starts_with(line, "MJB_EXPORT ")) {
            Function function;
            function.comment = comment;

            if(!parse_declaration(line, function)) {
                std::fprintf(stderr, "Cannot parse declaration: %.*s\n",
                    static_cast<int>(line.size()), line.data());

                return false;
            }

            functions.push_back(function);
            comment.clear();
        }
    }

    return true;
}

bool rust_type(const std::string &c_type, std::set<std::string> &ffi_types, std::string &output) {
    std::string type = trim(c_type);

    if(!type.empty() && type.back() == '*') {
        std::string base = trim(std::string_view(type).substr(0, type.size() - 1));
        const bool is_const = starts_with(base, "const ");

        if(is_const) {
            base = trim(std::string_view(base).substr(6));
        }

        std::string inner;

        if(!rust_type(base, ffi_types, inner)) {
            return false;
        }

        output = std::string(is_const ? "*const " : "*mut ") + inner;

        return true;
    }

    for(const auto &[c_name, rust_name] : FFI_TYPES) {
        if(type == c_name) {
            ffi_types.insert(std::string(rust_name));
            output = rust_name;

            return true;
        }
    }

    for(const auto &[c_name, rust_name] : PRIMITIVE_TYPES) {
        if(type == c_name) {
            output = rust_name;

            return true;
        }
    }

    // Mojibake types are declared by hand in lib.rs. C enums are `c_int` aliases.
    bool is_mojibake = starts_with(type, "mjb_") && type.size() > 4;

    for(const char c : type) {
        if(!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            is_mojibake = false;
        }
    }

    if(!is_mojibake) {
        std::fprintf(stderr, "Rust FFI: unsupported C type \"%s\"\n", c_type.c_str());

        return false;
    }

    output = type;

    return true;
}

std::string rust_name(const std::string &name) {
    for(const std::string_view keyword : RUST_KEYWORDS) {
        if(name == keyword) {
            return name + "_";
        }
    }

    return name;
}

// A function is bound unless it is variadic or takes a va_list, which stable Rust cannot declare.
bool is_bindable(const Function &function) {
    if(function.variadic) {
        return false;
    }

    return std::none_of(function.args.begin(), function.args.end(),
        [](const Arg &arg) { return trim(arg.type) == "va_list"; });
}

bool format_function(const Function &function, std::set<std::string> &ffi_types,
    std::string &output) {
    output = "    /// " + function.comment + "\n";

    if(function.nodiscard) {
        output += "    #[must_use]\n";
    }

    std::string ret;

    if(trim(function.ret) != "void") {
        std::string type;

        if(!rust_type(function.ret, ffi_types, type)) {
            return false;
        }

        ret = " -> " + type;
    }

    std::vector<std::string> args;

    for(const Arg &arg : function.args) {
        std::string type;

        if(!rust_type(arg.type, ffi_types, type)) {
            return false;
        }

        args.push_back(rust_name(arg.name) + ": " + type);
    }

    std::string one_line = "    pub fn " + function.name + "(";

    for(std::size_t i = 0; i < args.size(); ++i) {
        one_line += (i == 0 ? "" : ", ") + args[i];
    }

    one_line += ")" + ret + ";";

    // Match rustfmt: one line when it fits, otherwise one argument per line.
    if(one_line.size() <= MAX_WIDTH || args.empty()) {
        output += one_line;
    } else {
        output += "    pub fn " + function.name + "(\n";

        for(const std::string &arg : args) {
            output += "        " + arg + ",\n";
        }

        output += "    )" + ret + ";";
    }

    return true;
}

} // namespace

namespace mjb {

bool generate_rust_ffi() {
    std::vector<Function> functions;

    if(!read_functions(functions)) {
        return false;
    }

    std::set<std::string> ffi_types;
    std::string skipped;
    std::string body;

    for(const Function &function : functions) {
        if(!is_bindable(function)) {
            skipped += "\n// - " + function.name;

            continue;
        }

        std::string formatted;

        if(!format_function(function, ffi_types, formatted)) {
            return false;
        }

        body += (body.empty() ? "" : "\n\n") + formatted;
    }

    std::string imports;

    for(const std::string &type : ffi_types) {
        imports += (imports.empty() ? "" : ", ") + type;
    }

    std::string output;
    output += "// The Mojibake library\n";
    output += "//\n";
    output += "// This file is distributed under the MIT License. See LICENSE for details.\n";
    output += "\n";
    output += "// This file is automatically generated from utils/generate/functions.ts. ";
    output += "Do not edit.\n";
    output += "//\n";
    output += "// Variadic functions and functions that take a `va_list` are not bound:";
    output += skipped + "\n";
    output += "\n";
    output += "use core::ffi::{" + imports + "};\n";
    output += "\n";
    output += "use super::*;\n";
    output += "\n";
    output += "extern \"C\" {\n";
    output += body + "\n";
    output += "}\n";

    return write_file(root_path("src/rust/mojibake-sys/src/functions.rs"), output);
}

} // namespace mjb
