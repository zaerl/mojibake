//
//  The Mojibake library
//
//  This file is distributed under the MIT License. See LICENSE for details.
//

import SwiftUI

@main
struct MojibakeApp: App {
    private var windowTitle: String {
        guard let version = mjb_version() else {
            return "Mojibake"
        }

        return "Mojibake v\(String(cString: version))"
    }

    var body: some Scene {
        WindowGroup(windowTitle) {
            ContentView()
        }
    }
}
