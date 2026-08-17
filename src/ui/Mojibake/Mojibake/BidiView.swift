//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct BidiView: View {
    fileprivate enum BaseDirection: String, CaseIterable, Identifiable {
        case auto = "Auto"
        case leftToRight = "Left-to-Right"
        case rightToLeft = "Right-to-Left"

        var id: Self {
            self
        }

        var value: mjb_direction {
            switch self {
            case .auto:
                MJB_DIRECTION_AUTO
            case .leftToRight:
                MJB_DIRECTION_LTR
            case .rightToLeft:
                MJB_DIRECTION_RTL
            }
        }
    }

    @State private var input = ""
    @State private var baseDirection = BaseDirection.auto
    @State private var analysis: BidiAnalysis?
    @State private var errorMessage: String?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Bidi",
                    description: "Resolve and visualize the Unicode Bidirectional Algorithm."
                )

                Picker("Base Direction", selection: $baseDirection) {
                    ForEach(BaseDirection.allCases) { direction in
                        Text(direction.rawValue)
                    }
                }
                .pickerStyle(.segmented)

                GroupBox {
                    TextEditor(text: $input)
                        .font(.body)
                        .frame(minHeight: 140)
                        .accessibilityLabel("Bidirectional input text")
                } label: {
                    HStack {
                        Text("Logical Input")
                            .font(.headline)

                        Spacer()

                        Text(inputSummary)
                            .font(.caption.monospacedDigit())
                            .foregroundStyle(.secondary)
                    }
                }

                if let analysis {
                    BidiParagraphView(analysis: analysis)
                    BidiVisualOrderView(analysis: analysis)
                    BidiRunsView(runs: analysis.runs)
                    BidiCharactersView(characters: analysis.characters)
                } else if let errorMessage {
                    ContentUnavailableView(
                        "Bidi Resolution Failed",
                        systemImage: "exclamationmark.triangle",
                        description: Text(errorMessage)
                    )
                    .frame(maxWidth: .infinity, minHeight: 220)
                } else {
                    ContentUnavailableView(
                        "Enter Bidirectional Text",
                        systemImage: "arrow.left.arrow.right",
                        description: Text(
                            "Try mixing Latin, Arabic, Hebrew, numbers, and punctuation."
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
            resolve()
        }
        .onChange(of: baseDirection) {
            resolve()
        }
    }

    private var inputSummary: String {
        MojibakeCounting.summary(for: input)
    }

    private func resolve() {
        guard !input.isEmpty else {
            analysis = nil
            errorMessage = nil
            return
        }

        let resolution = BidiAnalysis.resolve(input: input, direction: baseDirection.value)
        analysis = resolution.analysis
        errorMessage = resolution.errorMessage
    }
}

private struct BidiResolution {
    let analysis: BidiAnalysis?
    let errorMessage: String?

    static func success(_ analysis: BidiAnalysis) -> Self {
        Self(analysis: analysis, errorMessage: nil)
    }

    static func failure(_ message: String) -> Self {
        Self(analysis: nil, errorMessage: message)
    }
}

private struct BidiAnalysis {
    let paragraphLevel: Int
    let direction: String
    let characters: [BidiCharacterAnalysis]
    let visualOrder: [Int]
    let runs: [BidiRunAnalysis]

    var visualCharacters: [BidiVisualCharacter] {
        visualOrder.enumerated().compactMap { visualIndex, logicalIndex in
            guard characters.indices.contains(logicalIndex) else {
                return nil
            }

            return BidiVisualCharacter(
                visualIndex: visualIndex,
                character: characters[logicalIndex]
            )
        }
    }

