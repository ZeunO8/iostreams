import SwiftUI

struct HttpResponse {
    let `protocol`: String
    let version: String
    let statusCode: String
    let statusText: String
    let headersJson: String
    let body: String
    let elapsedMs: Double
    let bodySize: Int
}

@MainActor
class NetworkViewModel: ObservableObject {
    @Published var response: HttpResponse?
    @Published var isLoading = false
    @Published var errorMessage: String?

    func fetchUrl(_ url: String) {
        isLoading = true
        errorMessage = nil

        DispatchQueue.global(qos: .userInitiated).async {
            var ioResponse = io_http_get(url)
            defer { io_http_response_free(&ioResponse) }

            let httpResponse = HttpResponse(
                protocol: String(cString: ioResponse.protocol),
                version: String(cString: ioResponse.version),
                statusCode: String(cString: ioResponse.statusCode),
                statusText: String(cString: ioResponse.statusText),
                headersJson: String(cString: ioResponse.headers_json),
                body: String(cString: ioResponse.body),
                elapsedMs: ioResponse.elapsed_ms,
                bodySize: Int(ioResponse.body_size)
            )

            DispatchQueue.main.async {
                self.response = httpResponse
                self.isLoading = false
            }
        }
    }
}

struct StatRow: View {
    let label: String
    let value: String

    var body: some View {
        HStack {
            Text(label)
                .font(.system(.body, design: .monospaced))
                .foregroundColor(.secondary)
            Spacer()
            Text(value)
                .font(.system(.body, design: .monospaced))
                .fontWeight(.medium)
        }
        .padding(.vertical, 4)
    }
}

struct ContentView: View {
    @StateObject private var viewModel = NetworkViewModel()
    @State private var url = "https://google.com"

    var body: some View {
        NavigationView {
            Form {
                Section("Request") {
                    HStack {
                        TextField("URL", text: $url)
                            .textFieldStyle(.roundedBorder)
                        Button("Fetch") {
                            viewModel.fetchUrl(url)
                        }
                        .disabled(viewModel.isLoading)
                    }
                }

                if viewModel.isLoading {
                    Section("Loading") {
                        HStack {
                            Spacer()
                            ProgressView()
                            Spacer()
                        }
                        .padding()
                    }
                }

                if let response = viewModel.response {
                    Section("Response Stats") {
                        StatRow(label: "Library", value: String(cString: io_version_string()))
                        StatRow(label: "Protocol", value: response.`protocol`)
                        StatRow(label: "Version", value: response.version)
                        StatRow(label: "Status", value: "\(response.statusCode) \(response.statusText)")
                        StatRow(label: "Time", value: String(format: "%.0f ms", response.elapsedMs))
                        StatRow(label: "Body Size", value: "\(response.bodySize) bytes")
                    }

                    if !response.headersJson.isEmpty && response.headersJson != "{}" {
                        Section("Headers") {
                            Text(response.headersJson)
                                .font(.system(.caption, design: .monospaced))
                                .textSelection(.enabled)
                        }
                    }

                    if !response.body.isEmpty {
                        Section("Body (first 500 chars)") {
                            Text(String(response.body.prefix(500)))
                                .font(.system(.caption, design: .monospaced))
                                .textSelection(.enabled)
                        }
                    }
                }

                if let error = viewModel.errorMessage {
                    Section("Error") {
                        Text(error)
                            .foregroundColor(.red)
                    }
                }
            }
            .navigationTitle("iostreams Test")
        }
    }
}

@main
struct IostreamsTestApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
