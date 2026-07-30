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
        let scalarCount = text.unicodeScalars.count
        let scalarLabel = scalarCount == 1 ? "scalar" : "scalars"
        return "\(text.utf8.count) bytes · \(scalarCount) \(scalarLabel)"
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
        operation: (
            UnsafePointer<CChar>?,
            Int,
            UnsafeMutablePointer<mjb_result>
        ) -> mjb_status
    ) -> [UInt8]? {
        let input = value.utf8CString

        return input.withUnsafeBufferPointer { inputBuffer in
            var result = mjb_result()
            let status = operation(
                inputBuffer.baseAddress,
                inputBuffer.count - 1,
                &result
            )

            guard status == MJB_STATUS_OK else {
                return nil
            }

            defer {
                _ = mjb_result_free(&result)
            }

            guard let output = result.output else {
                return result.output_size == 0 ? [] : nil
            }

            let bytes = UnsafeBufferPointer(start: output, count: Int(result.output_size))
            return bytes.map { UInt8(bitPattern: $0) }
        }
    }
}

enum MojibakeString {
    static func transform(
        _ value: String,
        operation: (
            UnsafePointer<CChar>?,
            Int,
            UnsafeMutablePointer<mjb_result>
        ) -> mjb_status
    ) -> String? {
        guard let bytes = MojibakeBytes.transform(value, operation: operation) else {
            return nil
        }

        return String(decoding: bytes, as: UTF8.self)
    }
}
