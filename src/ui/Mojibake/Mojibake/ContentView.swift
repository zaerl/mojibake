//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

struct ContentView: View {
    private enum Section: String, CaseIterable, Identifiable {
        case codepoint
        case normalize

        var id: Self {
            self
        }

        var title: String {
            rawValue.capitalized
        }

        var systemImage: String {
            switch self {
            case .codepoint:
                "character.cursor.ibeam"
            case .normalize:
                "textformat"
            }
        }
    }

    @State private var selection: Section? = .codepoint

    var body: some View {
        NavigationSplitView {
            List(Section.allCases, selection: $selection) { section in
                Label(section.title, systemImage: section.systemImage)
                    .tag(section)
            }
            .navigationSplitViewColumnWidth(min: 160, ideal: 180)
        } detail: {
            switch selection {
            case .codepoint:
                CodepointView()
            case .normalize:
                ContentUnavailableView(
                    "Normalize",
                    systemImage: "textformat",
                    description: Text("This tool will be added later.")
                )
            case nil:
                ContentUnavailableView("Select a tool", systemImage: "sidebar.left")
            }
        }
    }
}

#Preview {
    ContentView()
}
