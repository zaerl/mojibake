//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CollationView: View {
    @State private var firstInput = ""
    @State private var secondInput = ""
    @State private var strength = CollationStrengthOption.tertiary
    @State private var weighting = CollationWeightingOption.nonIgnorable
    @State private var resolution: CollationResolution?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Collation",
                    description: "Compare text using the locale-independent Unicode DUCET order."
                )

                CollationOptionsView(
                    strength: $strength,
                    weighting: $weighting
                )

                ViewThatFits(in: .horizontal) {
                    HStack(alignment: .top, spacing: 12) {
                        inputEditors
                    }

                    VStack(spacing: 12) {
                        inputEditors
                    }
                }

                if let resolution {
                    if let order = resolution.order {
                        CollationComparisonView(order: order)
                    }

                    if let firstKey = resolution.firstKey,
                        let secondKey = resolution.secondKey
                    {
                        ViewThatFits(in: .horizontal) {
                            HStack(alignment: .top, spacing: 12) {
                                keyViews(firstKey: firstKey, secondKey: secondKey)
                            }

                            VStack(spacing: 12) {
                                keyViews(firstKey: firstKey, secondKey: secondKey)
                            }
                        }
                    }

                    if let errorMessage = resolution.errorMessage {
                        ContentUnavailableView(
                            "Collation Failed",
                            systemImage: "exclamationmark.triangle",
                            description: Text(errorMessage)
                        )
                        .frame(maxWidth: .infinity, minHeight: 220)
                    }
                } else {
                    ContentUnavailableView(
                        "Enter Text to Compare",
                        systemImage: "arrow.up.arrow.down",
                        description: Text(
                            "Try “cafe” and “café”, then change the collation strength."
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
            compare()
        }
        .onChange(of: secondInput) {
            compare()
        }
        .onChange(of: strength) {
            compare()
        }
        .onChange(of: weighting) {
            compare()
        }
    }

    @ViewBuilder
    private var inputEditors: some View {
        ToolTextEditor("First Text", text: $firstInput)
        ToolTextEditor("Second Text", text: $secondInput)
    }

    @ViewBuilder
    private func keyViews(firstKey: [UInt8], secondKey: [UInt8]) -> some View {
        CollationKeyView(title: "First Sort Key", bytes: firstKey)
        CollationKeyView(title: "Second Sort Key", bytes: secondKey)
    }

    private func compare() {
        guard !firstInput.isEmpty || !secondInput.isEmpty else {
            resolution = nil
            return
        }

        resolution = CollationResolution.compare(
            firstInput,
            with: secondInput,
            weighting: weighting.value,
            strength: strength.value
        )
    }
}

private struct CollationOptionsView: View {
    @Binding var strength: CollationStrengthOption
    @Binding var weighting: CollationWeightingOption

