//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CaseView: View {
    private enum CaseMapping: String, CaseIterable, Identifiable {
        case upper = "Uppercase"
        case lower = "Lowercase"
        case title = "Titlecase"
        case casefold = "Case Fold"
        case casefoldSimple = "Simple Case Fold"

        var id: Self {
            self
        }

        var type: mjb_map_case_type {
            switch self {
            case .upper:
                MJB_CASE_UPPER
            case .lower:
                MJB_CASE_LOWER
            case .title:
                MJB_CASE_TITLE
            case .casefold:
                MJB_CASE_CASEFOLD
            case .casefoldSimple:
                MJB_CASE_CASEFOLD_SIMPLE
            }
        }

        var constantName: String {
            switch self {
            case .upper:
                "MJB_CASE_UPPER"
            case .lower:
                "MJB_CASE_LOWER"
            case .title:
                "MJB_CASE_TITLE"
            case .casefold:
                "MJB_CASE_CASEFOLD"
            case .casefoldSimple:
                "MJB_CASE_CASEFOLD_SIMPLE"
            }
        }
    }

    @State private var input = ""
    @State private var outputs: [CaseMapping: String] = [:]
    @State private var failedMappings: Set<CaseMapping> = []

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Case")
                        .font(.largeTitle)

                    Text("Apply every Unicode case mapping.")
                        .foregroundStyle(.secondary)
                }

                GroupBox {
                    TextEditor(text: $input)
                        .font(.body)
                        .frame(minHeight: 120)
                        .accessibilityLabel("Input text")
                } label: {
                    Text("Input")
                        .font(.headline)
                }

                ForEach(CaseMapping.allCases) { mapping in
                    GroupBox {
                        VStack(alignment: .leading, spacing: 8) {
                            TextEditor(text: .constant(outputs[mapping] ?? ""))
                                .font(.body)
                                .frame(minHeight: 88)
                                .accessibilityLabel("\(mapping.rawValue) output")

                            if failedMappings.contains(mapping) {
                                Label(
                                    "The case mapping failed.",
                                    systemImage: "exclamationmark.triangle"
                                )
                                .foregroundStyle(.red)
                            }
                        }
                    } label: {
                        HStack {
                            Text(mapping.rawValue)
                                .font(.headline)

                            Spacer()

                            Text(mapping.constantName)
                                .font(.caption.monospaced())
                                .foregroundStyle(.secondary)
                        }
                    }
                }
            }
            .frame(minWidth: 360, maxWidth: 760)
            .frame(maxWidth: .infinity)
            .padding()
        }
        .onChange(of: input) {
            mapCases()
        }
    }

    private func mapCases() {
        var newOutputs: [CaseMapping: String] = [:]
        var newFailures: Set<CaseMapping> = []

        for mapping in CaseMapping.allCases {
            guard let output = mapped(input, type: mapping.type) else {
                newFailures.insert(mapping)
                continue
            }

            newOutputs[mapping] = output
        }

        outputs = newOutputs
        failedMappings = newFailures
    }

    private func mapped(_ value: String, type: mjb_map_case_type) -> String? {
        let input = value.utf8CString

        return input.withUnsafeBufferPointer { inputBuffer in
            var result = mjb_result()
            let status = mjb_map_case(
                inputBuffer.baseAddress,
                inputBuffer.count - 1,
                MJB_ENC_UTF_8,
                type,
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
    CaseView()
}