    static func resolve(input: String, direction: mjb_direction) -> BidiResolution {
        let utf8 = input.utf8CString

        return utf8.withUnsafeBufferPointer { buffer in
            var paragraph = mjb_bidi_paragraph()
            let status = mjb_bidi_resolve(
                buffer.baseAddress,
                buffer.count - 1,
                MJB_ENC_UTF_8,
                direction,
                &paragraph
            )

            guard status == MJB_STATUS_OK else {
                return .failure(statusMessage(operation: "mjb_bidi_resolve", status: status))
            }

            defer {
                mjb_bidi_paragraph_free(&paragraph)
            }

            let characters = copyCharacters(from: paragraph)

            guard paragraph.count > 0 else {
                return .success(
                    BidiAnalysis(
                        paragraphLevel: Int(paragraph.paragraph_level),
                        direction: directionName(paragraph.direction),
                        characters: [],
                        visualOrder: [],
                        runs: []
                    )
                )
            }

            var visualOrder = [Int](repeating: 0, count: paragraph.count)
            let reorderStatus = visualOrder.withUnsafeMutableBufferPointer {
                mjb_bidi_reorder_line(
                    &paragraph,
                    0,
                    paragraph.count,
                    $0.baseAddress
                )
            }

            guard reorderStatus == MJB_STATUS_OK else {
                return .failure(
                    statusMessage(
                        operation: "mjb_bidi_reorder_line",
                        status: reorderStatus
                    )
                )
            }

            let runsResolution = copyRuns(from: paragraph, visualOrder: visualOrder)

            guard let runs = runsResolution.runs else {
                return .failure(runsResolution.errorMessage ?? "Visual run analysis failed.")
            }

            return .success(
                BidiAnalysis(
                    paragraphLevel: Int(paragraph.paragraph_level),
                    direction: directionName(paragraph.direction),
                    characters: characters,
                    visualOrder: visualOrder,
                    runs: runs
                )
            )
        }
    }

    private static func copyCharacters(
        from paragraph: mjb_bidi_paragraph
    ) -> [BidiCharacterAnalysis] {
        guard let source = paragraph.chars else {
            return []
        }

        return (0 ..< paragraph.count).map { index in
            let character = source[index]

            return BidiCharacterAnalysis(
                logicalIndex: index,
                codepoint: character.codepoint,
                byteOffset: character.byte_offset,
                level: Int(character.level),
                classID: Int(character.resolved_class.rawValue),
                mirroringGlyph: character.mirroring_glyph
            )
        }
    }

    private static func copyRuns(
        from sourceParagraph: mjb_bidi_paragraph,
        visualOrder: [Int]
    ) -> (runs: [BidiRunAnalysis]?, errorMessage: String?) {
        var paragraph = sourceParagraph
        var runCount = 0
        let countStatus = visualOrder.withUnsafeBufferPointer {
            mjb_bidi_line_runs(
                &paragraph,
                $0.baseAddress,
                $0.count,
                nil,
                &runCount
            )
        }

        guard countStatus == MJB_STATUS_OK else {
            return (
                nil,
                statusMessage(operation: "mjb_bidi_line_runs", status: countStatus)
            )
        }

        var sourceRuns = [mjb_bidi_run](repeating: mjb_bidi_run(), count: runCount)
        let fillStatus = visualOrder.withUnsafeBufferPointer { orderBuffer in
            sourceRuns.withUnsafeMutableBufferPointer { runBuffer in
                mjb_bidi_line_runs(
                    &paragraph,
                    orderBuffer.baseAddress,
                    orderBuffer.count,
                    runBuffer.baseAddress,
                    &runCount
                )
            }
        }

        guard fillStatus == MJB_STATUS_OK else {
            return (
                nil,
                statusMessage(operation: "mjb_bidi_line_runs", status: fillStatus)
            )
        }

        let runs = sourceRuns.prefix(runCount).enumerated().map { index, run in
            BidiRunAnalysis(
                id: index,
                start: run.start,
                end: run.end,
                level: Int(run.level),
                direction: directionName(run.direction)
            )
        }

        return (runs, nil)
    }

    private static func statusMessage(operation: String, status: mjb_status) -> String {
        MojibakeFormatting.statusMessage(operation: operation, status: status)
    }

    private static func directionName(_ direction: mjb_direction) -> String {
        switch direction {
        case MJB_DIRECTION_LTR:
            "Left-to-right"
        case MJB_DIRECTION_RTL:
            "Right-to-left"
        default:
            "Auto"
        }
    }
}

private struct BidiCharacterAnalysis: Identifiable {
    let logicalIndex: Int
    let codepoint: mjb_codepoint
    let byteOffset: Int
    let level: Int
    let classID: Int
    let mirroringGlyph: mjb_codepoint

    var id: Int {
        logicalIndex
    }

    var codepointText: String {
        BidiFormatting.codepoint(codepoint)
    }

