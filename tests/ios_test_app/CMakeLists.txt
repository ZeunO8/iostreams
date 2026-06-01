cmake_minimum_required(VERSION 3.20)
project(iostreams_test_app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_Swift_LANGUAGE_VERSION 5)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "This test app requires iOS toolchain. Use -DCMAKE_TOOLCHAIN_FILE=../../ios-sim.toolchain.cmake or ios-device.toolchain.cmake")
endif()

enable_language(Swift)

set(IOS_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../build-ios-sim" CACHE PATH "Path to iOS build directory with dependencies")

if(NOT EXISTS "${IOS_BUILD_DIR}/_deps/openssl_cmake-src")
    message(FATAL_ERROR "iOS build directory not found at ${IOS_BUILD_DIR}. Run the main iOS build first: cmake -S . -B build-ios-sim -DCMAKE_TOOLCHAIN_FILE=ios-sim.toolchain.cmake && cmake --build build-ios-sim")
endif()

set(OPENSSL_INCLUDE_DIR
    ${IOS_BUILD_DIR}/_deps/openssl_cmake-src/include
    ${IOS_BUILD_DIR}/_deps/openssl_cmake-build/include
)

add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES
    IMPORTED_LOCATION "${IOS_BUILD_DIR}/_deps/openssl_cmake-build/ssl/libssl.a"
)

add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES
    IMPORTED_LOCATION "${IOS_BUILD_DIR}/_deps/openssl_cmake-build/crypto/libcrypto.a"
)

add_library(archive_static STATIC IMPORTED)
set_target_properties(archive_static PROPERTIES
    IMPORTED_LOCATION "${IOS_BUILD_DIR}/_deps/libarchive-build/libarchive/libarchive.a"
    INTERFACE_INCLUDE_DIRECTORIES "${IOS_BUILD_DIR}/_deps/libarchive-src/libarchive"
)

set(APP_NAME IostreamsTestApp)

add_library(iostreams_bridge STATIC
    Sources/iostreams_bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/http_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/http_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/tcp_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/tcp_iostream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/tcp_streambuf.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/ssl_factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/socket_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/resolve_host_or_ip_to_ip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/populate_addr_from_ip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/string_is_ipv4.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/system_dns.cpp
)

target_include_directories(iostreams_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${OPENSSL_INCLUDE_DIR}
)

target_compile_definitions(iostreams_bridge PRIVATE
    LIBARCHIVE_STATIC
)

target_link_libraries(iostreams_bridge PRIVATE
    ssl
    crypto
    archive_static
)

set_target_properties(iostreams_bridge PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
)

set(SWIFT_SOURCES
    Sources/IostreamsTestApp.swift
)

set(BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Sources/IostreamsTestApp-Bridging-Header.h")

add_executable(${APP_NAME}
    ${SWIFT_SOURCES}
)

target_sources(${APP_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist"
)

target_include_directories(${APP_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources
)

target_link_libraries(${APP_NAME} PRIVATE
    iostreams_bridge
)

target_compile_options(${APP_NAME} PRIVATE
    -import-objc-header "${BRIDGING_HEADER}"
)

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist"
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)

if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set_target_properties(${APP_NAME} PROPERTIES
        XCODE_ATTRIBUTE_SDKROOT "iphonesimulator"
    )
else()
    set_target_properties(${APP_NAME} PROPERTIES
        XCODE_ATTRIBUTE_SDKROOT "iphoneos"
    )
endif()

if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    find_program(XCRUN xcrun REQUIRED)

    set(APP_BUNDLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/Debug-iphonesimulator/${APP_NAME}.app")

    add_custom_target(run_simulator
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo "Launching iOS Simulator with ${APP_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND}
            -DAPP_PATH=${APP_BUNDLE_DIR}
            -DXCRUN=${XCRUN}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/RunSimulator.cmake
        DEPENDS ${APP_NAME}
        COMMENT "Running ${APP_NAME} in iOS Simulator"
        VERBATIM
    )
endif()
