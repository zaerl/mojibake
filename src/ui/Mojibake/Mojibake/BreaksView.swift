//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct BreaksView: View {
    fileprivate enum OutputStyle: String, CaseIterable, Identifiable {
        case scalars = "Scalars"
        case noBreakBlocks = "No-Break Blocks"

        var id: Self {
            self
        }
    }

    fileprivate enum BreakMode: String, CaseIterable, Identifiable {
        case grapheme = "Grapheme Clusters"
        case word = "Words"
        case line = "Lines"
        case sentence = "Sentences"

        var id: Self {
            self
        }

        var functionName: String {
            switch self {
            case .grapheme:
                "mjb_next_grapheme_break"
            case .word:
                "mjb_next_word_break"
            case .line:
                "mjb_next_line_break"
            case .sentence:
                "mjb_next_sentence_break"
            }
        }

        var leadingBoundary: BreakBoundary {
            self == .line ? .noBreak : .allowed
        }
    }

    fileprivate enum BreakBoundary {
        case mandatory
        case noBreak
        case allowed

        init?(_ value: mjb_break_type) {
            switch value {
            case MJB_BT_MANDATORY:
                self = .mandatory
            case MJB_BT_NO_BREAK:
                self = .noBreak
            case MJB_BT_ALLOWED:
                self = .allowed
            default:
                return nil
            }
        }

        var symbol: String {
            switch self {
            case .mandatory:
                "!"
            case .noBreak:
                "×"
            case .allowed:
                "÷"
            }
        }

        var name: String {
            switch self {
            case .mandatory:
                "Mandatory break"
            case .noBreak:
                "No break"
            case .allowed:
                "Break allowed"
            }
        }

        var color: Color {
            switch self {
            case .mandatory:
                .orange
            case .noBreak:
                .secondary
            case .allowed:
                .green
            }
        }

        var isBreak: Bool {
            self != .noBreak
        }
    }

    @State private var input = ""
    @State private var outputStyle = OutputStyle.scalars
    @State private var results: [BreakMode: [BreakBoundary]] = [:]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Breaks",
                    description: "Inspect Unicode grapheme, word, line, and sentence boundaries."
                )

                GroupBox {
                    TextEditor(text: $input)
                        .font(.body)
                        .frame(minHeight: 140)
                        .accessibilityLabel("Input text")
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

                HStack {
                    Text("Output")
                        .font(.headline)

                    Spacer()

                    Picker("Output", selection: $outputStyle) {
                        ForEach(OutputStyle.allCases) { style in
                            Text(style.rawValue)
                        }
                    }
                    .labelsHidden()
                    .pickerStyle(.segmented)
                    .frame(maxWidth: 320)
                }

                HStack(spacing: 20) {
                    BoundaryLegend(boundary: .allowed)
                    BoundaryLegend(boundary: .noBreak)
                    BoundaryLegend(boundary: .mandatory)
                }
                .frame(maxWidth: .infinity)

                ForEach(BreakMode.allCases) { mode in
                    BreakResultView(
                        mode: mode,
                        input: input,
                        boundaries: results[mode] ?? [],
                        outputStyle: outputStyle
                    )
                }
            }
            .frame(minWidth: 360, maxWidth: 900)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            analyze()
        }
    }

    private var inputSummary: String {
        let scalarCount = input.unicodeScalars.count
        let scalarLabel = scalarCount == 1 ? "scalar" : "scalars"
        return "\(input.utf8.count) bytes · \(scalarCount) \(scalarLabel)"
    }

    private func analyze() {
        let utf8 = input.utf8CString

        results = utf8.withUnsafeBufferPointer { buffer in
            let byteLength = buffer.count - 1

            return [
                .grapheme: graphemeBreaks(buffer.baseAddress, byteLength: byteLength),
                .word: wordBreaks(buffer.baseAddress, byteLength: byteLength),
                .line: lineBreaks(buffer.baseAddress, byteLength: byteLength),
                .sentence: sentenceBreaks(buffer.baseAddress, byteLength: byteLength),
            ]
        }
    }

    private func graphemeBreaks(
        _ buffer: UnsafePointer<CChar>?,
        byteLength: Int
    ) -> [BreakBoundary] {
        var state = mjb_next_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_next_grapheme_break(buffer, byteLength, MJB_ENC_UTF_8, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func wordBreaks(
        _ buffer: UnsafePointer<CChar>?,
        byteLength: Int
    ) -> [BreakBoundary] {
        var state = mjb_next_word_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_next_word_break(buffer, byteLength, MJB_ENC_UTF_8, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func lineBreaks(
        _ buffer: UnsafePointer<CChar>?,
        byteLength: Int
    ) -> [BreakBoundary] {
        var state = mjb_next_line_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_next_line_break(buffer, byteLength, MJB_ENC_UTF_8, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func sentenceBreaks(
        _ buffer: UnsafePointer<CChar>?,
        byteLength: Int
    ) -> [BreakBoundary] {
        var state = mjb_next_sentence_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_next_sentence_break(buffer, byteLength, MJB_ENC_UTF_8, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }
}

private struct BreakResultView: View {
    let mode: BreaksView.BreakMode
    let input: String
    let boundaries: [BreaksView.BreakBoundary]
    let outputStyle: BreaksView.OutputStyle

    private var scalars: [Unicode.Scalar] {
        Array(input.unicodeScalars)
    }

    private var breakCount: Int {
        boundaries.filter(\.isBreak).count
    }

    var body: some View {
        GroupBox {
            if input.isEmpty {
                Text("Enter text to inspect its boundaries.")
                    .foregroundStyle(.secondary)
                    .frame(maxWidth: .infinity, minHeight: 56)
            } else {
                output
                    .padding(.vertical, 6)

                if boundaries.count != scalars.count {
                    Label(
                        "Boundary analysis did not cover every Unicode scalar.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.red)
                }
            }
        } label: {
            HStack {
                Text(mode.rawValue)
                    .font(.headline)

                Spacer()

                if !input.isEmpty {
                    Text("\(breakCount) breaks")
                        .font(.caption.monospacedDigit())
                        .foregroundStyle(.secondary)
                }

                Text(mode.functionName)
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
        .frame(maxWidth: .infinity)
    }

    @ViewBuilder
    private var output: some View {
        switch outputStyle {
        case .scalars:
            BoundaryFlowLayout(spacing: 8) {
                ForEach(scalars.indices, id: \.self) { index in
                    BoundaryScalarToken(
                        leadingBoundary: boundary(before: index),
                        scalar: scalars[index],
                        trailingBoundary: trailingBoundary(after: index)
                    )
                }
            }
        case .noBreakBlocks:
            BoundaryFlowLayout(spacing: 8) {
                ForEach(noBreakBlocks) { block in
                    BoundaryBlockToken(
                        leadingBoundary: block.leadingBoundary,
                        scalars: Array(scalars[block.range]),
                        trailingBoundary: block.trailingBoundary
                    )
                }
            }
        }
    }

    private var noBreakBlocks: [NoBreakBlock] {
        guard let lastIndex = scalars.indices.last else {
            return []
        }

        var blocks: [NoBreakBlock] = []
        var startIndex = scalars.startIndex
        var leadingBoundary = mode.leadingBoundary

        for index in scalars.indices {
            let trailingBoundary = boundary(after: index)

            if index == lastIndex {
                blocks.append(
                    NoBreakBlock(
                        range: startIndex ..< scalars.endIndex,
                        leadingBoundary: leadingBoundary,
                        trailingBoundary: trailingBoundary
                    )
                )
            } else if trailingBoundary?.isBreak == true {
                blocks.append(
                    NoBreakBlock(
                        range: startIndex ..< index + 1,
                        leadingBoundary: leadingBoundary,
                        trailingBoundary: nil
                    )
                )
                startIndex = index + 1
                leadingBoundary = trailingBoundary ?? .allowed
            }
        }

        return blocks
    }

    private func boundary(before index: Int) -> BreaksView.BreakBoundary? {
        if index == scalars.startIndex {
            return mode.leadingBoundary
        }

        let boundaryIndex = index - 1
        return boundaries.indices.contains(boundaryIndex) ? boundaries[boundaryIndex] : nil
    }

    private func trailingBoundary(after index: Int) -> BreaksView.BreakBoundary? {
        guard index == scalars.index(before: scalars.endIndex) else {
            return nil
        }

        return boundaries.indices.contains(index) ? boundaries[index] : nil
    }

    private func boundary(after index: Int) -> BreaksView.BreakBoundary? {
        boundaries.indices.contains(index) ? boundaries[index] : nil
    }
}

private struct NoBreakBlock: Identifiable {
    let range: Range<Int>
    let leadingBoundary: BreaksView.BreakBoundary
    let trailingBoundary: BreaksView.BreakBoundary?

    var id: Int {
        range.lowerBound
    }
}

private struct BoundaryScalarToken: View {
    let leadingBoundary: BreaksView.BreakBoundary?
    let scalar: Unicode.Scalar
    let trailingBoundary: BreaksView.BreakBoundary?

    var body: some View {
        HStack(spacing: 6) {
            if let leadingBoundary {
                BoundarySymbol(boundary: leadingBoundary)
            }

            ScalarToken(scalar: scalar)

            if let trailingBoundary {
                BoundarySymbol(boundary: trailingBoundary)
            }
        }
        .fixedSize()
    }
}

private struct BoundaryBlockToken: View {
    let leadingBoundary: BreaksView.BreakBoundary
    let scalars: [Unicode.Scalar]
    let trailingBoundary: BreaksView.BreakBoundary?

    var body: some View {
        HStack(spacing: 6) {
            BoundarySymbol(boundary: leadingBoundary)

            HStack(spacing: 4) {
                ForEach(scalars.indices, id: \.self) { index in
                    ScalarToken(scalar: scalars[index], showsBackground: false)
                }
            }
            .padding(.horizontal, 6)
            .padding(.vertical, 3)
            .background(.quaternary, in: RoundedRectangle(cornerRadius: 7))
            .overlay {
                RoundedRectangle(cornerRadius: 7)
                    .stroke(.secondary.opacity(0.2))
            }

            if let trailingBoundary {
                BoundarySymbol(boundary: trailingBoundary)
            }
        }
        .fixedSize()
    }
}

private struct BoundaryFlowLayout: Layout {
    let spacing: CGFloat

    func sizeThatFits(
        proposal: ProposedViewSize,
        subviews: Subviews,
        cache: inout ()
    ) -> CGSize {
        layout(
            subviews: subviews,
            maxWidth: proposal.width ?? .greatestFiniteMagnitude
        ).size
    }

    func placeSubviews(
        in bounds: CGRect,
        proposal: ProposedViewSize,
        subviews: Subviews,
        cache: inout ()
    ) {
        let result = layout(subviews: subviews, maxWidth: bounds.width)

        for (index, subview) in subviews.enumerated() {
            let position = CGPoint(
                x: bounds.minX + result.origins[index].x,
                y: bounds.minY + result.origins[index].y
            )
            subview.place(
                at: position,
                anchor: .topLeading,
                proposal: ProposedViewSize(result.sizes[index])
            )
        }
    }

    private func layout(subviews: Subviews, maxWidth: CGFloat) -> FlowLayoutResult {
        let sizes = subviews.map { $0.sizeThatFits(.unspecified) }
        var origins: [CGPoint] = []
        var x: CGFloat = 0
        var y: CGFloat = 0
        var rowHeight: CGFloat = 0
        var contentWidth: CGFloat = 0

        for size in sizes {
            if x > 0 && x + size.width > maxWidth {
                x = 0
                y += rowHeight + spacing
                rowHeight = 0
            }

            origins.append(CGPoint(x: x, y: y))
            x += size.width + spacing
            rowHeight = max(rowHeight, size.height)
            contentWidth = max(contentWidth, x - spacing)
        }

        let proposedWidth = maxWidth.isFinite ? maxWidth : contentWidth

        return FlowLayoutResult(
            origins: origins,
            sizes: sizes,
            size: CGSize(width: proposedWidth, height: y + rowHeight)
        )
    }
}

private struct FlowLayoutResult {
    let origins: [CGPoint]
    let sizes: [CGSize]
    let size: CGSize
}

private struct BoundaryLegend: View {
    let boundary: BreaksView.BreakBoundary

    var body: some View {
        HStack(spacing: 5) {
            BoundarySymbol(boundary: boundary)
            Text(boundary.name)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
    }
}

private struct BoundarySymbol: View {
    let boundary: BreaksView.BreakBoundary

    var body: some View {
        Text(boundary.symbol)
            .font(.title3.monospaced().weight(.semibold))
            .foregroundStyle(boundary.color)
            .accessibilityLabel(boundary.name)
            .help(boundary.name)
    }
}

private struct ScalarToken: View {
    let scalar: Unicode.Scalar
    var showsBackground = true

    var body: some View {
        Text(displayValue)
            .font(.title3)
            .frame(minWidth: 24, minHeight: 32)
            .padding(.horizontal, 4)
            .background {
                if showsBackground {
                    RoundedRectangle(cornerRadius: 5)
                        .fill(.quaternary)
                }
            }
            .accessibilityLabel(codepoint)
            .help(codepoint)
    }

    private var codepoint: String {
        MojibakeFormatting.codepoint(scalar.value)
    }

    private var displayValue: String {
        switch scalar.value {
        case 0x09:
            return "⇥"
        case 0x0A:
            return "↵"
        case 0x0D:
            return "␍"
        case 0x20:
            return "␠"
        case 0x200D:
            return "ZWJ"
        default:
            if let variationSelectorNumber {
                return "VS\(variationSelectorNumber)"
            }

            if mjb_codepoint_is_combining(scalar.value) {
                return "◌\(String(scalar))"
            }

            if !mjb_codepoint_is_graphic(scalar.value) {
                return codepoint
            }

            return String(scalar)
        }
    }

    private var variationSelectorNumber: UInt32? {
        switch scalar.value {
        case 0xFE00 ... 0xFE0F:
            scalar.value - 0xFE00 + 1
        case 0xE0100 ... 0xE01EF:
            scalar.value - 0xE0100 + 17
        default:
            nil
        }
    }
}

#Preview {
    BreaksView()
}
