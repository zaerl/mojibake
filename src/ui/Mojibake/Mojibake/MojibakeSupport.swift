//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import Foundation
import SwiftUI

struct ToolHeader: View {
    let title: LocalizedStringKey
    let description: LocalizedStringKey

    init(_ title: LocalizedStringKey, description: LocalizedStringKey) {
        self.title = title
        self.description = description
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(title)
                .font(.largeTitle)

            Text(description)
                .foregroundStyle(.secondary)
        }
    }
}

struct ToolTextEditor: View {
    let title: LocalizedStringKey
    @Binding var text: String
    let minimumHeight: CGFloat

    init(
        _ title: LocalizedStringKey,
        text: Binding<String>,
        minimumHeight: CGFloat = 100
    ) {
        self.title = title
        _text = text
        self.minimumHeight = minimumHeight
    }

    var body: some View {
        GroupBox {
            TextEditor(text: $text)
                .font(.body)
                .frame(minHeight: minimumHeight)
                .accessibilityLabel(title)
        } label: {
            HStack {
                Text(title)
                    .font(.headline)

                Spacer()

                Text(summary)
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
        }
        .frame(minWidth: 260, maxWidth: .infinity)
    }

    private var summary: String {
        MojibakeCounting.summary(for: text)
    }
}

enum MojibakeCounting {
    static func codepointCount(of value: String) -> Int? {
        count(of: value, using: mjb_codepoint_count)
    }

    static func graphemeCount(of value: String) -> Int? {
        count(of: value, using: mjb_grapheme_count)
    }

    static func wordCount(of value: String) -> Int? {
        count(of: value, using: mjb_word_count)
    }

    static func summary(for value: String) -> String {
        var components = ["\(value.utf8.count) bytes"]

        if let scalarCount = codepointCount(of: value) {
            components.append(counted(scalarCount, "scalar", "scalars"))
        }

        if let graphemeCount = graphemeCount(of: value) {
            components.append(counted(graphemeCount, "grapheme", "graphemes"))
        }

        if let wordCount = wordCount(of: value) {
            components.append(counted(wordCount, "word", "words"))
        }

        return components.joined(separator: " · ")
    }

    private static func counted(_ count: Int, _ singular: String, _ plural: String) -> String {
        "\(count) \(count == 1 ? singular : plural)"
    }

    private static func count(
        of value: String,
        using function: (
            UnsafePointer<mjb_text>?,
            mjb_malformed_policy,
            UnsafeMutablePointer<Int>?,
            UnsafeMutablePointer<mjb_diagnostic>?
        ) -> mjb_status
    ) -> Int? {
        MojibakeText.with(value) { text in
            var count = 0

            guard function(text, MJB_MALFORMED_STOP, &count, nil) == MJB_STATUS_OK else {
                return nil
            }

            return count
        }
    }
}

enum MojibakeText {
    /// Calls `body` with a `mjb_text` that borrows the UTF-8 bytes of `value`.
    static func with<Result>(
        _ value: String,
        _ body: (UnsafePointer<mjb_text>) -> Result
    ) -> Result {
        with(Array(value.utf8), encoding: MJB_ENC_UTF_8, body)
    }

    /// Calls `body` with a `mjb_text` that borrows `bytes`. The pointer is valid only inside `body`.
    static func with<Result>(
        _ bytes: [UInt8],
        encoding: mjb_encoding,
        _ body: (UnsafePointer<mjb_text>) -> Result
    ) -> Result {
        bytes.withUnsafeBytes { rawBuffer in
            with(
                rawBuffer.bindMemory(to: CChar.self).baseAddress,
                byteLength: rawBuffer.count,
                encoding: encoding,
                body
            )
        }
    }

    /// Calls `body` with a `mjb_text` that borrows `byteLength` bytes starting at `buffer`.
    static func with<Result>(
        _ buffer: UnsafePointer<CChar>?,
        byteLength: Int,
        encoding: mjb_encoding = MJB_ENC_UTF_8,
        _ body: (UnsafePointer<mjb_text>) -> Result
    ) -> Result {
        var text = mjb_text(buffer: buffer, byte_length: byteLength, encoding: encoding)

        return withUnsafePointer(to: &text, body)
    }
}

enum MojibakeFormatting {
    static func yesOrNo(_ value: Bool) -> String {
        value ? String(localized: "Yes") : String(localized: "No")
    }

    static func idAndName(_ id: Int, _ name: String) -> String {
        "[\(id)] \(name)"
    }

    static func codepoint(_ codepoint: mjb_codepoint) -> String {
        String(format: "U+%04X", codepoint)
    }

    static func string<T>(fromCString value: T) -> String {
        var value = value

        return withUnsafePointer(to: &value) {
            $0.withMemoryRebound(to: CChar.self, capacity: MemoryLayout<T>.size) {
                String(cString: $0)
            }
        }
    }

    static func statusMessage(operation: String, status: mjb_status) -> String {
        let message = mjb_status_message(status).map(String.init(cString:)) ?? "Unknown error"
        return "\(operation): \(message)"
    }
}

enum MojibakeBytes {
    static func transform(
        _ value: String,
        operation: (UnsafePointer<mjb_text>, UnsafeMutablePointer<mjb_result>) -> mjb_status
    ) -> [UInt8]? {
        transform(Array(value.utf8), encoding: MJB_ENC_UTF_8, operation: operation)
    }

    static func transform(
        _ bytes: [UInt8],
        encoding: mjb_encoding,
        operation: (UnsafePointer<mjb_text>, UnsafeMutablePointer<mjb_result>) -> mjb_status
    ) -> [UInt8]? {
        MojibakeText.with(bytes, encoding: encoding) { text in
            var result = mjb_result()
            let status = operation(text, &result)

            guard status == MJB_STATUS_OK else {
                return nil
            }

            defer {
                _ = mjb_result_free(&result)
            }

            guard let output = result.output else {
                return result.output_size == 0 ? [] : nil
            }

            let outputBytes = UnsafeBufferPointer(start: output, count: Int(result.output_size))
            return outputBytes.map { UInt8(bitPattern: $0) }
        }
    }
}

enum MojibakeString {
    static func transform(
        _ value: String,
        operation: (UnsafePointer<mjb_text>, UnsafeMutablePointer<mjb_result>) -> mjb_status
    ) -> String? {
        guard let bytes = MojibakeBytes.transform(value, operation: operation) else {
            return nil
        }

        return String(decoding: bytes, as: UTF8.self)
    }
}
