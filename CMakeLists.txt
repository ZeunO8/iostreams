cmake_minimum_required(VERSION 3.2.1...4.8.8)

project(iostreams VERSION 1.2.0.0)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
add_compile_definitions(LIBARCHIVE_STATIC)
set(BUILD_SHARED_LIBS OFF)
set(ENABLE_TEST OFF)
set(ENABLE_COVERAGE OFF)
set(ENABLE_INSTALL OFF)

include(FetchContent)

file(GLOB IOSTREAMS_SOURCES src/*.cpp)

add_library(iostreams STATIC ${IOSTREAMS_SOURCES})

target_compile_features(iostreams PRIVATE cxx_std_20)

if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS TRUE)
    add_compile_definitions(IOS)
    message(STATUS "FetchContent: openssl-cmake for iOS")
    FetchContent_Declare(openssl_cmake
        GIT_REPOSITORY https://github.com/janbar/openssl-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(openssl_cmake)
    add_library(OpenSSL::SSL ALIAS ssl)
    add_library(OpenSSL::Crypto ALIAS crypto)
    set(OPENSSL_INCLUDE_DIR ${openssl_cmake_SOURCE_DIR}/include ${openssl_cmake_BINARY_DIR}/include)
elseif(ANDROID)
    FetchContent_Declare(android_openssl
        GIT_REPOSITORY https://github.com/KDAB/android_openssl.git
        GIT_TAG master
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
else()
    find_package(OpenSSL REQUIRED)
endif()

target_include_directories(iostreams PRIVATE include ${OPENSSL_INCLUDE_DIR})
target_link_libraries(iostreams PRIVATE OpenSSL::SSL OpenSSL::Crypto)

message(STATUS "FetchContent: libarchive")

set(ENABLE_WERROR OFF)
FetchContent_Declare(libarchive
    GIT_REPOSITORY https://github.com/ZeunO8/libarchive.git
    GIT_TAG master)
FetchContent_MakeAvailable(libarchive)

if(MSVC)
    set(CFLAGS "${CFLAGS} /wd4090")
    set(CXXFLAGS "${CXXFLAGS} /wd4090")
else()
    set(CFLAGS "${CFLAGS} -w -Wno-cast-qual")
    set(CXXFLAGS "${CXXFLAGS} -w -Wno-cast-qual")
endif()

target_include_directories(iostreams PRIVATE ${libarchive_SOURCE_DIR}/libarchive)
target_link_libraries(iostreams PRIVATE archive_static)

include(./Tests.cmake)
