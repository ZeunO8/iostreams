#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#if defined(_WIN32)
#include <windows.h>
#elif defined(MACOS) || defined(IOS)
#include <fcntl.h>
#include <mach-o/dyld.h>
#include <sys/event.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <sys/inotify.h>
#endif
#if defined(__linux) || defined(MACOS) || defined(IOS)
#include <arpa/inet.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#endif
#define STANDARD std
#define GLMATH glm
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