    var body: some View {
        GroupBox {
            VStack(alignment: .leading, spacing: 10) {
                Grid(alignment: .leading, horizontalSpacing: 16, verticalSpacing: 8) {
                    GridRow {
                        Text("Strength")

                        Picker("Strength", selection: $strength) {
                            ForEach(CollationStrengthOption.allCases) { option in
                                Text(option.rawValue)
                                    .tag(option)
                            }
                        }
                        .labelsHidden()
                        .pickerStyle(.menu)
                        .frame(minWidth: 150, alignment: .leading)
                    }

                    GridRow {
                        Text("Variable Weighting")

                        Picker("Variable Weighting", selection: $weighting) {
                            ForEach(CollationWeightingOption.allCases) { option in
                                Text(option.rawValue)
                                    .tag(option)
                            }
                        }
                        .labelsHidden()
                        .pickerStyle(.menu)
                        .frame(minWidth: 150, alignment: .leading)
                    }
                }

                Divider()

                Text("\(strength.description) \(weighting.description)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            .padding(.vertical, 4)
        } label: {
            Text("Options")
                .font(.headline)
        }
    }
}

private struct CollationComparisonView: View {
    let order: Int

    var body: some View {
        GroupBox {
            HStack(alignment: .top, spacing: 12) {
                Image(systemName: systemImage)
                    .font(.title2)
                    .foregroundStyle(.blue)

                VStack(alignment: .leading, spacing: 4) {
                    Text(title)
                        .font(.headline)

                    Text(detail)
                        .foregroundStyle(.secondary)
                }

                Spacer()

                Text(symbol)
                    .font(.largeTitle.monospaced())
                    .accessibilityLabel(title)
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, 4)
        } label: {
            Text("Comparison")
                .font(.headline)
        }
    }

    private var title: String {
        if order < 0 {
            return "First Text Sorts Before Second Text"
        }

        if order > 0 {
            return "First Text Sorts After Second Text"
        }

        return "Texts Compare Equally"
    }

    private var detail: String {
        order == 0
            ? "The selected collation levels contain no differences."
            : "The ordering is determined by the first differing collation weight."
    }

    private var symbol: String {
        if order < 0 {
            return "<"
        }

        if order > 0 {
            return ">"
        }

        return "="
    }

    private var systemImage: String {
        order == 0 ? "equal.circle.fill" : "arrow.left.and.right.circle.fill"
    }
}

private struct CollationKeyView: View {
    let title: LocalizedStringKey
    let bytes: [UInt8]

    var body: some View {
        GroupBox {
            Text(hexadecimal)
                .font(.body.monospaced())
                .foregroundStyle(bytes.isEmpty ? .secondary : .primary)
                .textSelection(.enabled)
                .frame(maxWidth: .infinity, minHeight: 64, alignment: .topLeading)
        } label: {
            HStack {
                Text(title)
                    .font(.headline)

                Spacer()

                Text("\(bytes.count) bytes")
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
        }
        .frame(minWidth: 260, maxWidth: .infinity)
    }

    private var hexadecimal: String {
        guard !bytes.isEmpty else {
            return "No effective weights"
        }

        return bytes
            .map { String(format: "%02X", $0) }
            .joined(separator: " ")
    }
}

private struct CollationResolution {
    let order: Int?
    let firstKey: [UInt8]?
    let secondKey: [UInt8]?
    let errorMessage: String?

    static func compare(
        _ first: String,
        with second: String,
        weighting: mjb_collation_variable_weighting,
        strength: mjb_collation_strength
    ) -> CollationResolution {
        let comparison = compareStrings(
            first,
            second,
            weighting: weighting,
            strength: strength
        )

        guard let order = comparison.order else {
            return failure(comparison.errorMessage)
        }

        let firstResult = sortKey(
            for: first,
            weighting: weighting,
            strength: strength
        )

        guard let firstKey = firstResult.bytes else {
            return failure(firstResult.errorMessage)
        }

        let secondResult = sortKey(
            for: second,
            weighting: weighting,
            strength: strength
        )

        guard let secondKey = secondResult.bytes else {
            return failure(secondResult.errorMessage)
        }

        return CollationResolution(
            order: order,
            firstKey: firstKey,
            secondKey: secondKey,
            errorMessage: nil
        )
    }

    private static func compareStrings(
        _ first: String,
        _ second: String,
        weighting: mjb_collation_variable_weighting,
        strength: mjb_collation_strength
    ) -> (order: Int?, errorMessage: String?) {
        let firstUTF8 = first.utf8CString
        let secondUTF8 = second.utf8CString

        return firstUTF8.withUnsafeBufferPointer { firstBuffer in
            secondUTF8.withUnsafeBufferPointer { secondBuffer in
                var order: Int32 = 0
                let status = mjb_collation_compare(
                    firstBuffer.baseAddress,
                    firstBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    secondBuffer.baseAddress,
                    secondBuffer.count - 1,
                    MJB_ENC_UTF_8,
                    weighting,
                    strength,
                    &order
                )

                guard status == MJB_STATUS_OK else {
                    return (
                        nil,
                        MojibakeFormatting.statusMessage(
                            operation: "mjb_collation_compare",
                            status: status
                        )
                    )
                }

                return (Int(order), nil)
            }
        }
    }

    private static func sortKey(
        for input: String,
        weighting: mjb_collation_variable_weighting,
        strength: mjb_collation_strength
    ) -> (bytes: [UInt8]?, errorMessage: String?) {
        var status = MJB_STATUS_OK
        let bytes = MojibakeBytes.transform(input) { buffer, byteLength, result in
            status = mjb_collation_key(
                buffer,
                byteLength,
                MJB_ENC_UTF_8,
                MJB_MALFORMED_STOP,
                weighting,
                strength,
                result,
                nil
            )
            return status
        }

        guard let bytes else {
            return (
                nil,
                MojibakeFormatting.statusMessage(
                    operation: "mjb_collation_key",
                    status: status
                )
            )
        }

        return (bytes, nil)
    }

    private static func failure(_ message: String?) -> CollationResolution {
        CollationResolution(
            order: nil,
            firstKey: nil,
            secondKey: nil,
            errorMessage: message ?? "Unknown collation error"
        )
    }
}

private enum CollationStrengthOption: String, CaseIterable, Identifiable {
    case primary = "Primary"
    case secondary = "Secondary"
    case tertiary = "Tertiary"
    case quaternary = "Quaternary"

    var id: Self {
        self
    }

    var value: mjb_collation_strength {
        switch self {
        case .primary:
            MJB_COLLATION_PRIMARY
        case .secondary:
            MJB_COLLATION_SECONDARY
        case .tertiary:
            MJB_COLLATION_TERTIARY
        case .quaternary:
            MJB_COLLATION_QUATERNARY
        }
    }

    var description: String {
        switch self {
        case .primary:
            "Compare base characters while ignoring accents and case."
        case .secondary:
            "Also compare accents while continuing to ignore case."
        case .tertiary:
            "Also compare case and other tertiary variants."
        case .quaternary:
            "Also compare variable elements moved to level four by shifted weighting."
        }
    }
}

private enum CollationWeightingOption: String, CaseIterable, Identifiable {
    case nonIgnorable = "Non-Ignorable"
    case shifted = "Shifted"

    var id: Self {
        self
    }

    var value: mjb_collation_variable_weighting {
        switch self {
        case .nonIgnorable:
            MJB_COLLATION_NON_IGNORABLE
        case .shifted:
            MJB_COLLATION_SHIFTED
        }
    }

    var description: String {
        switch self {
        case .nonIgnorable:
            "Keep punctuation and other variable elements at their normal weights."
        case .shifted:
            "Move variable elements to level four so lower strengths ignore them."
        }
    }
}

#Preview {
    CollationView()
}
