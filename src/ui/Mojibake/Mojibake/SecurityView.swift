//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct SecurityView: View {
    @State private var firstInput = ""
    @State private var secondInput = ""
    @State private var resolution = SecurityResolution.empty

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Security",
                    description: "Inspect UTS #39 confusables and resolved script sets."
                )

                ViewThatFits(in: .horizontal) {
                    HStack(alignment: .top, spacing: 12) {
                        inputEditors
                    }

                    VStack(spacing: 12) {
                        inputEditors
                    }
                }

                if let comparison = resolution.comparison {
                    SecurityComparisonView(isConfusable: comparison)
                }

                if let first = resolution.first {
                    DetailSectionView(
                        title: "First Text Analysis",
                        rows: first.rows,
                        linksCodepoints: false
                    )
                }

                if let second = resolution.second {
                    DetailSectionView(
                        title: "Comparison Text Analysis",
                        rows: second.rows,
                        linksCodepoints: false
                    )
                }

                if let errorMessage = resolution.errorMessage {
                    ContentUnavailableView(
                        "Security Analysis Failed",
                        systemImage: "exclamationmark.triangle",
                        description: Text(errorMessage)
                    )
                    .frame(maxWidth: .infinity, minHeight: 220)
                } else if resolution.isEmpty {
                    ContentUnavailableView(
                        "Enter Text to Analyze",
                        systemImage: "lock.shield",
                        description: Text(
                            "Compare text such as “hello” with “hеllo”, "
                                + "where the second word contains a Cyrillic е."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 220)
                }
            }
            .frame(minWidth: 360, maxWidth: 900)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: firstInput) {
            analyze()
        }
        .onChange(of: secondInput) {
            analyze()
        }
    }

    private func analyze() {
        resolution = SecurityResolution.analyze(
            first: firstInput,
            second: secondInput
        )
    }

    @ViewBuilder
    private var inputEditors: some View {
        ToolTextEditor("First Text", text: $firstInput)
        ToolTextEditor("Comparison Text", text: $secondInput)
    }
}

private struct SecurityComparisonView: View {
    let isConfusable: Bool

    var body: some View {
        GroupBox {
            HStack(alignment: .top, spacing: 12) {
                Image(
                    systemName: isConfusable
                        ? "exclamationmark.triangle.fill"
                        : "checkmark.shield.fill"
                )
                .font(.title2)
                .foregroundStyle(isConfusable ? .orange : .green)

                VStack(alignment: .leading, spacing: 4) {
                    Text(
                        isConfusable
                            ? "Visually Confusable"
                            : "Not Visually Confusable"
                    )
                    .font(.headline)

                    Text(
                        isConfusable
                            ? "The texts share the same UTS #39 confusable skeleton."
                            : "The texts have different UTS #39 confusable skeletons."
                    )
                    .foregroundStyle(.secondary)
                }

                Spacer()
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, 4)
        } label: {
            Text("Comparison")
                .font(.headline)
        }
    }
}

private struct SecurityResolution {
    let first: SecurityTextAnalysis?
    let second: SecurityTextAnalysis?
    let comparison: Bool?
    let errorMessage: String?

    static let empty = SecurityResolution(
        first: nil,
        second: nil,
        comparison: nil,
        errorMessage: nil
    )

    var isEmpty: Bool {
        first == nil && second == nil
    }

    static func analyze(first: String, second: String) -> SecurityResolution {
        var firstAnalysis: SecurityTextAnalysis?
        var secondAnalysis: SecurityTextAnalysis?

        if !first.isEmpty {
            let result = SecurityTextAnalysis.analyze(first)

            guard let analysis = result.analysis else {
                return SecurityResolution(
                    first: nil,
                    second: nil,
                    comparison: nil,
                    errorMessage: result.errorMessage
                )
            }

            firstAnalysis = analysis
        }

        if !second.isEmpty {
            let result = SecurityTextAnalysis.analyze(second)

            guard let analysis = result.analysis else {
                return SecurityResolution(
                    first: firstAnalysis,
                    second: nil,
                    comparison: nil,
                    errorMessage: result.errorMessage
                )
            }

            secondAnalysis = analysis
        }

        var comparison: Bool?

        if !first.isEmpty && !second.isEmpty {
            let result = compare(first, with: second)

            guard let isConfusable = result.isConfusable else {
                return SecurityResolution(
                    first: firstAnalysis,
                    second: secondAnalysis,
                    comparison: nil,
                    errorMessage: result.errorMessage
                )
            }

            comparison = isConfusable
        }

        return SecurityResolution(
            first: firstAnalysis,
            second: secondAnalysis,
            comparison: comparison,
            errorMessage: nil
        )
    }

