//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct NormalizationView: View {
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
    @State private var selectedForm = NormalizationForm.nfc
    @State private var normalizationFailed = false

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            VStack(alignment: .leading, spacing: 4) {
                Text("Normalize")
                    .font(.largeTitle)

                Text("Convert text to a Unicode normalization form.")
                    .foregroundStyle(.secondary)
            }

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
            }

            GroupBox {
                TextEditor(text: $input)
                    .font(.body)
                    .frame(minHeight: 160)
                    .accessibilityLabel("Input text")
            } label: {
                Text("Input")
                    .font(.headline)
            }

            GroupBox {
                TextEditor(text: .constant(output))
                    .font(.body)
                    .frame(minHeight: 160)
                    .accessibilityLabel("Normalized output")
            } label: {
                HStack {
                    Text("Normalized Output")
                        .font(.headline)

                    Spacer()

                    Text(selectedForm.rawValue)
                        .font(.caption.monospaced())
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
        }
        .frame(minWidth: 360, maxWidth: 760, maxHeight: .infinity, alignment: .top)
        .frame(maxWidth: .infinity)
        .padding()
        .onChange(of: input) {
            normalize()
        }
        .onChange(of: selectedForm) {
            normalize()
        }
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

    private func normalized(_ value: String, form: mjb_normalization) -> String? {
        let input = value.utf8CString

        return input.withUnsafeBufferPointer { inputBuffer in
            var result = mjb_result()
            let status = mjb_normalize(
                inputBuffer.baseAddress,
                inputBuffer.count - 1,
                MJB_ENC_UTF_8,
                form,
                MJB_ENC_UTF_8,
                &result
            )

            guard status == MJB_STATUS_OK else {
                return nil
            }

            defer {
                _ = mjb_result_free(&result)
            }

            guard let output = result.output else {
                return result.output_size == 0 ? "" : nil
            }

            let bytes = UnsafeBufferPointer(start: output, count: Int(result.output_size))
            return String(decoding: bytes.map { UInt8(bitPattern: $0) }, as: UTF8.self)
        }
    }
}

#Preview {
    NormalizationView()
}
