//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct NormalizationView: View {
    private enum QuickCheck {
        case yes
        case no
        case maybe

        init?(_ value: mjb_quick_check_result) {
            switch value {
            case MJB_QC_YES:
                self = .yes
            case MJB_QC_NO:
                self = .no
            case MJB_QC_MAYBE:
                self = .maybe
            default:
                return nil
            }
        }

        var name: String {
            switch self {
            case .yes:
                "Yes"
            case .no:
                "No"
            case .maybe:
                "Maybe"
            }
        }

        var systemImage: String {
            switch self {
            case .yes:
                "checkmark.circle.fill"
            case .no:
                "xmark.circle.fill"
            case .maybe:
                "questionmark.circle.fill"
            }
        }

        var color: Color {
            switch self {
            case .yes:
                .green
            case .no:
                .orange
            case .maybe:
                .secondary
            }
        }
    }

    private enum NormalizationForm: String, CaseIterable, Identifiable {
        case nfc = "NFC"
        case nfd = "NFD"
        case nfkc = "NFKC"
        case nfkd = "NFKD"

        var id: Self {
            self
        }

        var value: mjb_normalization {
            switch self {
            case .nfc:
                MJB_NORMALIZATION_NFC
            case .nfd:
                MJB_NORMALIZATION_NFD
            case .nfkc:
                MJB_NORMALIZATION_NFKC
            case .nfkd:
                MJB_NORMALIZATION_NFKD
            }
        }

        var description: String {
            switch self {
            case .nfc:
                "Canonical decomposition followed by canonical composition"
            case .nfd:
                "Canonical decomposition"
            case .nfkc:
                "Compatibility decomposition followed by canonical composition"
            case .nfkd:
                "Compatibility decomposition"
            }
        }
    }

    @State private var input = ""
    @State private var output = ""
    @State private var nfkcCasefoldOutput = ""
    @State private var selectedForm = NormalizationForm.nfc
    @State private var quickCheck = QuickCheck.yes
    @State private var quickCheckFailed = false
    @State private var normalizationFailed = false
    @State private var nfkcCasefoldFailed = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Normalize",
                    description: "Normalize text and apply the Unicode NFKC_Casefold transform."
                )

                VStack(alignment: .leading, spacing: 6) {
                    Picker("Normalization Form", selection: $selectedForm) {
                        ForEach(NormalizationForm.allCases) { form in
                            Text(form.rawValue)
                                .tag(form)
                        }
                    }
                    .pickerStyle(.segmented)

                    Text(selectedForm.description)
                        .font(.caption)
                        .foregroundStyle(.secondary)

                    HStack {
                        if quickCheckFailed {
                            Label(
                                "Quick Check Failed",
                                systemImage: "exclamationmark.triangle"
                            )
                            .foregroundStyle(.red)
                        } else {
                            Label(
                                "Quick Check: \(quickCheck.name)",
                                systemImage: quickCheck.systemImage
                            )
                            .foregroundStyle(quickCheck.color)
                        }

                        Spacer()

                        Text("mjb_normalization_quick_check")
                            .font(.caption.monospaced())
                            .foregroundStyle(.secondary)
                    }
                    .font(.caption)
                }

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

                GroupBox {
                    TextEditor(text: .constant(output))
                        .font(.body)
                        .frame(minHeight: 120)
                        .accessibilityLabel("Normalized output")
                } label: {
                    HStack {
                        Text("Normalized Output")
                            .font(.headline)

                        Spacer()

                        Text(outputSummary)
                            .font(.caption.monospacedDigit())
                            .foregroundStyle(.secondary)
                    }
                }

                if normalizationFailed {
                    Label(
                        "The input could not be normalized.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.red)
                }

                GroupBox {
                    VStack(alignment: .leading, spacing: 8) {
                        TextEditor(text: .constant(nfkcCasefoldOutput))
                            .font(.body)
                            .frame(minHeight: 120)
                            .accessibilityLabel("NFKC casefold output")

                        HStack {
                            Text(
                                "Compatibility folding, full case folding, and "
                                    + "default-ignorable removal."
                            )
                            .font(.caption)
                            .foregroundStyle(.secondary)

                            Spacer()

                            Text(nfkcCasefoldOutputSummary)
                                .font(.caption.monospacedDigit())
                                .foregroundStyle(.secondary)
                        }
                    }
                } label: {
                    HStack {
                        Text("NFKC Casefold Output")
                            .font(.headline)

                        Spacer()

                        Text("mjb_nfkc_casefold")
                            .font(.caption.monospaced())
                            .foregroundStyle(.secondary)
                    }
                }

                if nfkcCasefoldFailed {
                    Label(
                        "The input could not be NFKC casefolded.",
                        systemImage: "exclamationmark.triangle"
                    )
                    .foregroundStyle(.red)
                }
            }
            .frame(minWidth: 360, maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            updateResults()
        }
        .onChange(of: selectedForm) {
            updateResults()
        }
    }

    private var inputSummary: String {
        MojibakeCounting.summary(for: input)
    }

    private var outputSummary: String {
        MojibakeCounting.summary(for: output)
    }

    private var nfkcCasefoldOutputSummary: String {
        MojibakeCounting.summary(for: nfkcCasefoldOutput)
    }

    private func updateResults() {
        runQuickCheck()
        normalize()
        nfkcCasefold()
    }

    private func runQuickCheck() {
        let utf8 = input.utf8CString
        var result = MJB_QC_NO
        let status = utf8.withUnsafeBufferPointer { buffer in
            mjb_normalization_quick_check(
                buffer.baseAddress,
                buffer.count - 1,
                MJB_ENC_UTF_8,
                selectedForm.value,
                &result
            )
        }

        guard status == MJB_STATUS_OK, let resolvedResult = QuickCheck(result) else {
            quickCheckFailed = true
            return
        }

        quickCheck = resolvedResult
        quickCheckFailed = false
    }

    private func normalize() {
        guard let normalized = normalized(input, form: selectedForm.value) else {
            output = ""
            normalizationFailed = true
            return
        }

        output = normalized
        normalizationFailed = false
    }

    private func nfkcCasefold() {
        guard let transformed = nfkcCasefolded(input) else {
            nfkcCasefoldOutput = ""
            nfkcCasefoldFailed = true
            return
        }

        nfkcCasefoldOutput = transformed
        nfkcCasefoldFailed = false
    }

    private func normalized(_ value: String, form: mjb_normalization) -> String? {
        MojibakeString.transform(value) { input, byteLength, result in
            mjb_normalize(
                input,
                byteLength,
                MJB_ENC_UTF_8,
                form,
                MJB_ENC_UTF_8,
                result
            )
        }
    }

    private func nfkcCasefolded(_ value: String) -> String? {
        MojibakeString.transform(value) { input, byteLength, result in
            mjb_nfkc_casefold(
                input,
                byteLength,
                MJB_ENC_UTF_8,
                MJB_ENC_UTF_8,
                result
            )
        }
    }
}

#Preview {
    NormalizationView()
}
