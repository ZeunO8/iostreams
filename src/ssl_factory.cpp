#include <iostreams/ssl_factory.hpp>
#include <iostreams/platform.hpp>
#include <openssl/x509v3.h>
using namespace iostreams;
SSL_CTX* ssl_factory::createClient(const std::string& host)
{
	SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
	{
		throw std::runtime_error("failed to create SSL context");
	}
    X509_VERIFY_PARAM *param = SSL_CTX_get0_param(ctx);
    X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    X509_VERIFY_PARAM_set1_host(param, host.c_str(), 0);
	return ctx;
}
SSL_CTX* ssl_factory::createServer()
{
	SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_server_method());
	if (!ssl_ctx)
	{
		throw std::runtime_error("failed to create SSL context");
	}
	const int kBits = 2048;
	const int kCertDuration = 365;
	RSA* rsa = nullptr;
	BIGNUM* bne = nullptr;
	X509* x509 = nullptr;
	EVP_PKEY* pkey = nullptr;
	BIO* certBio = nullptr;
	BIO* keyBio = nullptr;
	int ret;
	bne = BN_new();
	ret = BN_set_word(bne, RSA_F4);
	if (ret != 1)
	{
		goto cleanup;
	}
	rsa = RSA_new();
	ret = RSA_generate_key_ex(rsa, kBits, bne, nullptr);
	if (ret != 1)
	{
		goto cleanup;
	}
	pkey = EVP_PKEY_new();
	if (!EVP_PKEY_assign_RSA(pkey, rsa))
	{
		goto cleanup;
	}
	{
		rsa = nullptr;
		x509 = X509_new();
		X509_set_version(x509, 2);
		ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
		X509_gmtime_adj(X509_get_notBefore(x509), 0);
		X509_gmtime_adj(X509_get_notAfter(x509), 60 * 60 * 24 * kCertDuration);
		X509_set_pubkey(x509, pkey);
		X509_NAME* name = X509_get_subject_name(x509);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"localhost", -1, -1, 0);
		X509_set_issuer_name(x509, name);

		X509_EXTENSION* ext = X509V3_EXT_conf_nid(nullptr, nullptr, NID_subject_alt_name, "DNS:localhost,IP:127.0.0.1");
		if (ext)
		{
			X509_add_ext(x509, ext, -1);
			X509_EXTENSION_free(ext);
		}

		if (!X509_sign(x509, pkey, EVP_sha256()))
		{
			goto cleanup;
		}
		certBio = BIO_new(BIO_s_mem());
		keyBio = BIO_new(BIO_s_mem());
		if (!PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr))
		{
			goto cleanup;
		}
		if (!PEM_write_bio_X509(certBio, x509))
		{
			goto cleanup;
		}
		{
			if (SSL_CTX_use_certificate(ssl_ctx, x509) != 1 ||
				SSL_CTX_use_PrivateKey(ssl_ctx, pkey) != 1)
			{
				throw std::runtime_error("Failed to assign certificate or private key to SSL context");
			}
		}
	}
cleanup:
	if (bne)
		BN_free(bne);
	if (rsa)
		RSA_free(rsa);
	if (pkey)
		EVP_PKEY_free(pkey);
	if (x509)
		X509_free(x509);
	if (certBio)
		BIO_free(certBio);
	if (keyBio)
		BIO_free(keyBio);
	return ssl_ctx;
}
// SSL_CTX* ssl_factory::createServer(const interfaces::IFile& cert, const interfaces::IFile& key)
// {
// 	std::string certString;
// 	certString.resize(cert.size());
// 	((interfaces::IFile&)cert).readBytes(0, certString.size(), certString.data());
// 	std::string keyString;
// 	keyString.resize(key.size());
// 	((interfaces::IFile&)key).readBytes(0, keyString.size(), keyString.data());
// 	return createServer(certString, keyString);
// }
SSL_CTX* ssl_factory::createServer(const std::string& cert, const std::string& key)
{
	SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
	if (!ctx)
	{
		throw std::runtime_error("failed to create SSL context");
	}
	BIO* certBio = BIO_new_mem_buf(cert.data(), static_cast<int>(cert.size()));
	X509* x509 = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
	BIO_free(certBio);
	if (!x509)
	{
		SSL_CTX_free(ctx);
		throw std::runtime_error("Failed to load X509 certificate");
	}
	if (SSL_CTX_use_certificate(ctx, x509) != 1)
	{
		X509_free(x509);
		SSL_CTX_free(ctx);
		throw std::runtime_error("Failed to assign certificate to SSL context");
	}
	BIO* keyBio = BIO_new_mem_buf(key.data(), static_cast<int>(key.size()));
	EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);
	BIO_free(keyBio);
	if (!pkey)
	{
		X509_free(x509);
		SSL_CTX_free(ctx);
		throw std::runtime_error("Failed to load private key");
	}
	if (SSL_CTX_use_PrivateKey(ctx, pkey) != 1)
	{
		EVP_PKEY_free(pkey);
		X509_free(x509);
		SSL_CTX_free(ctx);
		throw std::runtime_error("Failed to assign private key to SSL context");
	}
	if (SSL_CTX_check_private_key(ctx) != 1)
	{
		EVP_PKEY_free(pkey);
		X509_free(x509);
		SSL_CTX_free(ctx);
		throw std::runtime_error("Private key does not match certificate");
	}
	EVP_PKEY_free(pkey);
	X509_free(x509);
	return ctx;
}
