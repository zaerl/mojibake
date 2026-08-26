// The Mojibake library
//
// This file is distributed under the MIT License. See LICENSE for details.

const std = @import("std");
const mjb = @import("mojibake");

// This is a simple Zig example of how to use the Mojibake library.
// Run `make example-zig` to compile it.
// ./build/example-zig to run it.
pub fn main(init: std.process.Init) !u8 {
    var stdout_buffer: [1024]u8 = undefined;
    var stdout_writer = std.Io.File.stdout().writer(init.io, &stdout_buffer);
    const stdout = &stdout_writer.interface;
    defer stdout.flush() catch {};

    const input = "Cafe\u{0301}";
    {
        var result: mjb.mjb_result = .{};

        // Normalize example: in NFC e + ◌́ -> é (U+00E9)
        if (mjb.mjb_normalize(
            input.ptr,
            input.len,
            mjb.MJB_ENC_UTF_8,
            mjb.MJB_MALFORMED_STOP,
            mjb.MJB_NORMALIZATION_NFC,
            mjb.MJB_ENC_UTF_8,
            &result,
            null,
        ) != mjb.MJB_STATUS_OK) {
            return 1;
        }
        defer _ = mjb.mjb_result_free(&result);

        // Cafe + ◌́ (U+0301, COMBINING ACUTE ACCENT) -> Café
        try printString(stdout, input);

        // Caf + é (U+00E9, LATIN SMALL LETTER E WITH ACUTE) -> Café
        try printString(stdout, result.output[0..result.output_size]);

        const mojibake = "文字化け";

        // Codepoint count example: mjb_codepoint_count counts Unicode codepoints, not bytes.
        var codepoint_count: usize = 0;

        if(mjb.mjb_codepoint_count(
            mojibake.ptr,
            mojibake.len,
            mjb.MJB_ENC_UTF_8,
            mjb.MJB_MALFORMED_STOP,
            &codepoint_count,
            null,
        ) != mjb.MJB_STATUS_OK) {
            return error.CodepointCountFailed;
        }

        try stdout.print(
            "\"{s}\" encoded in UTF-8 is {d} bytes long, and {d} codepoints long\n",
            .{ mojibake, mojibake.len, codepoint_count },
        );
    }

    const case_input = "Straße";
    {
        var result: mjb.mjb_result = .{};

        // NFKC casefold example: in NFKC casefold, ß -> ss
        if (mjb.mjb_nfkc_casefold(
            case_input.ptr,
            case_input.len,
            mjb.MJB_ENC_UTF_8,
            mjb.MJB_MALFORMED_STOP,
            mjb.MJB_ENC_UTF_8,
            &result,
            null,
        ) != mjb.MJB_STATUS_OK) {
            return 1;
        }
        defer _ = mjb.mjb_result_free(&result);

        try stdout.print("{s} -> {s}\n", .{
            case_input,
            result.output[0..result.output_size],
        });
    }

    try stdout.flush();

    return 0;
}

fn printString(stdout: *std.Io.Writer, input: []const u8) std.Io.Writer.Error!void {
    for (input) |byte| {
        if (byte >= 0x21 and byte <= 0x7E) {
            try stdout.writeByte(byte);
        } else {
            try stdout.print("<{X:0>2}>", .{byte});
        }
    }

    try stdout.writeByte('\n');
}
