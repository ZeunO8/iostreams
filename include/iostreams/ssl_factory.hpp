#pragma once
#include <openssl/ssl.h>
#include <string>
namespace iostreams
{
    struct ssl_factory
    {
        static SSL_CTX* createClient(const std::string& host);
        static SSL_CTX* createServer();
        // static SSL_CTX* createServer(const interfaces::IFile &cert, const interfaces::IFile &key);
        static SSL_CTX* createServer(const std::string &cert, const std::string &key);
    };
}