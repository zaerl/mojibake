//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct EncodingInspectorView: View {
    @State private var inputMode = EncodingInputMode.text
    @State private var textInput = ""
    @State private var hexInput = ""
    @State private var source = EncodingSourceOption.automatic
    @State private var resolution: EncodingResolution?
    @State private var inputError: String?

    private let columns = [
        GridItem(.adaptive(minimum: 260), alignment: .top),
    ]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Encoding Inspector",
                    description: "Detect, validate, decode, and convert Unicode byte sequences."
                )

                inputControls
                inputEditor

                if let inputError {
                    Label(inputError, systemImage: "exclamationmark.triangle")
                        .foregroundStyle(.red)
                } else if let resolution {
                    DetailSectionView(
                        title: "Analysis",
                        rows: resolution.analysisRows,
                        linksCodepoints: false
                    )

                    if let decodedText = resolution.decodedText {
                        EncodingDecodedTextView(
                            text: decodedText,
                            codepoints: resolution.codepoints
                        )
                    }

                    if !resolution.conversions.isEmpty {
                        LazyVGrid(columns: columns, alignment: .leading, spacing: 12) {
                            ForEach(resolution.conversions) { conversion in
                                EncodingConversionView(conversion: conversion)
                            }
                        }
                    }

                    if let errorMessage = resolution.errorMessage {
                        ContentUnavailableView(
                            "Unable to Decode Input",
                            systemImage: "exclamationmark.triangle",
                            description: Text(errorMessage)
                        )
                        .frame(maxWidth: .infinity, minHeight: 200)
                    }
                } else {
                    ContentUnavailableView(
                        "Enter Text or Bytes",
                        systemImage: "memorychip",
                        description: Text(
                            "Try text such as “café 🙂” or hexadecimal bytes such as "
                                + "“63 61 66 C3 A9”."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 260)
                }
            }
            .frame(minWidth: 360, maxWidth: 900)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: inputMode) {
            changeInputMode()
        }
        .onChange(of: textInput) {
            analyze()
        }
        .onChange(of: hexInput) {
            analyze()
        }
        .onChange(of: source) {
            analyze()
        }
    }

    private var inputControls: some View {
        HStack(alignment: .firstTextBaseline, spacing: 16) {
            Picker("Input Mode", selection: $inputMode) {
                ForEach(EncodingInputMode.allCases) { mode in
                    Text(mode.rawValue)
                        .tag(mode)
                }
            }
            .pickerStyle(.segmented)
            .frame(maxWidth: 300)

            Spacer()

            if inputMode == .hex {
                Picker("Interpret As", selection: $source) {
                    ForEach(EncodingSourceOption.allCases) { option in
                        Text(option.rawValue)
                            .tag(option)
                    }
                }
                .pickerStyle(.menu)
                .fixedSize()
            } else {
                Text("UTF-8 source")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }

    @ViewBuilder
    private var inputEditor: some View {
        switch inputMode {
        case .text:
            ToolTextEditor(
                "UTF-8 Text",
                text: $textInput,
                minimumHeight: 120
            )
        case .hex:
            GroupBox {
                TextEditor(text: $hexInput)
                    .font(.body.monospaced())
                    .frame(minHeight: 120)
                    .accessibilityLabel("Hexadecimal bytes")
            } label: {
                HStack {
                    Text("Hexadecimal Bytes")
                        .font(.headline)

                    Spacer()

                    Text(hexByteSummary)
                        .font(.caption.monospacedDigit())
                        .foregroundStyle(.secondary)
                }
            }
        }
    }

    private var hexByteSummary: String {
        guard let byteCount = HexByteParser.parse(hexInput).bytes?.count else {
            return "Invalid"
        }

        return "\(byteCount) bytes"
    }

    private func changeInputMode() {
        switch inputMode {
        case .text:
            if let decodedText = resolution?.decodedText {
                textInput = decodedText
            }
        case .hex:
            hexInput = EncodingFormatting.hex(Array(textInput.utf8))
            source = .automatic
        }

        analyze()
    }

    private func analyze() {
        let bytes: [UInt8]
        let selectedSource: EncodingSourceOption

        switch inputMode {
        case .text:
            bytes = Array(textInput.utf8)
            selectedSource = .utf8
            inputError = nil
        case .hex:
            let parsed = HexByteParser.parse(hexInput)

            guard let parsedBytes = parsed.bytes else {
                resolution = nil
                inputError = parsed.errorMessage
                return
            }

            bytes = parsedBytes
            selectedSource = source
            inputError = nil
        }

        guard !bytes.isEmpty else {
            resolution = nil
            return
        }

        resolution = EncodingResolution.analyze(bytes, source: selectedSource)
    }
}

private struct EncodingDecodedTextView: View {
    let text: String
    let codepoints: String

    var body: some View {
        GroupBox {
            VStack(alignment: .leading, spacing: 10) {
                Text(text)
                    .font(.title3)
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, minHeight: 44, alignment: .topLeading)

                Divider()

                Text(codepoints)
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
                    .textSelection(.enabled)
            }
            .padding(.vertical, 4)
        } label: {
            Text("Decoded Text")
                .font(.headline)
        }
    }
}

