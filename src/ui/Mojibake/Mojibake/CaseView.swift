//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct CaseView: View {
    private enum CaseLocale: String, CaseIterable, Identifiable {
        case english = "English"
        case turkish = "Turkish"
        case azerbaijani = "Azerbaijani"
        case lithuanian = "Lithuanian"

        var id: Self {
            self
        }

        var value: mjb_locale {
            switch self {
            case .english:
                MJB_LOCALE_EN
            case .turkish:
                MJB_LOCALE_TR
            case .azerbaijani:
                MJB_LOCALE_AZ
            case .lithuanian:
                MJB_LOCALE_LT
            }
        }

        var constantName: String {
            switch self {
            case .english:
                "MJB_LOCALE_EN"
            case .turkish:
                "MJB_LOCALE_TR"
            case .azerbaijani:
                "MJB_LOCALE_AZ"
            case .lithuanian:
                "MJB_LOCALE_LT"
            }
        }

        var description: String {
            switch self {
            case .english:
                "Use the default non-Turkic Unicode mappings."
            case .turkish:
                "Apply Turkish dotted-I casing and Turkic case folding."
            case .azerbaijani:
                "Apply Azerbaijani dotted-I casing and Turkic case folding."
            case .lithuanian:
                "Apply Lithuanian dot-above casing with default case folding."
            }
        }
    }

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
    @State private var selectedLocale = CaseLocale.english
    @State private var outputs: [CaseMapping: String] = [:]
    @State private var failedMappings: Set<CaseMapping> = []
    @State private var localeErrorMessage: String?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                ToolHeader(
                    "Case",
                    description: "Apply every Unicode case mapping with locale tailoring."
                )

                VStack(alignment: .leading, spacing: 6) {
                    HStack {
                        Text("Casing Locale")
                            .font(.headline)

                        Spacer()

                        Picker("Casing Locale", selection: $selectedLocale) {
                            ForEach(CaseLocale.allCases) { locale in
                                Text(locale.rawValue)
                                    .tag(locale)
                            }
                        }
                        .labelsHidden()
                        .pickerStyle(.menu)

                        Text(selectedLocale.constantName)
                            .font(.caption.monospaced())
                            .foregroundStyle(.secondary)
                    }

                    Text(selectedLocale.description)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                if let localeErrorMessage {
                    Label(localeErrorMessage, systemImage: "exclamationmark.triangle")
                        .foregroundStyle(.red)
                }

                GroupBox {
                    TextEditor(text: $input)
                        .font(.body)
                        .frame(minHeight: 120)
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
        .onChange(of: selectedLocale) {
            mapCases()
        }
    }

    private var inputSummary: String {
        MojibakeCounting.summary(for: input)
    }

    private func mapCases() {
        var newOutputs: [CaseMapping: String] = [:]
        var newFailures: Set<CaseMapping> = []
        let previousLocale = mjb_get_locale()
        let localeStatus = mjb_set_locale(selectedLocale.value)

        guard localeStatus == MJB_STATUS_OK else {
            outputs = [:]
            failedMappings = Set(CaseMapping.allCases)
            localeErrorMessage = MojibakeFormatting.statusMessage(
                operation: "mjb_set_locale",
                status: localeStatus
            )
            return
        }

        defer {
            _ = mjb_set_locale(previousLocale)
        }

        localeErrorMessage = nil

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
        MojibakeString.transform(value) { input, byteLength, result in
            mjb_map_case(
                input,
                byteLength,
                MJB_ENC_UTF_8,
                MJB_MALFORMED_STOP,
                type,
                MJB_ENC_UTF_8,
                result,
                nil
            )
        }
    }
}

#Preview {
    CaseView()
}
