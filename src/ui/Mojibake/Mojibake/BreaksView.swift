//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct BreaksView: View {
    fileprivate enum OutputStyle: String, CaseIterable, Identifiable {
        case codepoints = "Codepoints"
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
                "mjb_grapheme_break_next"
            case .word:
                "mjb_word_break_next"
            case .line:
                "mjb_line_break_next"
            case .sentence:
                "mjb_sentence_break_next"
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
    @State private var outputStyle = OutputStyle.codepoints
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
        MojibakeCounting.summary(for: input)
    }

    private func analyze() {
        results = MojibakeText.with(input) { text in
            [
                .grapheme: graphemeBreaks(text),
                .word: wordBreaks(text),
                .line: lineBreaks(text),
                .sentence: sentenceBreaks(text),
            ]
        }
    }

    private func graphemeBreaks(_ text: UnsafePointer<mjb_text>) -> [BreakBoundary] {
        var state = mjb_grapheme_break_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_grapheme_break_next(text, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func wordBreaks(_ text: UnsafePointer<mjb_text>) -> [BreakBoundary] {
        var state = mjb_word_break_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_word_break_next(text, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func lineBreaks(_ text: UnsafePointer<mjb_text>) -> [BreakBoundary] {
        var state = mjb_line_break_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_line_break_next(text, &state)

            guard type != MJB_BT_NOT_SET else {
                return boundaries
            }

            if let boundary = BreakBoundary(type) {
                boundaries.append(boundary)
            }
        }
    }

    private func sentenceBreaks(_ text: UnsafePointer<mjb_text>) -> [BreakBoundary] {
        var state = mjb_sentence_break_state()
        var boundaries: [BreakBoundary] = []

        while true {
            let type = mjb_sentence_break_next(text, &state)

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

                Button {
                    copyVisualization()
                } label: {
                    Image(systemName: "doc.on.doc")
                }
                .buttonStyle(.borderless)
                .disabled(input.isEmpty)
                .accessibilityLabel("Copy \(mode.rawValue)")
                .help("Copy the boundaries as text")
            }
        }
        .frame(maxWidth: .infinity)
    }

    private var visualization: String {
        var tokens: [String] = []

        switch outputStyle {
        case .codepoints:
            for index in scalars.indices {
                if let boundary = boundary(before: index) {
                    tokens.append(boundary.symbol)
                }

                tokens.append(ScalarToken.displayValue(for: scalars[index]))
            }
        case .noBreakBlocks:
            for block in noBreakBlocks {
                tokens.append(block.leadingBoundary.symbol)
                tokens.append(
                    scalars[block.range]
                        .map { ScalarToken.displayValue(for: $0) }
                        .joined(separator: " ")
                )
            }
        }

        if let lastIndex = scalars.indices.last, let boundary = boundary(after: lastIndex) {
            tokens.append(boundary.symbol)
        }

        return tokens.joined(separator: " ")
    }

    private func copyVisualization() {
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(visualization, forType: .string)
    }

    @ViewBuilder
    private var output: some View {
        switch outputStyle {
        case .codepoints:
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
            .background(.quaternary, in: RoundedRectangle(cornerRadius: 5))
            .overlay {
                RoundedRectangle(cornerRadius: 5)
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
        Self.displayValue(for: scalar)
    }

    static func displayValue(for scalar: Unicode.Scalar) -> String {
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
            if let variationSelectorNumber = variationSelectorNumber(for: scalar) {
                return "VS\(variationSelectorNumber)"
            }

            if mjb_codepoint_is_combining(scalar.value) {
                return "◌\(String(scalar))"
            }

            if !mjb_codepoint_is_graphic(scalar.value) {
                return MojibakeFormatting.codepoint(scalar.value)
            }

            return String(scalar)
        }
    }

    private static func variationSelectorNumber(for scalar: Unicode.Scalar) -> UInt32? {
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