    var displayValue: String {
        BidiFormatting.displayValue(codepoint)
    }

    var visualValue: String {
        mirroringGlyph == 0 ? displayValue : BidiFormatting.displayValue(mirroringGlyph)
    }

    var classAbbreviation: String {
        BidiFormatting.classAbbreviation(classID)
    }

    var className: String {
        BidiFormatting.className(classID)
    }

    var mirroringText: String {
        mirroringGlyph == 0 ? "None" : BidiFormatting.codepoint(mirroringGlyph)
    }
}

private struct BidiVisualCharacter: Identifiable {
    let visualIndex: Int
    let character: BidiCharacterAnalysis

    var id: Int {
        visualIndex
    }
}

private struct BidiRunAnalysis: Identifiable {
    let id: Int
    let start: Int
    let end: Int
    let level: Int
    let direction: String
}

private struct BidiParagraphView: View {
    let analysis: BidiAnalysis

    var body: some View {
        GroupBox {
            HStack(spacing: 12) {
                BidiMetric(label: "Paragraph Level", value: String(analysis.paragraphLevel))
                BidiMetric(label: "Direction", value: analysis.direction)
                BidiMetric(label: "Characters", value: String(analysis.characters.count))
                BidiMetric(label: "Visual Runs", value: String(analysis.runs.count))
            }
            .frame(maxWidth: .infinity)
        } label: {
            HStack {
                Text("Paragraph")
                    .font(.headline)

                Spacer()

                Text("mjb_bidi_resolve")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct BidiMetric: View {
    let label: String
    let value: String

    var body: some View {
        VStack(spacing: 3) {
            Text(value)
                .font(.headline.monospacedDigit())

            Text(label)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity)
    }
}

private struct BidiVisualOrderView: View {
    let analysis: BidiAnalysis

    private let columns = [
        GridItem(.adaptive(minimum: 76, maximum: 100), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(analysis.visualCharacters) { item in
                    BidiVisualToken(item: item)
                }
            }
            .environment(\.layoutDirection, .leftToRight)
            .padding(.vertical, 4)
        } label: {
            HStack {
                Text("Visual Order")
                    .font(.headline)

                Spacer()

                Text("mjb_bidi_reorder_line")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct BidiVisualToken: View {
    let item: BidiVisualCharacter

    var body: some View {
        VStack(spacing: 3) {
            Text(item.character.visualValue)
                .font(.title2)
                .frame(minHeight: 30)

            Text("v\(item.visualIndex) ← #\(item.character.logicalIndex)")
                .font(.caption2.monospacedDigit())
                .foregroundStyle(.secondary)

            Text("L\(item.character.level) · \(item.character.classAbbreviation)")
                .font(.caption2.monospaced())
                .foregroundStyle(levelColor)
        }
        .frame(maxWidth: .infinity)
        .padding(6)
        .background(levelColor.opacity(0.12), in: RoundedRectangle(cornerRadius: 7))
        .overlay {
            RoundedRectangle(cornerRadius: 7)
                .stroke(levelColor.opacity(0.3))
        }
        .help(helpText)
    }

    private var levelColor: Color {
        item.character.level.isMultiple(of: 2) ? .blue : .purple
    }

    private var helpText: String {
        let character = item.character
        var components = [
            character.codepointText,
            "logical \(character.logicalIndex)",
            "visual \(item.visualIndex)",
            "level \(character.level)",
            character.className,
        ]

        if character.mirroringGlyph != 0 {
            components.append("mirrored as \(character.mirroringText)")
        }

        return components.joined(separator: " · ")
    }
}

private struct BidiRunsView: View {
    let runs: [BidiRunAnalysis]

    private let columns = [
        GridItem(.adaptive(minimum: 150), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(runs) { run in
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Run \(run.id + 1)")
                            .font(.headline)

                        Text("Visual positions \(run.start)..<\(run.end)")
                            .font(.caption.monospacedDigit())

                        Text("Level \(run.level) · \(run.direction)")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(8)
                    .background(.quaternary, in: RoundedRectangle(cornerRadius: 7))
                }
            }
        } label: {
            HStack {
                Text("Visual Runs")
                    .font(.headline)

                Spacer()

                Text("mjb_bidi_line_runs")
                    .font(.caption.monospaced())
                    .foregroundStyle(.secondary)
            }
        }
    }
}

private struct BidiCharactersView: View {
    let characters: [BidiCharacterAnalysis]

    private let columns = [
        GridItem(.adaptive(minimum: 210), spacing: 8),
    ]

    var body: some View {
        GroupBox {
            LazyVGrid(columns: columns, alignment: .leading, spacing: 8) {
                ForEach(characters) { character in
                    BidiCharacterCard(character: character)
                }
            }
        } label: {
            Text("Resolved Characters")
                .font(.headline)
        }
    }
}

private struct BidiCharacterCard: View {
    let character: BidiCharacterAnalysis

    var body: some View {
        HStack(alignment: .top, spacing: 10) {
            Text(character.displayValue)
                .font(.title2)
                .frame(minWidth: 42, minHeight: 42)
                .background(.quaternary, in: RoundedRectangle(cornerRadius: 6))

            VStack(alignment: .leading, spacing: 3) {
                Text("#\(character.logicalIndex) · \(character.codepointText)")
                    .font(.caption.monospaced())

                Text("Byte \(character.byteOffset) · Level \(character.level)")
                    .font(.caption.monospacedDigit())

                Text("\(character.classAbbreviation) · \(character.className)")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                if character.mirroringGlyph != 0 {
                    Text("Mirror: \(character.mirroringText)")
                        .font(.caption.monospaced())
                        .foregroundStyle(.secondary)
                }
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(8)
        .overlay {
            RoundedRectangle(cornerRadius: 7)
                .stroke(.secondary.opacity(0.2))
        }
    }
}

private enum BidiFormatting {
    private static let classAbbreviations = [
        "—", "L", "R", "AL", "EN", "ES", "ET", "AN", "CS", "NSM", "BN", "B",
        "S", "WS", "ON", "LRE", "LRO", "RLE", "RLO", "PDF", "LRI", "RLI", "FSI",
        "PDI",
    ]

    private static let classNames = [
        "Not set",
        "Left-to-right",
        "Right-to-left",
        "Right-to-left Arabic",
        "European number",
        "European number separator",
        "European number terminator",
        "Arabic number",
        "Common number separator",
        "Nonspacing mark",
        "Boundary neutral",
        "Paragraph separator",
        "Segment separator",
        "Whitespace",
        "Other neutral",
        "Left-to-right embedding",
        "Left-to-right override",
        "Right-to-left embedding",
        "Right-to-left override",
        "Pop directional format",
        "Left-to-right isolate",
        "Right-to-left isolate",
        "First strong isolate",
        "Pop directional isolate",
    ]

    static func classAbbreviation(_ id: Int) -> String {
        classAbbreviations.indices.contains(id) ? classAbbreviations[id] : "?"
    }

    static func className(_ id: Int) -> String {
        classNames.indices.contains(id) ? classNames[id] : "Unknown"
    }

    static func codepoint(_ codepoint: mjb_codepoint) -> String {
        MojibakeFormatting.codepoint(codepoint)
    }

    static func displayValue(_ codepoint: mjb_codepoint) -> String {
        switch codepoint {
        case 0x09:
            return "⇥"
        case 0x0A:
            return "↵"
        case 0x0D:
            return "␍"
        case 0x20:
            return "␠"
        case 0x061C:
            return "ALM"
        case 0x200E:
            return "LRM"
        case 0x200F:
            return "RLM"
        case 0x202A:
            return "LRE"
        case 0x202B:
            return "RLE"
        case 0x202C:
            return "PDF"
        case 0x202D:
            return "LRO"
        case 0x202E:
            return "RLO"
        case 0x2066:
            return "LRI"
        case 0x2067:
            return "RLI"
        case 0x2068:
            return "FSI"
        case 0x2069:
            return "PDI"
        default:
            guard let scalar = Unicode.Scalar(codepoint) else {
                return self.codepoint(codepoint)
            }

            if mjb_codepoint_is_combining(codepoint) {
                return "◌\(String(scalar))"
            }

            if !mjb_codepoint_is_graphic(codepoint) {
                return self.codepoint(codepoint)
            }

            return String(scalar)
        }
    }
}

#Preview {
    BidiView()
}
