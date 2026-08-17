//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct TerminalWidthView: View {
    fileprivate enum WidthProfile: String, CaseIterable, Identifiable {
        case narrow = "Narrow"
        case eastAsian = "East Asian"

        var id: Self {
            self
        }

        var value: mjb_terminal_width_profile {
            switch self {
            case .narrow:
                MJB_TERMINAL_WIDTH_NARROW
            case .eastAsian:
                MJB_TERMINAL_WIDTH_EAST_ASIAN
            }
        }

        var explanation: String {
            switch self {
            case .narrow:
                "Ambiguous East Asian Width characters count as one column."
            case .eastAsian:
                "Ambiguous East Asian Width characters count as two columns."
            }
        }
    }

    @State private var input = ""
    @State private var selectedProfile = WidthProfile.narrow
    @State private var maxColumns = 10
    @State private var analysis: TerminalWidthAnalysis?
    @State private var errorMessage: String?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Terminal Width",
                    description: "Estimate terminal cells for printable, single-line text."
                )

                GroupBox {
                    TextField("Text to measure", text: $input)
                        .font(.body)
                        .textFieldStyle(.plain)
                        .padding(6)
                        .accessibilityLabel("Terminal width input text")
                } label: {
                    HStack {
                        Text("Input")
                            .font(.headline)

                        Spacer()

                        Text(inputSummary)
                            .font(.caption.monospacedDigit())
                            .foregroundStyle(.secondary)
                    }
                }

                VStack(alignment: .leading, spacing: 6) {
                    Picker("Terminal Profile", selection: $selectedProfile) {
                        ForEach(WidthProfile.allCases) { profile in
                            Text(profile.rawValue)
                        }
                    }
                    .pickerStyle(.segmented)

                    Text(selectedProfile.explanation)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                if let analysis {
                    WidthTotalsView(
                        totals: analysis.totals,
                        selectedProfile: selectedProfile
                    )

                    HStack {
                        Text("Truncation Budget")
                            .font(.headline)

                        Spacer()

                        Stepper(
                            "\(maxColumns) \(maxColumns == 1 ? "column" : "columns")",
                            value: $maxColumns,
                            in: 0 ... 200
                        )
                        .monospacedDigit()
                    }

                    WidthTruncationView(
                        previews: analysis.truncationPreviews,
                        profile: selectedProfile,
                        maxColumns: maxColumns
                    )

                    WidthClustersView(
                        clusters: analysis.clusters,
                        profile: selectedProfile
                    )
                } else if let errorMessage {
                    ContentUnavailableView(
                        "Terminal Width Failed",
                        systemImage: "exclamationmark.triangle",
                        description: Text(errorMessage)
                    )
                    .frame(maxWidth: .infinity, minHeight: 220)
                } else {
                    ContentUnavailableView(
                        "Enter Text to Measure",
                        systemImage: "ruler",
                        description: Text(
                            "Try ASCII, combining marks, CJK text, ambiguous symbols, or emoji."
                        )
                    )
                    .frame(maxWidth: .infinity, minHeight: 220)
                }
            }
            .frame(minWidth: 360, maxWidth: 900)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            measure()
        }
        .onChange(of: selectedProfile) {
            measure()
        }
        .onChange(of: maxColumns) {
            measure()
        }
    }

    private var inputSummary: String {
        MojibakeCounting.summary(for: input)
    }

    private func measure() {
        guard !input.isEmpty else {
            analysis = nil
            errorMessage = nil
            return
        }

        let resolution = TerminalWidthAnalysis.measure(
            input: input,
            profile: selectedProfile,
            maxColumns: maxColumns
        )
        analysis = resolution.analysis
        errorMessage = resolution.errorMessage
    }
}

private struct TerminalWidthResolution {
    let analysis: TerminalWidthAnalysis?
    let errorMessage: String?

    static func success(_ analysis: TerminalWidthAnalysis) -> Self {
        Self(analysis: analysis, errorMessage: nil)
    }

    static func failure(_ message: String) -> Self {
        Self(analysis: nil, errorMessage: message)
    }
}

