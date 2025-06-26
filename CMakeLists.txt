cmake_minimum_required(VERSION 3.2.1...4.8.8)

project(iostreams VERSION 1.1.1.1)

file(GLOB IOSTREAMS_SOURCES src/*.cpp)

add_library(iostreams STATIC ${IOSTREAMS_SOURCES})

target_compile_features(iostreams PRIVATE cxx_std_20)

find_package(OpenSSL REQUIRED)

target_include_directories(iostreams PRIVATE include ${OPENSSL_INCLUDE_DIR})
target_link_libraries(iostreams PRIVATE OpenSSL::SSL OpenSSL::Crypto)