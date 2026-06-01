# iOS Test App

Swift UI application that tests iostreams library on iOS Simulator.

## What it does

- Makes an HTTP GET request using iostreams' `http_client::restSync()` to any URL (default: `https://google.com`)
- Displays response stats: protocol, version, status code, response time, body size
- Shows response headers and body preview

## Prerequisites

1. Install iOS Simulator runtime (if not already installed):
   - Open Xcode
   - Go to Xcode > Settings > Platforms
   - Click '+' and install "iOS Simulator"

2. Build iostreams for iOS Simulator first:
   ```bash
   cmake -S . -B build-ios-sim -DCMAKE_TOOLCHAIN_FILE=ios-sim.toolchain.cmake
   cmake --build build-ios-sim
   ```

## Build & Run

```bash
# Configure
cmake -S tests/ios_test_app -B tests/ios_test_app/build -G Xcode -DCMAKE_TOOLCHAIN_FILE=ios-sim.toolchain.cmake

# Build
cmake --build tests/ios_test_app/build

# Build + launch in simulator
cmake --build tests/ios_test_app/build --target run_simulator
```

## How it works

```
Swift UI App
    |
    v
IostreamsTestApp-Bridging-Header.h
    |
    v
iostreams_bridge.h / .cpp  (C wrapper)
    |
    v
iostreams source files (http_client, tcp_client, ssl_factory, etc.)
    |
    v
OpenSSL 1.1.1w + libarchive (from build-ios-sim/_deps)
```

The app uses a C-compatible bridge (`extern "C"`) to expose iostreams' C++ HTTP client to Swift. The bridge handles:
- Making the HTTP request
- Timing the request
- Converting C++ types to C strings for Swift interop
- Memory management via `io_http_response_free()`

## File structure

```
tests/ios_test_app/
├── CMakeLists.txt              # Build configuration
├── RunSimulator.cmake          # Simulator launch script
├── Sources/
│   ├── IostreamsTestApp.swift  # SwiftUI app
│   ├── IostreamsTestApp-Bridging-Header.h
│   ├── iostreams_bridge.h      # C API header
│   └── iostreams_bridge.cpp    # C++ -> C wrapper
└── Resources/
    └── Info.plist              # App bundle metadata
```