private struct TerminalWidthAnalysis {
    let totals: [TerminalWidthView.WidthProfile: Int]
    let clusters: [TerminalWidthCluster]
    let truncationPreviews: [TerminalWidthTruncation]

    static func measure(
        input: String,
        profile: TerminalWidthView.WidthProfile,
        maxColumns: Int
    ) -> TerminalWidthResolution {
        let utf8 = input.utf8CString

        return utf8.withUnsafeBufferPointer { buffer in
            let byteLength = buffer.count - 1
            var totals: [TerminalWidthView.WidthProfile: Int] = [:]

            for candidate in TerminalWidthView.WidthProfile.allCases {
                let widthResult = terminalWidth(
                    buffer: buffer.baseAddress,
                    byteLength: byteLength,
                    profile: candidate.value
                )

                guard let width = widthResult.width else {
                    return .failure(widthResult.errorMessage ?? "Terminal width failed.")
                }

                totals[candidate] = width
            }

            let clusterResult = measureClusters(
                buffer: buffer,
                byteLength: byteLength,
                profile: profile.value
            )

            guard let clusters = clusterResult.clusters else {
                return .failure(clusterResult.errorMessage ?? "Grapheme analysis failed.")
            }

            let graphemeBytes = mjb_truncate_grapheme_width(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8,
                profile.value,
                maxColumns
            )
            let wordBytes = mjb_truncate_word_width(
                buffer.baseAddress,
                byteLength,
                MJB_ENC_UTF_8,
                profile.value,
                maxColumns
            )
            let previews = [
                makeTruncation(
                    title: "Grapheme-Safe",
                    functionName: "mjb_truncate_grapheme_width",
                    buffer: buffer,
                    byteCount: graphemeBytes,
                    profile: profile.value
                ),
                makeTruncation(
                    title: "Word-Safe",
                    functionName: "mjb_truncate_word_width",
                    buffer: buffer,
                    byteCount: wordBytes,
                    profile: profile.value
                ),
            ]

            return .success(
                TerminalWidthAnalysis(
                    totals: totals,
                    clusters: clusters,
                    truncationPreviews: previews
                )
            )
        }
    }

    private static func measureClusters(
        buffer: UnsafeBufferPointer<CChar>,
        byteLength: Int,
        profile: mjb_terminal_width_profile
    ) -> (clusters: [TerminalWidthCluster]?, errorMessage: String?) {
        var clusters: [TerminalWidthCluster] = []
        var byteStart = 0

        while byteStart < byteLength {
            let remainingBytes = byteLength - byteStart
            let clusterLength = mjb_truncate_grapheme(
                buffer.baseAddress?.advanced(by: byteStart),
                remainingBytes,
                MJB_ENC_UTF_8,
                1
            )
            let byteEnd = byteStart + clusterLength

            guard byteEnd > byteStart, byteEnd <= byteLength else {
                return (nil, "mjb_truncate_grapheme returned an invalid byte boundary.")
            }

            let value = string(from: buffer, range: byteStart ..< byteEnd)
            let clusterBytes = Array(buffer[byteStart ..< byteEnd])
            let widthResult = clusterBytes.withUnsafeBufferPointer {
                terminalWidth(
                    buffer: $0.baseAddress,
                    byteLength: $0.count,
                    profile: profile
                )
            }

            guard let width = widthResult.width else {
                return (nil, widthResult.errorMessage)
            }

            clusters.append(
                TerminalWidthCluster(
                    id: clusters.count,
                    value: value,
                    byteStart: byteStart,
                    byteEnd: byteEnd,
                    width: width
                )
            )

            byteStart = byteEnd
        }

        return (clusters, nil)
    }

    private static func makeTruncation(
        title: String,
        functionName: String,
        buffer: UnsafeBufferPointer<CChar>,
        byteCount: Int,
        profile: mjb_terminal_width_profile
    ) -> TerminalWidthTruncation {
        let value = string(from: buffer, range: 0 ..< byteCount)
        let bytes = Array(buffer.prefix(byteCount))
        let width = bytes.withUnsafeBufferPointer {
            terminalWidth(
                buffer: $0.baseAddress,
                byteLength: $0.count,
                profile: profile
            ).width ?? 0
        }

        return TerminalWidthTruncation(
            title: title,
            functionName: functionName,
            value: value,
            byteCount: byteCount,
            width: width
        )
    }

