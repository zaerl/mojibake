//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct ContentView: View {
    private enum Tool: String, Identifiable {
        case codepoint = "Codepoint"
        case characters = "Characters"
        case emoji = "Emoji"
        case security = "Security"
        case idna = "IDNA"
        case collation = "Collation"
        case caseless = "Caseless Match"
        case encoding = "Encoding"
        case `case` = "Case"
        case breaks = "Breaks"
        case bidi = "Bidi"
        case terminalWidth = "Terminal Width"
        case normalize = "Normalize"
        case filter = "Filter"

        var id: Self {
            self
        }

        var title: String {
            rawValue
        }

        var systemImage: String {
            switch self {
            case .codepoint:
                "character.cursor.ibeam"
            case .characters:
                "text.quote"
            case .emoji:
                "face.smiling"
            case .security:
                "lock.shield"
            case .idna:
                "globe"
            case .collation:
                "arrow.up.arrow.down"
            case .caseless:
                "equal.circle"
            case .encoding:
                "memorychip"
            case .case:
                "textformat.abc"
            case .breaks:
                "text.word.spacing"
            case .bidi:
                "arrow.left.arrow.right"
            case .terminalWidth:
                "ruler"
            case .normalize:
                "textformat"
            case .filter:
                "line.3.horizontal.decrease"
            }
        }
    }

    private enum SidebarSection: String, CaseIterable, Identifiable {
        case characterData = "Character Data"
        case textProcessing = "Text Processing"
        case textAnalysis = "Text Analysis"
        case comparison = "Comparison"
        case security = "Security"

        var id: Self {
            self
        }

        var tools: [Tool] {
            switch self {
            case .characterData:
                [.codepoint, .characters, .emoji]
            case .textProcessing:
                [.case, .normalize, .filter]
            case .textAnalysis:
                [.encoding, .breaks, .bidi, .terminalWidth]
            case .comparison:
                [.collation, .caseless]
            case .security:
                [.security, .idna]
            }
        }
    }

    @State private var selection: Tool? = .codepoint
    @State private var requestedCodepoint: String?
    @State private var expandedSections = Set(SidebarSection.allCases)

    var body: some View {
        NavigationSplitView {
            List(selection: $selection) {
                ForEach(SidebarSection.allCases) { section in
                    Section(
                        section.rawValue,
                        isExpanded: expansionBinding(for: section)
                    ) {
                        ForEach(section.tools) { tool in
                            Label(tool.title, systemImage: tool.systemImage)
                                .tag(tool)
                        }
                    }
                }
            }
            .navigationSplitViewColumnWidth(min: 160, ideal: 180)
        } detail: {
            switch selection {
            case .codepoint:
                CodepointView(requestedCodepoint: $requestedCodepoint)
            case .characters:
                CharactersView(onCodepointSelected: showCodepoint)
            case .emoji:
                EmojiView()
            case .security:
                SecurityView()
            case .idna:
                IDNAView()
            case .collation:
                CollationView()
            case .caseless:
                CaselessMatchingView()
            case .encoding:
                EncodingInspectorView()
            case .case:
                CaseView()
            case .breaks:
                BreaksView()
            case .bidi:
                BidiView()
            case .terminalWidth:
                TerminalWidthView()
            case .normalize:
                NormalizationView()
            case .filter:
                FilterView()
            case nil:
                ContentUnavailableView("Select a tool", systemImage: "sidebar.left")
            }
        }
    }

    private func expansionBinding(for section: SidebarSection) -> Binding<Bool> {
        Binding {
            expandedSections.contains(section)
        } set: { isExpanded in
            if isExpanded {
                expandedSections.insert(section)
            } else {
                expandedSections.remove(section)
            }
        }
    }

    private func showCodepoint(_ codepoint: String) {
        requestedCodepoint = codepoint
        selection = .codepoint
    }
}

#Preview {
    ContentView()
}