    private static func compare(
        _ first: String,
        with second: String
    ) -> (isConfusable: Bool?, errorMessage: String?) {
        let firstUTF8 = first.utf8CString
        let secondUTF8 = second.utf8CString

        return firstUTF8.withUnsafeBufferPointer { firstBuffer in
            secondUTF8.withUnsafeBufferPointer { secondBuffer in
                var isConfusable = false
                let status = mjb_confusable_match(
                    firstBuffer.baseAddress,
                    firstBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    secondBuffer.baseAddress,
                    secondBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    &isConfusable
                )

                guard status == MJB_STATUS_OK else {
                    return (
                        nil,
                        MojibakeFormatting.statusMessage(
                            operation: "mjb_confusable_match",
                            status: status
                        )
                    )
                }

                return (isConfusable, nil)
            }
        }
    }
}

private struct SecurityTextAnalysis {
    let skeleton: String
    let isDefaultIdentifier: Bool
    let isNFKCIdentifier: Bool
    let scriptSet: SecurityScriptSet

    var rows: [DetailRow] {
        [
            DetailRow("Confusable Skeleton", skeleton),
            DetailRow(
                "Skeleton Codepoints",
                skeletonCodepoints,
                monospaced: true
            ),
            DetailRow(
                "Default Identifier",
                MojibakeFormatting.yesOrNo(isDefaultIdentifier)
            ),
            DetailRow(
                "NFKC Identifier",
                MojibakeFormatting.yesOrNo(isNFKCIdentifier)
            ),
            DetailRow("Resolved Script Set", scriptSet.summary),
            DetailRow("Resolved Scripts", scriptSet.scriptsDescription),
        ]
    }

    private var skeletonCodepoints: String {
        if skeleton.isEmpty {
            return "None"
        }

        return skeleton.unicodeScalars
            .map { MojibakeFormatting.codepoint($0.value) }
            .joined(separator: " ")
    }

    static func analyze(
        _ input: String
    ) -> (analysis: SecurityTextAnalysis?, errorMessage: String?) {
        var skeletonStatus = MJB_STATUS_OK
        let skeleton = MojibakeString.transform(input) { buffer, byteLength, result in
            skeletonStatus = mjb_confusable_skeleton(
                buffer,
                byteLength,
                MJB_ENC_UTF_8,
                MJB_ENC_UTF_8,
                result
            )
            return skeletonStatus
        }

        guard let skeleton else {
            return (
                nil,
                MojibakeFormatting.statusMessage(
                    operation: "mjb_confusable_skeleton",
                    status: skeletonStatus
                )
            )
        }

        let identifierResults = input.utf8CString.withUnsafeBufferPointer { buffer in
            let byteLength = buffer.count - 1
            return (
                mjb_is_identifier(
                    buffer.baseAddress,
                    byteLength,
                    MJB_ENC_UTF_8,
                    MJB_IDENTIFIER_DEFAULT
                ),
                mjb_is_identifier(
                    buffer.baseAddress,
                    byteLength,
                    MJB_ENC_UTF_8,
                    MJB_IDENTIFIER_NFKC
                )
            )
        }

        let scriptResult = SecurityScriptSet.resolve(input)

        guard let scriptSet = scriptResult.scriptSet else {
            return (nil, scriptResult.errorMessage)
        }

        return (
            SecurityTextAnalysis(
                skeleton: skeleton,
                isDefaultIdentifier: identifierResults.0,
                isNFKCIdentifier: identifierResults.1,
                scriptSet: scriptSet
            ),
            nil
        )
    }
}

private struct SecurityScriptSet {
    let kind: mjb_script_set_kind
    let scripts: [mjb_script]