    private static func terminalWidth(
        buffer: UnsafePointer<CChar>?,
        byteLength: Int,
        profile: mjb_terminal_width_profile
    ) -> (width: Int?, errorMessage: String?) {
        var width = 0
        let status = mjb_terminal_width(
            buffer,
            byteLength,
            MJB_ENC_UTF_8,
            profile,
            &width
        )

        guard status == MJB_STATUS_OK else {
            return (
                nil,
                MojibakeFormatting.statusMessage(
                    operation: "mjb_terminal_width",
                    status: status
                )
            )
        }

        return (width, nil)
    }

    private static func string(
        from buffer: UnsafeBufferPointer<CChar>,
        range: Range<Int>
    ) -> String {
        let bytes = buffer[range].map { UInt8(bitPattern: $0) }
        return String(decoding: bytes, as: UTF8.self)
    }
}

private struct TerminalWidthCluster: Identifiable {
    let id: Int
    let value: String
    let byteStart: Int
    let byteEnd: Int
    let width: Int

    var displayValue: String {
        TerminalWidthFormatting.displayValue(value)
    }

    var codepoints: String {
        value.unicodeScalars
            .map { MojibakeFormatting.codepoint($0.value) }
            .joined(separator: " ")
    }

    var eastAsianWidths: String {
        value.unicodeScalars
            .map { TerminalWidthFormatting.eastAsianWidth(mjb_codepoint($0.value)) }
            .joined(separator: " · ")
    }
}

private struct TerminalWidthTruncation: Identifiable {
    let title: String
    let functionName: String
    let value: String
    let byteCount: Int
    let width: Int

    var id: String {
        functionName
    }
}

private struct WidthTotalsView: View {
    let totals: [TerminalWidthView.WidthProfile: Int]
    let selectedProfile: TerminalWidthView.WidthProfile

