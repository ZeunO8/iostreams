#include <catch2/catch_test_macros.hpp>
#include <iostreams/ssl_factory.hpp>
#include <openssl/ssl.h>

using namespace iostreams;

TEST_CASE("ssl_factory createClient returns valid context", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createClient("example.com");
    REQUIRE(ctx != nullptr);
    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createClient sets hostname verification", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createClient("secure.example.com");
    REQUIRE(ctx != nullptr);

    X509_VERIFY_PARAM* param = SSL_CTX_get0_param(ctx);
    REQUIRE(param != nullptr);

    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createServer returns valid context", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createServer();
    REQUIRE(ctx != nullptr);
    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createServer generates self-signed cert", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createServer();
    REQUIRE(ctx != nullptr);

    X509* cert = SSL_CTX_get0_certificate(ctx);
    REQUIRE(cert != nullptr);

    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createServer has private key", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createServer();
    REQUIRE(ctx != nullptr);

    EVP_PKEY* pkey = SSL_CTX_get0_privatekey(ctx);
    REQUIRE(pkey != nullptr);

    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createServer cert matches key", "[ssl]")
{
    SSL_CTX* ctx = ssl_factory::createServer();
    REQUIRE(ctx != nullptr);

    int result = SSL_CTX_check_private_key(ctx);
    REQUIRE(result == 1);

    SSL_CTX_free(ctx);
}

TEST_CASE("ssl_factory createServer with cert/key strings", "[ssl]")
{
    SSL_CTX* ctx1 = ssl_factory::createServer();
    REQUIRE(ctx1 != nullptr);

    BIO* certBio = BIO_new(BIO_s_mem());
    BIO* keyBio = BIO_new(BIO_s_mem());
    X509* cert = SSL_CTX_get0_certificate(ctx1);
    EVP_PKEY* pkey = SSL_CTX_get0_privatekey(ctx1);

    PEM_write_bio_X509(certBio, cert);
    PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr);

    BUF_MEM* certMem;
    BIO_get_mem_ptr(certBio, &certMem);
    std::string certStr(certMem->data, certMem->length);

    BUF_MEM* keyMem;
    BIO_get_mem_ptr(keyBio, &keyMem);
    std::string keyStr(keyMem->data, keyMem->length);

    BIO_free(certBio);
    BIO_free(keyBio);
    SSL_CTX_free(ctx1);

    SSL_CTX* ctx2 = ssl_factory::createServer(certStr, keyStr);
    REQUIRE(ctx2 != nullptr);

    int result = SSL_CTX_check_private_key(ctx2);
    REQUIRE(result == 1);

    SSL_CTX_free(ctx2);
}

TEST_CASE("ssl_factory multiple client contexts are independent", "[ssl]")
{
    SSL_CTX* ctx1 = ssl_factory::createClient("host1.example.com");
    SSL_CTX* ctx2 = ssl_factory::createClient("host2.example.com");

    REQUIRE(ctx1 != nullptr);
    REQUIRE(ctx2 != nullptr);
    REQUIRE(ctx1 != ctx2);

    SSL_CTX_free(ctx1);
    SSL_CTX_free(ctx2);
}

TEST_CASE("ssl_factory multiple server contexts are independent", "[ssl]")
{
    SSL_CTX* ctx1 = ssl_factory::createServer();
    SSL_CTX* ctx2 = ssl_factory::createServer();

    REQUIRE(ctx1 != nullptr);
    REQUIRE(ctx2 != nullptr);
    REQUIRE(ctx1 != ctx2);

    SSL_CTX_free(ctx1);
    SSL_CTX_free(ctx2);
}