    var summary: String {
        switch kind {
        case MJB_SCRIPT_SET_EMPTY:
            "Empty · mixed-script"
        case MJB_SCRIPT_SET_ALL:
            "All · Common or Inherited only"
        case MJB_SCRIPT_SET_RESOLVED:
            "\(scripts.count) resolved \(scripts.count == 1 ? "script" : "scripts")"
        default:
            "Unknown"
        }
    }

    var scriptsDescription: String {
        guard kind == MJB_SCRIPT_SET_RESOLVED else {
            return kind == MJB_SCRIPT_SET_ALL ? "All" : "None"
        }

        return scripts
            .map(SecurityScriptFormatting.name)
            .joined(separator: " · ")
    }

    static func resolve(
        _ input: String
    ) -> (scriptSet: SecurityScriptSet?, errorMessage: String?) {
        let utf8 = input.utf8CString

        return utf8.withUnsafeBufferPointer { buffer in
            let byteLength = buffer.count - 1
            var count = 0
            var kind = MJB_SCRIPT_SET_EMPTY
            var status = mjb_resolved_script_set(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8,
                nil,
                &count,
                &kind
            )

            guard status == MJB_STATUS_OK else {
                return (
                    nil,
                    MojibakeFormatting.statusMessage(
                        operation: "mjb_resolved_script_set",
                        status: status
                    )
                )
            }

            guard count > 0 else {
                return (SecurityScriptSet(kind: kind, scripts: []), nil)
            }

            var scripts = [mjb_script](repeating: MJB_SC_NOT_SET, count: count)
            var outputCount = scripts.count

            status = scripts.withUnsafeMutableBufferPointer { scriptsBuffer in
                mjb_resolved_script_set(
                    buffer.baseAddress,
                    byteLength,
                    MJB_ENC_UTF_8,
                    scriptsBuffer.baseAddress,
                    &outputCount,
                    &kind
                )
            }

            guard status == MJB_STATUS_OK else {
                return (
                    nil,
                    MojibakeFormatting.statusMessage(
                        operation: "mjb_resolved_script_set",
                        status: status
                    )
                )
            }

            return (
                SecurityScriptSet(
                    kind: kind,
                    scripts: Array(scripts.prefix(outputCount))
                ),
                nil
            )
        }
    }
}

private enum SecurityScriptFormatting {
    nonisolated static func name(_ script: mjb_script) -> String {
        let name: String

        switch script {
        case MJB_SC_ARAB:
            name = "Arabic"
        case MJB_SC_ARMN:
            name = "Armenian"
        case MJB_SC_BENG:
            name = "Bengali"
        case MJB_SC_BOPO:
            name = "Bopomofo"
        case MJB_SC_CYRL:
            name = "Cyrillic"
        case MJB_SC_DEVA:
            name = "Devanagari"
        case MJB_SC_ETHI:
            name = "Ethiopic"
        case MJB_SC_GEOR:
            name = "Georgian"
        case MJB_SC_GREK:
            name = "Greek"
        case MJB_SC_GUJR:
            name = "Gujarati"
        case MJB_SC_GURU:
            name = "Gurmukhi"
        case MJB_SC_HANG:
            name = "Hangul"
        case MJB_SC_HANI:
            name = "Han"
        case MJB_SC_HEBR:
            name = "Hebrew"
        case MJB_SC_HIRA:
            name = "Hiragana"
        case MJB_SC_KANA:
            name = "Katakana"
        case MJB_SC_LATN:
            name = "Latin"
        case MJB_SC_TAML:
            name = "Tamil"
        case MJB_SC_TELU:
            name = "Telugu"
        case MJB_SC_THAI:
            name = "Thai"
        case MJB_SC_TIBT:
            name = "Tibetan"
        case MJB_SC_ZINH:
            name = "Inherited"
        case MJB_SC_ZYYY:
            name = "Common"
        case MJB_SC_ZZZZ:
            name = "Unknown"
        case MJB_SC_HANB:
            name = "Han with Bopomofo"
        case MJB_SC_JPAN:
            name = "Japanese"
        case MJB_SC_KORE:
            name = "Korean"
        default:
            name = "Script"
        }

        return "\(name) [\(script.rawValue)]"
    }
}

#Preview {
    SecurityView()
}