    private let columns = [
        GridItem(.adaptive(minimum: 150), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, spacing: 8) {
                ForEach(TerminalWidthView.WidthProfile.allCases) { profile in
                    VStack(spacing: 4) {
                        Text(String(totals[profile] ?? 0))
                            .font(.title2.monospacedDigit())

                        Text(profile.rawValue)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity)
                    .padding(8)
                    .background(
                        profile == selectedProfile
                            ? Color.accentColor.opacity(0.12)
                            : Color.clear,
                        in: RoundedRectangle(cornerRadius: 7)
                    )
                    .overlay {
                        RoundedRectangle(cornerRadius: 7)
                            .stroke(
                                profile == selectedProfile
                                    ? Color.accentColor.opacity(0.4)
                                    : Color.secondary.opacity(0.2)
                            )
                    }
                }
            }
        } label: {
            HStack {
                Text("Total Columns")
                    .font(.headline)

                Spacer()

                Text("mjb_terminal_width")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct WidthTruncationView: View {
    let previews: [TerminalWidthTruncation]
    let profile: TerminalWidthView.WidthProfile
    let maxColumns: Int

    private let columns = [
        GridItem(.adaptive(minimum: 260), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(previews) { preview in
                    VStack(alignment: .leading, spacing: 6) {
                        HStack {
                            Text(preview.title)
                                .font(.headline)

                            Spacer()

                            Text("\(preview.width)/\(maxColumns) columns")
                                .font(.caption.monospacedDigit())
                                .foregroundStyle(.secondary)
                        }

                        Text(preview.value.isEmpty ? "Empty result" : preview.value)
                            .font(.body)
                            .textSelection(.enabled)
                            .frame(maxWidth: .infinity, minHeight: 38, alignment: .leading)
                            .padding(6)
                            .background(.quaternary, in: RoundedRectangle(cornerRadius: 5))

                        Text("\(preview.byteCount) bytes · \(preview.functionName)")
                            .font(.caption.monospaced())
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                    .overlay {
                        RoundedRectangle(cornerRadius: 7)
                            .stroke(.secondary.opacity(0.2))
                    }
                }
            }
        } label: {
            HStack {
                Text("Truncated Output")
                    .font(.headline)

                Spacer()

                Text(profile.rawValue)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct WidthClustersView: View {
    let clusters: [TerminalWidthCluster]
    let profile: TerminalWidthView.WidthProfile

    private let columns = [
        GridItem(.adaptive(minimum: 170), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(clusters) { cluster in
                    TerminalWidthClusterCard(cluster: cluster)
                }
            }
        } label: {
            HStack {
                Text("Grapheme Clusters")
                    .font(.headline)

                Spacer()

                Text(profile.rawValue)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct TerminalWidthClusterCard: View {
    let cluster: TerminalWidthCluster

    var body: some View {
        VStack(alignment: .leading, spacing: 5) {
            HStack(alignment: .firstTextBaseline) {
                Text(cluster.displayValue)
                    .font(.title2)
                    .lineLimit(1)
                    .minimumScaleFactor(0.6)

                Spacer()

                Text(widthLabel)
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(widthColor)
            }

            Text(cluster.codepoints)
                .font(.caption.monospaced())
                .lineLimit(1)

            Text("EAW \(cluster.eastAsianWidths)")
                .font(.caption)
                .foregroundStyle(.secondary)

            Text("Bytes \(cluster.byteStart)..<\(cluster.byteEnd)")
                .font(.caption.monospacedDigit())
                .foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(8)
        .background(widthColor.opacity(0.1), in: RoundedRectangle(cornerRadius: 7))
        .overlay {
            RoundedRectangle(cornerRadius: 7)
                .stroke(widthColor.opacity(0.3))
        }
        .help(cluster.codepoints)
    }

    private var widthLabel: String {
        "\(cluster.width) \(cluster.width == 1 ? "column" : "columns")"
    }

    private var widthColor: Color {
        switch cluster.width {
        case 0:
            .secondary
        case 1:
            .blue
        default:
            .purple
        }
    }
}

private enum TerminalWidthFormatting {
    static func displayValue(_ value: String) -> String {
        switch value {
        case "\t":
            return "⇥"
        case "\n":
            return "↵"
        case "\r":
            return "␍"
        case " ":
            return "␠"
        default:
            let scalars = Array(value.unicodeScalars)

            if scalars.allSatisfy({ !mjb_codepoint_is_graphic($0.value) }) {
                return scalars.map { invisibleName($0.value) }.joined(separator: " + ")
            }

            if scalars.allSatisfy({ mjb_codepoint_is_combining($0.value) }) {
                return "◌\(value)"
            }

            return value
        }
    }

    static func eastAsianWidth(_ codepoint: mjb_codepoint) -> String {
        var width = MJB_EAW_NOT_SET

        guard mjb_codepoint_east_asian_width(codepoint, &width) == MJB_STATUS_OK else {
            return "?"
        }

        switch width {
        case MJB_EAW_AMBIGUOUS:
            return "A"
        case MJB_EAW_FULL_WIDTH:
            return "F"
        case MJB_EAW_HALF_WIDTH:
            return "H"
        case MJB_EAW_NEUTRAL:
            return "N"
        case MJB_EAW_NARROW:
            return "Na"
        case MJB_EAW_WIDE:
            return "W"
        default:
            return "?"
        }
    }

    private static func invisibleName(_ codepoint: mjb_codepoint) -> String {
        switch codepoint {
        case 0x200B:
            "ZWSP"
        case 0x200C:
            "ZWNJ"
        case 0x200D:
            "ZWJ"
        case 0xFE00 ... 0xFE0F:
            "VS\(codepoint - 0xFE00 + 1)"
        case 0xE0100 ... 0xE01EF:
            "VS\(codepoint - 0xE0100 + 17)"
        default:
            MojibakeFormatting.codepoint(codepoint)
        }
    }
}

#Preview {
    TerminalWidthView()
}
