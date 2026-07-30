//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct FilterView: View {
    private enum FilterOption: String, CaseIterable, Identifiable {
        case normalize = "Normalize"
        case spaces = "Spaces"
        case collapseSpaces = "Collapse Spaces"
        case controls = "Controls"
        case numeric = "Numeric"
        case limitCombining = "Limit Combining"

        var id: Self {
            self
        }

        var flag: mjb_filter_flags {
            switch self {
            case .normalize:
                MJB_FILTER_NORMALIZE
            case .spaces:
                MJB_FILTER_SPACES
            case .collapseSpaces:
                MJB_FILTER_COLLAPSE_SPACES
            case .controls:
                MJB_FILTER_CONTROLS
            case .numeric:
                MJB_FILTER_NUMERIC
            case .limitCombining:
                MJB_FILTER_LIMIT_COMBINING
            }
        }

        var description: String {
            switch self {
            case .normalize:
                "Normalize the text to NFC before applying other filters."
            case .spaces:
                "Convert Unicode whitespace to ASCII spaces."
            case .collapseSpaces:
                "Replace each run of whitespace with one ASCII space."
            case .controls:
                "Remove control characters except common whitespace controls."
            case .numeric:
                "Convert decimal digits from other scripts to ASCII digits."
            case .limitCombining:
                "Limit consecutive combining marks."
            }
        }
    }

    @State private var input = ""
    @State private var output = ""
    @State private var selectedOptions: Set<FilterOption> = []
    @State private var filteringFailed = false

    private let columns = [
        GridItem(.flexible(), alignment: .topLeading),
        GridItem(.flexible(), alignment: .topLeading),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            ToolHeader(
                "Filter",
                description: "Apply one or more Unicode text filters."
            )

            GroupBox {
                LazyVGrid(columns: columns, alignment: .leading, spacing: 12) {
                    ForEach(FilterOption.allCases) { option in
                        Toggle(isOn: selectionBinding(for: option)) {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(option.rawValue)

                                Text(option.description)
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                        .toggleStyle(.checkbox)
                        .frame(maxWidth: .infinity, alignment: .leading)
                    }
                }
                .padding(.vertical, 4)
            } label: {
                HStack {
                    Text("Filters")
                        .font(.headline)

                    Spacer()

                    Button("Clear All") {
                        selectedOptions.removeAll()
                    }
                    .disabled(selectedOptions.isEmpty)
                }
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

            GroupBox {
                TextEditor(text: .constant(output))
                    .font(.body)
                    .frame(minHeight: 120)
                    .accessibilityLabel("Filtered output")
            } label: {
                HStack {
                    Text("Filtered Output")
                        .font(.headline)

                    Spacer()

                    Text(selectionSummary)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            if filteringFailed {
                Label(
                    "The input could not be filtered.",
                    systemImage: "exclamationmark.triangle"
                )
                .foregroundStyle(.red)
            }
        }
        .frame(minWidth: 360, maxWidth: 760, maxHeight: .infinity, alignment: .top)
        .frame(maxWidth: .infinity)
        .padding()
        .onChange(of: input) {
            filter()
        }
        .onChange(of: selectedOptions) {
            filter()
        }
    }

    private var filters: mjb_filter_flags {
        let rawValue = selectedOptions.reduce(MJB_FILTER_NONE.rawValue) { value, option in
            value | option.flag.rawValue
        }

        return mjb_filter_flags(rawValue: rawValue)
    }

    private var selectionSummary: String {
        if selectedOptions.isEmpty {
            return "None"
        }

        return "\(selectedOptions.count) selected"
    }

    private func selectionBinding(for option: FilterOption) -> Binding<Bool> {
        Binding {
            selectedOptions.contains(option)
        } set: { isSelected in
            if isSelected {
                selectedOptions.insert(option)
            } else {
                selectedOptions.remove(option)
            }
        }
    }

    private func filter() {
        guard let filtered = filtered(input, filters: filters) else {
            output = ""
            filteringFailed = true
            return
        }

        output = filtered
        filteringFailed = false
    }

    private func filtered(_ value: String, filters: mjb_filter_flags) -> String? {
        MojibakeString.transform(value) { input, byteLength, result in
            mjb_filter(
                input,
                byteLength,
                MJB_ENC_UTF_8,
                filters,
                MJB_ENC_UTF_8,
                result
            )
        }
    }
}

#Preview {
    FilterView()
}