private struct EncodingConversionView: View {
    let conversion: EncodingConversion

    var body: some View {
        GroupBox {
            if let bytes = conversion.bytes {
                Text(EncodingFormatting.hex(bytes))
                    .font(.body.monospaced())
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, minHeight: 68, alignment: .topLeading)
            } else {
                Label(
                    conversion.errorMessage ?? "Conversion failed.",
                    systemImage: "exclamationmark.triangle"
                )
                .foregroundStyle(.red)
                .frame(maxWidth: .infinity, minHeight: 68, alignment: .topLeading)
            }
        } label: {
            HStack {
                Text(conversion.target.rawValue)
                    .font(.headline)

                Spacer()

                Text(conversion.byteSummary)
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct EncodingResolution {
    let detectedEncoding: String
    let interpretedEncoding: String
    let isASCII: Bool
    let isUTF8: Bool
    let isUTF16: Bool
    let codepointCount: Int?
    let decodedText: String?
    let codepoints: String
    let conversions: [EncodingConversion]
    let errorMessage: String?

    var analysisRows: [DetailRow] {
        [
            DetailRow("Detected Encoding", detectedEncoding),
            DetailRow("Interpreted As", interpretedEncoding),
            DetailRow("Valid ASCII", MojibakeFormatting.yesOrNo(isASCII)),
            DetailRow("Valid UTF-8", MojibakeFormatting.yesOrNo(isUTF8)),
            DetailRow(
                "Valid UTF-16 (Either Endian)",
                MojibakeFormatting.yesOrNo(isUTF16)
            ),
            DetailRow(
                "Codepoints",
                codepointCount.map(String.init) ?? "Unavailable"
            ),
        ]
    }

    static func analyze(
        _ bytes: [UInt8],
        source: EncodingSourceOption
    ) -> EncodingResolution {
        let properties = inspect(bytes)
        let detected = properties.detected

        guard let inputEncoding = source.resolvedEncoding(detected: detected) else {
            return EncodingResolution(
                detectedEncoding: EncodingFormatting.name(detected: detected),
                interpretedEncoding: "Unresolved",
                isASCII: properties.isASCII,
                isUTF8: properties.isUTF8,
                isUTF16: properties.isUTF16,
                codepointCount: nil,
                decodedText: nil,
                codepoints: "Unavailable",
                conversions: [],
                errorMessage: "Select the encoding used by these bytes."
            )
        }

        if let errorMessage = validationError(
            for: inputEncoding,
            isASCII: properties.isASCII,
            isUTF8: properties.isUTF8
        ) {
            return EncodingResolution(
                detectedEncoding: EncodingFormatting.name(detected: detected),
                interpretedEncoding: source.resolvedName(detected: detected),
                isASCII: properties.isASCII,
                isUTF8: properties.isUTF8,
                isUTF16: properties.isUTF16,
                codepointCount: nil,
                decodedText: nil,
                codepoints: "Unavailable",
                conversions: [],
                errorMessage: errorMessage
            )
        }

        let utf8Conversion = EncodingConversion.convert(
            bytes,
            from: inputEncoding,
            to: .utf8
        )

        guard let utf8Bytes = utf8Conversion.bytes else {
            return EncodingResolution(
                detectedEncoding: EncodingFormatting.name(detected: detected),
                interpretedEncoding: source.resolvedName(detected: detected),
                isASCII: properties.isASCII,
                isUTF8: properties.isUTF8,
                isUTF16: properties.isUTF16,
                codepointCount: nil,
                decodedText: nil,
                codepoints: "Unavailable",
                conversions: [],
                errorMessage: utf8Conversion.errorMessage ?? "UTF-8 conversion failed."
            )
        }

        let conversions = EncodingConversionTarget.allCases.map { target in
            target == .utf8
                ? utf8Conversion
                : EncodingConversion.convert(bytes, from: inputEncoding, to: target)
        }
        let text = String(decoding: utf8Bytes, as: UTF8.self)
        let count = countCodepoints(bytes, encoding: inputEncoding)
        let codepoints = text.unicodeScalars.isEmpty
            ? "None"
            : text.unicodeScalars
                .map { MojibakeFormatting.codepoint($0.value) }
                .joined(separator: " ")

        return EncodingResolution(
            detectedEncoding: EncodingFormatting.name(detected: detected),
            interpretedEncoding: source.resolvedName(detected: detected),
            isASCII: properties.isASCII,
            isUTF8: properties.isUTF8,
            isUTF16: properties.isUTF16,
            codepointCount: count,
            decodedText: text,
            codepoints: codepoints,
            conversions: conversions,
            errorMessage: nil
        )
    }

    private static func validationError(
        for encoding: mjb_encoding,
        isASCII: Bool,
        isUTF8: Bool
    ) -> String? {
        if encoding.rawValue == MJB_ENC_ASCII.rawValue && !isASCII {
            return "The bytes contain values outside ASCII."
        }

        if encoding.rawValue == MJB_ENC_UTF_8.rawValue && !isUTF8 {
            return "The bytes are not a well-formed UTF-8 sequence."
        }

        return nil
    }

    private static func inspect(
        _ bytes: [UInt8]
    ) -> (detected: mjb_encoding, isASCII: Bool, isUTF8: Bool, isUTF16: Bool) {
        bytes.withUnsafeBytes { rawBuffer in
            let buffer = rawBuffer.bindMemory(to: CChar.self)
            return (
                mjb_detect_encoding(buffer.baseAddress, buffer.count),
                mjb_is_ascii(buffer.baseAddress, buffer.count),
                mjb_is_utf8(buffer.baseAddress, buffer.count),
                mjb_is_utf16(buffer.baseAddress, buffer.count)
            )
        }
    }

    private static func countCodepoints(
        _ bytes: [UInt8],
        encoding: mjb_encoding
    ) -> Int {
        bytes.withUnsafeBytes { rawBuffer in
            let buffer = rawBuffer.bindMemory(to: CChar.self)
            return mjb_count_codepoints(
                buffer.baseAddress,
                buffer.count,
                encoding
            )
        }
    }
}

private struct EncodingConversion: Identifiable {
    let target: EncodingConversionTarget
    let bytes: [UInt8]?
    let errorMessage: String?

    var id: EncodingConversionTarget {
        target
    }

    var byteSummary: String {
        bytes.map { "\($0.count) bytes" } ?? "Failed"
    }

    static func convert(
        _ input: [UInt8],
        from inputEncoding: mjb_encoding,
        to target: EncodingConversionTarget
    ) -> EncodingConversion {
        var status = MJB_STATUS_OK
        let output = MojibakeBytes.transform(input) { buffer, byteLength, result in
            status = mjb_convert_encoding(
                buffer,
                byteLength,
                inputEncoding,
                target.value,
                result
            )
            return status
        }

        return EncodingConversion(
            target: target,
            bytes: output,
            errorMessage: output == nil
                ? MojibakeFormatting.statusMessage(
                    operation: "mjb_convert_encoding",
                    status: status
                )
                : nil
        )
    }
}

private enum EncodingInputMode: String, CaseIterable, Identifiable {
    case text = "Text"
    case hex = "Hex Bytes"

    var id: Self {
        self
    }
}

private enum EncodingConversionTarget: String, CaseIterable, Identifiable {
    case utf8 = "UTF-8"
    case utf16LE = "UTF-16LE"
    case utf16BE = "UTF-16BE"
    case utf32LE = "UTF-32LE"
    case utf32BE = "UTF-32BE"

    var id: Self {
        self
    }

    var value: mjb_encoding {
        switch self {
        case .utf8:
            MJB_ENC_UTF_8
        case .utf16LE:
            MJB_ENC_UTF_16LE
        case .utf16BE:
            MJB_ENC_UTF_16BE
        case .utf32LE:
            MJB_ENC_UTF_32LE
        case .utf32BE:
            MJB_ENC_UTF_32BE
        }
    }
}

private enum EncodingSourceOption: String, CaseIterable, Identifiable {
    case automatic = "Automatic"
    case ascii = "ASCII"
    case utf8 = "UTF-8"
    case utf16 = "UTF-16 (BOM)"
    case utf16LE = "UTF-16LE"
    case utf16BE = "UTF-16BE"
    case utf32 = "UTF-32 (BOM)"
    case utf32LE = "UTF-32LE"
    case utf32BE = "UTF-32BE"

    var id: Self {
        self
    }

    func resolvedEncoding(detected: mjb_encoding) -> mjb_encoding? {
        switch self {
        case .automatic:
            return EncodingFormatting.concreteEncoding(from: detected)
        default:
            return value
        }
    }

    func resolvedName(detected: mjb_encoding) -> String {
        if self == .automatic {
            guard let resolved = resolvedEncoding(detected: detected) else {
                return "Unresolved"
            }

            return "\(EncodingFormatting.name(detected: resolved)) (automatic)"
        }

        return rawValue
    }

    private var value: mjb_encoding {
        switch self {
        case .automatic:
            MJB_ENC_UNKNOWN
        case .ascii:
            MJB_ENC_ASCII
        case .utf8:
            MJB_ENC_UTF_8
        case .utf16:
            MJB_ENC_UTF_16
        case .utf16LE:
            MJB_ENC_UTF_16LE
        case .utf16BE:
            MJB_ENC_UTF_16BE
        case .utf32:
            MJB_ENC_UTF_32
        case .utf32LE:
            MJB_ENC_UTF_32LE
        case .utf32BE:
            MJB_ENC_UTF_32BE
        }
    }
}

private enum EncodingFormatting {
    nonisolated static func concreteEncoding(from detected: mjb_encoding) -> mjb_encoding? {
        let rawValue = detected.rawValue

        if rawValue & MJB_ENC_UTF_32.rawValue != 0 {
            return detected
        }

        if rawValue & MJB_ENC_UTF_16.rawValue != 0 {
            return detected
        }

        if rawValue & MJB_ENC_UTF_8.rawValue != 0 {
            return MJB_ENC_UTF_8
        }

        if rawValue & MJB_ENC_ASCII.rawValue != 0 {
            return MJB_ENC_ASCII
        }

        return nil
    }

    nonisolated static func name(detected: mjb_encoding) -> String {
        let rawValue = detected.rawValue

        if rawValue & MJB_ENC_UTF_32.rawValue != 0 {
            if rawValue & MJB_ENC_UTF_32LE.rawValue != 0 {
                return "UTF-32LE (BOM)"
            }

            if rawValue & MJB_ENC_UTF_32BE.rawValue != 0 {
                return "UTF-32BE (BOM)"
            }
        }

        if rawValue & MJB_ENC_UTF_16.rawValue != 0 {
            if rawValue & MJB_ENC_UTF_16LE.rawValue != 0 {
                return "UTF-16LE (BOM)"
            }

            if rawValue & MJB_ENC_UTF_16BE.rawValue != 0 {
                return "UTF-16BE (BOM)"
            }
        }

        let isASCII = rawValue & MJB_ENC_ASCII.rawValue != 0
        let isUTF8 = rawValue & MJB_ENC_UTF_8.rawValue != 0

        if isASCII && isUTF8 {
            return "ASCII · UTF-8"
        }

        if isUTF8 {
            return "UTF-8"
        }

        if isASCII {
            return "ASCII"
        }

        return "Unknown"
    }

    nonisolated static func hex(_ bytes: [UInt8]) -> String {
        bytes
            .map { String(format: "%02X", $0) }
            .joined(separator: " ")
    }
}

private enum HexByteParser {
    nonisolated static func parse(
        _ input: String
    ) -> (bytes: [UInt8]?, errorMessage: String?) {
        let tokens = input.split {
            $0.isWhitespace || $0 == "," || $0 == ":"
        }

        guard !tokens.isEmpty else {
            return ([], nil)
        }

        if tokens.count == 1 {
            let token = stripPrefix(String(tokens[0]))

            if token.count > 2 {
                guard token.count.isMultiple(of: 2) else {
                    return (nil, "A continuous hexadecimal value must contain complete bytes.")
                }

                var bytes: [UInt8] = []
                var index = token.startIndex

                while index < token.endIndex {
                    let end = token.index(index, offsetBy: 2)
                    let pair = String(token[index..<end])

                    guard let byte = UInt8(pair, radix: 16) else {
                        return (nil, "“\(pair)” is not a hexadecimal byte.")
                    }

                    bytes.append(byte)
                    index = end
                }

                return (bytes, nil)
            }
        }

        var bytes: [UInt8] = []

        for rawToken in tokens {
            let token = stripPrefix(String(rawToken))

            guard token.count == 2, let byte = UInt8(token, radix: 16) else {
                return (nil, "“\(rawToken)” is not a two-digit hexadecimal byte.")
            }

            bytes.append(byte)
        }

        return (bytes, nil)
    }

    private nonisolated static func stripPrefix(_ token: String) -> String {
        token.lowercased().hasPrefix("0x") ? String(token.dropFirst(2)) : token
    }
}

#Preview {
    EncodingInspectorView()
}
