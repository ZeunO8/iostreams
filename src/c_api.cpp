#include <iostreams/c_api.h>
#include <iostreams/tcp_client.hpp>
#include <iostreams/tcp_server.hpp>
#include <iostreams/tcp_iostream.hpp>
#include <iostreams/udp_client.hpp>
#include <iostreams/udp_server.hpp>
#include <iostreams/udp_iostream.hpp>
#include <iostreams/udpmc_sender.hpp>
#include <iostreams/udpmc_receiver.hpp>
#include <iostreams/ssl_factory.hpp>
#include <iostreams/http_client.hpp>
#include <iostreams/http_common.hpp>
#include <iostreams/socket_init.hpp>
#include <iostreams/get_local_ip.hpp>
#include <iostreams/string_is_ipv4.hpp>
#include <iostreams/resolve_host_or_ip_to_ip.hpp>
#include <iostreams/archive_stream.hpp>
#include <iostreams/Serial.hpp>
#include <cstring>
#include <string>
#include <sstream>
#include <memory>
#include <new>

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

struct bit_array_wrapper {
    const uint8_t* data;
    size_t sz;
    size_t size() const { return sz; }
    uint8_t operator[](size_t i) const { return data[i]; }
};

struct bit_array_wrapper_write {
    uint8_t* data;
    size_t sz;
    size_t size() const { return sz; }
    uint8_t& operator[](size_t i) { return data[i]; }
};

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static char* copy_c_string(const std::string& str) {
    if (str.empty()) {
        char* empty = (char*)malloc(1);
        empty[0] = '\0';
        return empty;
    }
    char* copy = (char*)malloc(str.size() + 1);
    memcpy(copy, str.data(), str.size());
    copy[str.size()] = '\0';
    return copy;
}

static std::string headers_to_json(const iostreams::http::Headers& headers) {
    std::string json = "{";
    bool first = true;
    for (const auto& pair : headers) {
        if (!first) json += ",";
        json += "\"" + pair.first + "\":\"" + pair.second + "\"";
        first = false;
    }
    json += "}";
    return json;
}

/* ============================================================================
 * Opaque Handle Structures
 * ============================================================================ */

struct io_tcp_client {
    iostreams::tcp_client* ptr;
};

struct io_tcp_server {
    iostreams::tcp_server* ptr;
};

struct io_tcp_stream {
    iostreams::streams::tcp_iostream* ptr;
    int fd;
    SSL* ssl;
    size_t client_id;
};

struct io_udp_client {
    iostreams::udp_client* ptr;
};

struct io_udp_server {
    iostreams::udp_server* ptr;
};

struct io_udp_stream {
    std::shared_ptr<iostreams::streams::udp_iostream> ptr;
};

struct io_udpmc_sender {
    iostreams::udpmc_sender* ptr;
};

struct io_udpmc_receiver {
    iostreams::udpmc_receiver* ptr;
};

struct io_serial {
    Serial* ptr;
    bool owns_stream;
};

struct io_ssl_ctx {
    SSL_CTX* ptr;
};

struct io_archive {
    iostreams::archive_stream* ptr;
};

/* ============================================================================
 * Socket Initialization
 * ============================================================================ */

extern "C" void io_socket_init(void) {
    iostreams::socket_init::initialize();
}

/* ============================================================================
 * SSL Context Factory
 * ============================================================================ */

extern "C" io_ssl_ctx_t* io_ssl_create_client_ctx(const char* host) {
    if (!host) return nullptr;
    try {
        auto ctx = iostreams::ssl_factory::createClient(std::string(host));
        auto handle = new (std::nothrow) io_ssl_ctx;
        if (handle) handle->ptr = ctx;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_ssl_ctx_t* io_ssl_create_server_ctx(void) {
    try {
        auto ctx = iostreams::ssl_factory::createServer();
        auto handle = new (std::nothrow) io_ssl_ctx;
        if (handle) handle->ptr = ctx;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_ssl_ctx_t* io_ssl_create_server_ctx_with_certs(const char* cert_path, const char* key_path) {
    if (!cert_path || !key_path) return nullptr;
    try {
        auto ctx = iostreams::ssl_factory::createServer(std::string(cert_path), std::string(key_path));
        auto handle = new (std::nothrow) io_ssl_ctx;
        if (handle) handle->ptr = ctx;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_ssl_free_ctx(io_ssl_ctx_t* ctx) {
    if (ctx && ctx->ptr) {
        SSL_CTX_free(ctx->ptr);
    }
    delete ctx;
}

/* ============================================================================
 * TCP Client
 * ============================================================================ */

extern "C" io_tcp_client_t* io_tcp_client_create(const char* host, int port) {
    if (!host) return nullptr;
    try {
        auto client = new (std::nothrow) iostreams::tcp_client(std::string(host), port);
        auto handle = new (std::nothrow) io_tcp_client;
        if (handle) handle->ptr = client;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_tcp_client_t* io_tcp_client_create_ssl(const char* host, int port, io_ssl_ctx_t* ssl_ctx, int verify_certs) {
    if (!host) return nullptr;
    try {
        SSL_CTX* ctx = ssl_ctx ? ssl_ctx->ptr : nullptr;
        auto client = new (std::nothrow) iostreams::tcp_client(std::string(host), port, ctx, verify_certs != 0);
        auto handle = new (std::nothrow) io_tcp_client;
        if (handle) handle->ptr = client;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_tcp_client_destroy(io_tcp_client_t* client) {
    if (client) {
        delete client->ptr;
        delete client;
    }
}

extern "C" int64_t io_tcp_client_write(io_tcp_client_t* client, const void* data, size_t size) {
    if (!client || !client->ptr || !data) return IO_ERROR;
    try {
        client->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int64_t io_tcp_client_read(io_tcp_client_t* client, void* buffer, size_t size) {
    if (!client || !client->ptr || !buffer) return IO_ERROR;
    try {
        client->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(client->ptr->gcount());
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_client_ssl_upgrade(io_tcp_client_t* client, io_ssl_ctx_t* ctx, io_ssl_ctx_t* ssl) {
    (void)client;
    (void)ctx;
    (void)ssl;
    return IO_ERROR;
}

extern "C" int io_tcp_client_close(io_tcp_client_t* client) {
    if (!client || !client->ptr) return IO_ERROR;
    try {
        return client->ptr->close() ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_client_wait_readable(io_tcp_client_t* client, int timeout_ms) {
    if (!client || !client->ptr) return IO_ERROR;
    try {
        auto* tcp_buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(client->ptr->rdbuf());
        if (!tcp_buf) return IO_ERROR;
        return tcp_buf->wait_readable(timeout_ms) ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_client_wait_writable(io_tcp_client_t* client, int timeout_ms) {
    if (!client || !client->ptr) return IO_ERROR;
    try {
        auto* tcp_buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(client->ptr->rdbuf());
        if (!tcp_buf) return IO_ERROR;
        return tcp_buf->wait_writable(timeout_ms) ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_client_bytes_available(io_tcp_client_t* client) {
    if (!client || !client->ptr) return IO_ERROR;
    try {
        auto* tcp_buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(client->ptr->rdbuf());
        if (!tcp_buf) return IO_ERROR;
        return tcp_buf->bytes_available();
    } catch (...) {
        return IO_ERROR;
    }
}

/* ============================================================================
 * TCP Server
 * ============================================================================ */

extern "C" io_tcp_server_t* io_tcp_server_create(int port) {
    try {
        auto server = new (std::nothrow) iostreams::tcp_server(port);
        auto handle = new (std::nothrow) io_tcp_server;
        if (handle) handle->ptr = server;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_tcp_server_t* io_tcp_server_create_ssl(int port, io_ssl_ctx_t* ssl_ctx, int enable_non_blocking) {
    try {
        SSL_CTX* ctx = ssl_ctx ? ssl_ctx->ptr : nullptr;
        auto server = new (std::nothrow) iostreams::tcp_server(port, false, ctx, enable_non_blocking != 0);
        auto handle = new (std::nothrow) io_tcp_server;
        if (handle) handle->ptr = server;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_tcp_server_t* io_tcp_server_create_bitstream(int port, int bit_stream, io_ssl_ctx_t* ssl_ctx, int enable_non_blocking) {
    try {
        SSL_CTX* ctx = ssl_ctx ? ssl_ctx->ptr : nullptr;
        auto server = new (std::nothrow) iostreams::tcp_server(port, bit_stream != 0, ctx, enable_non_blocking != 0);
        auto handle = new (std::nothrow) io_tcp_server;
        if (handle) handle->ptr = server;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_tcp_server_destroy(io_tcp_server_t* server) {
    if (server) {
        delete server->ptr;
        delete server;
    }
}

extern "C" io_tcp_stream_t* io_tcp_server_accept(io_tcp_server_t* server, size_t* out_client_id) {
    if (!server || !server->ptr) return nullptr;
    try {
        iostreams::tcp_server::ClientTuple* tuple_ptr = nullptr;
        int fd = server->ptr->acceptOne(&tuple_ptr);
        if (fd < 0 || !tuple_ptr) return nullptr;

        auto& [client_fd, serial, iostream] = *tuple_ptr;
        auto stream = new (std::nothrow) io_tcp_stream;
        if (stream) {
            stream->ptr = iostream.get();
            stream->fd = client_fd;
            stream->ssl = nullptr;
            stream->client_id = 0;
        }

        if (out_client_id) {
            *out_client_id = 0;
        }
        return stream;
    } catch (...) {
        return nullptr;
    }
}

extern "C" int io_tcp_server_close_client(io_tcp_server_t* server, size_t client_id) {
    if (!server || !server->ptr) return IO_ERROR;
    try {
        return server->ptr->closeClient(client_id) ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_server_close(io_tcp_server_t* server) {
    if (!server || !server->ptr) return IO_ERROR;
    try {
        return server->ptr->close() ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_server_upgrade_ssl(io_tcp_server_t* server, io_ssl_ctx_t* ssl_ctx) {
    if (!server || !server->ptr || !ssl_ctx) return IO_ERROR;
    try {
        server->ptr->upgradeSSL(ssl_ctx->ptr);
        return IO_SUCCESS;
    } catch (...) {
        return IO_ERROR_SSL;
    }
}

/* ============================================================================
 * TCP Stream
 * ============================================================================ */

extern "C" void io_tcp_stream_destroy(io_tcp_stream_t* stream) {
    delete stream;
}

extern "C" int64_t io_tcp_stream_write(io_tcp_stream_t* stream, const void* data, size_t size) {
    if (!stream || !stream->ptr || !data) return IO_ERROR;
    try {
        stream->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int64_t io_tcp_stream_read(io_tcp_stream_t* stream, void* buffer, size_t size) {
    if (!stream || !stream->ptr || !buffer) return IO_ERROR;
    try {
        stream->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(stream->ptr->gcount());
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_stream_ssl_upgrade(io_tcp_stream_t* stream, io_ssl_ctx_t* ctx, io_ssl_ctx_t* ssl) {
    (void)stream;
    (void)ctx;
    (void)ssl;
    return IO_ERROR;
}

extern "C" int io_tcp_stream_close(io_tcp_stream_t* stream) {
    if (!stream || !stream->ptr) return IO_ERROR;
    try {
        return stream->ptr->close() ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_stream_wait_readable(io_tcp_stream_t* stream, int timeout_ms) {
    if (!stream || !stream->ptr) return IO_ERROR;
    try {
        auto* tcp_buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(stream->ptr->rdbuf());
        if (!tcp_buf) return IO_ERROR;
        return tcp_buf->wait_readable(timeout_ms) ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_tcp_stream_wait_writable(io_tcp_stream_t* stream, int timeout_ms) {
    if (!stream || !stream->ptr) return IO_ERROR;
    try {
        auto* tcp_buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(stream->ptr->rdbuf());
        if (!tcp_buf) return IO_ERROR;
        return tcp_buf->wait_writable(timeout_ms) ? IO_SUCCESS : IO_ERROR;
    } catch (...) {
        return IO_ERROR;
    }
}

/* ============================================================================
 * UDP Client
 * ============================================================================ */

extern "C" io_udp_client_t* io_udp_client_create(const char* host, int port) {
    if (!host) return nullptr;
    try {
        auto client = new (std::nothrow) iostreams::udp_client(std::string(host), port);
        auto handle = new (std::nothrow) io_udp_client;
        if (handle) handle->ptr = client;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_udp_client_destroy(io_udp_client_t* client) {
    if (client) {
        delete client->ptr;
        delete client;
    }
}

extern "C" int64_t io_udp_client_write(io_udp_client_t* client, const void* data, size_t size) {
    if (!client || !client->ptr || !data) return IO_ERROR;
    try {
        client->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int64_t io_udp_client_read(io_udp_client_t* client, void* buffer, size_t size) {
    if (!client || !client->ptr || !buffer) return IO_ERROR;
    try {
        client->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(client->ptr->gcount());
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_udp_client_close(io_udp_client_t* client) {
    if (!client || !client->ptr) return IO_ERROR;
    try {
        return IO_SUCCESS;
    } catch (...) {
        return IO_ERROR;
    }
}

/* ============================================================================
 * UDP Server
 * ============================================================================ */

extern "C" io_udp_server_t* io_udp_server_create(int port) {
    try {
        auto server = new (std::nothrow) iostreams::udp_server(port);
        auto handle = new (std::nothrow) io_udp_server;
        if (handle) handle->ptr = server;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_udp_server_t* io_udp_server_create_bitstream(int port, int bit_stream) {
    try {
        auto server = new (std::nothrow) iostreams::udp_server(port, bit_stream != 0);
        auto handle = new (std::nothrow) io_udp_server;
        if (handle) handle->ptr = server;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_udp_server_destroy(io_udp_server_t* server) {
    if (server) {
        delete server->ptr;
        delete server;
    }
}

extern "C" io_udp_stream_t* io_udp_server_receive(io_udp_server_t* server, int non_block, unsigned int timeout_us) {
    if (!server || !server->ptr) return nullptr;
    try {
        auto stream = server->ptr->receiveOne(non_block != 0, timeout_us);
        if (!stream) return nullptr;
        auto handle = new (std::nothrow) io_udp_stream;
        if (handle) handle->ptr = stream;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_udp_server_close(io_udp_server_t* server) {
    (void)server;
}

/* ============================================================================
 * UDP Stream
 * ============================================================================ */

extern "C" void io_udp_stream_destroy(io_udp_stream_t* stream) {
    delete stream;
}

extern "C" int64_t io_udp_stream_write(io_udp_stream_t* stream, const void* data, size_t size) {
    if (!stream || !stream->ptr || !data) return IO_ERROR;
    try {
        stream->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int64_t io_udp_stream_read(io_udp_stream_t* stream, void* buffer, size_t size) {
    if (!stream || !stream->ptr || !buffer) return IO_ERROR;
    try {
        stream->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(stream->ptr->gcount());
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int io_udp_stream_close(io_udp_stream_t* stream) {
    if (!stream) return IO_ERROR;
    stream->ptr.reset();
    return IO_SUCCESS;
}

/* ============================================================================
 * UDP Multicast Sender
 * ============================================================================ */

extern "C" io_udpmc_sender_t* io_udpmc_sender_create(const char* host, int port) {
    if (!host) return nullptr;
    try {
        auto sender = new (std::nothrow) iostreams::udpmc_sender(std::string(host), port);
        auto handle = new (std::nothrow) io_udpmc_sender;
        if (handle) handle->ptr = sender;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_udpmc_sender_destroy(io_udpmc_sender_t* sender) {
    if (sender) {
        delete sender->ptr;
        delete sender;
    }
}

extern "C" int64_t io_udpmc_sender_write(io_udpmc_sender_t* sender, const void* data, size_t size) {
    if (!sender || !sender->ptr || !data) return IO_ERROR;
    try {
        sender->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" void io_udpmc_sender_close(io_udpmc_sender_t* sender) {
    (void)sender;
}

/* ============================================================================
 * UDP Multicast Receiver
 * ============================================================================ */

extern "C" io_udpmc_receiver_t* io_udpmc_receiver_create(const char* host, int port) {
    if (!host) return nullptr;
    try {
        auto receiver = new (std::nothrow) iostreams::udpmc_receiver(std::string(host), port);
        auto handle = new (std::nothrow) io_udpmc_receiver;
        if (handle) handle->ptr = receiver;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_udpmc_receiver_destroy(io_udpmc_receiver_t* receiver) {
    if (receiver) {
        delete receiver->ptr;
        delete receiver;
    }
}

extern "C" int64_t io_udpmc_receiver_read(io_udpmc_receiver_t* receiver, void* buffer, size_t size) {
    if (!receiver || !receiver->ptr || !buffer) return IO_ERROR;
    try {
        receiver->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(receiver->ptr->gcount());
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" void io_udpmc_receiver_close(io_udpmc_receiver_t* receiver) {
    (void)receiver;
}

/* ============================================================================
 * Serial (Bitstream)
 * ============================================================================ */

extern "C" io_serial_t* io_serial_create(void) {
    auto serial = new (std::nothrow) Serial();
    auto handle = new (std::nothrow) io_serial;
    if (handle) {
        handle->ptr = serial;
        handle->owns_stream = false;
    }
    return handle;
}

extern "C" io_serial_t* io_serial_create_buffer(int64_t size, char* buffer) {
    auto serial = new (std::nothrow) Serial(size, buffer);
    auto handle = new (std::nothrow) io_serial;
    if (handle) {
        handle->ptr = serial;
        handle->owns_stream = false;
    }
    return handle;
}

extern "C" io_serial_t* io_serial_create_count(int count) {
    auto serial = new (std::nothrow) Serial(count != 0);
    auto handle = new (std::nothrow) io_serial;
    if (handle) {
        handle->ptr = serial;
        handle->owns_stream = false;
    }
    return handle;
}

extern "C" void io_serial_destroy(io_serial_t* serial) {
    if (serial) {
        delete serial->ptr;
        delete serial;
    }
}

extern "C" int64_t io_serial_write_bytes(io_serial_t* serial, const void* src, size_t size) {
    if (!serial || !serial->ptr || !src) return IO_ERROR;
    try {
        return serial->ptr->writeBytes((const char*)src, size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" int64_t io_serial_read_bytes(io_serial_t* serial, void* dest, size_t size) {
    if (!serial || !serial->ptr || !dest) return IO_ERROR;
    try {
        return serial->ptr->readBytes((char*)dest, size);
    } catch (...) {
        return IO_ERROR;
    }
}

extern "C" void io_serial_write_byte(io_serial_t* serial, char byte) {
    if (serial && serial->ptr) {
        serial->ptr->writeByte(byte);
    }
}

extern "C" char io_serial_read_byte(io_serial_t* serial) {
    if (serial && serial->ptr) {
        return serial->ptr->readByte();
    }
    return 0;
}

extern "C" void io_serial_write_bit(io_serial_t* serial, int bit) {
    if (serial && serial->ptr) {
        serial->ptr->writeBit(bit != 0);
    }
}

extern "C" int io_serial_read_bit(io_serial_t* serial) {
    if (serial && serial->ptr) {
        return serial->ptr->readBit() ? 1 : 0;
    }
    return 0;
}

extern "C" void io_serial_write_bits(io_serial_t* serial, const uint8_t* bits, size_t index, size_t count) {
    if (serial && serial->ptr && bits) {
        bit_array_wrapper wrapper;
        wrapper.data = bits;
        wrapper.sz = index + count;
        serial->ptr->writeBits(wrapper, index, count);
    }
}

extern "C" void io_serial_read_bits(io_serial_t* serial, uint8_t* bits, size_t index, size_t count) {
    if (serial && serial->ptr && bits) {
        bit_array_wrapper_write wrapper;
        wrapper.data = bits;
        wrapper.sz = index + count;
        serial->ptr->readBits(wrapper, index, count);
    }
}

extern "C" int64_t io_serial_get_write_position(io_serial_t* serial) {
    if (!serial || !serial->ptr) return IO_ERROR;
    return serial->ptr->getWritePosition();
}

extern "C" int64_t io_serial_get_read_position(io_serial_t* serial) {
    if (!serial || !serial->ptr) return IO_ERROR;
    return serial->ptr->getReadPosition();
}

extern "C" void io_serial_set_write_position(io_serial_t* serial, size_t index) {
    if (serial && serial->ptr) {
        serial->ptr->setWritePosition(index);
    }
}

extern "C" void io_serial_set_read_position(io_serial_t* serial, size_t index) {
    if (serial && serial->ptr) {
        serial->ptr->setReadPosition(index);
    }
}

extern "C" int64_t io_serial_get_write_length(io_serial_t* serial) {
    if (!serial || !serial->ptr) return IO_ERROR;
    return serial->ptr->getWriteLength();
}

extern "C" int64_t io_serial_get_read_length(io_serial_t* serial) {
    if (!serial || !serial->ptr) return IO_ERROR;
    return serial->ptr->getReadLength();
}

extern "C" void io_serial_synchronize(io_serial_t* serial) {
    if (serial && serial->ptr) {
        serial->ptr->synchronize();
    }
}

extern "C" int io_serial_is_read_eof(io_serial_t* serial) {
    if (!serial || !serial->ptr) return 0;
    return serial->ptr->is_read_eof() ? 1 : 0;
}

extern "C" int io_serial_is_read_empty(io_serial_t* serial) {
    if (!serial || !serial->ptr) return 0;
    return serial->ptr->is_read_empty() ? 1 : 0;
}

extern "C" int io_serial_did_not_read_whole_size(io_serial_t* serial) {
    if (!serial || !serial->ptr) return 0;
    return serial->ptr->did_not_read_whole_size() ? 1 : 0;
}

extern "C" size_t io_serial_get_last_bytes_read(io_serial_t* serial) {
    if (!serial || !serial->ptr) return 0;
    return serial->ptr->get_last_bytes_read();
}

extern "C" void io_serial_clear_read(io_serial_t* serial) {
    if (serial && serial->ptr) {
        serial->ptr->clearRead();
    }
}

/* ============================================================================
 * Serial over TCP Stream
 * ============================================================================ */

extern "C" io_serial_t* io_serial_create_tcp(io_tcp_stream_t* stream, int bit_stream) {
    if (!stream || !stream->ptr) return nullptr;
    try {
        auto serial = new (std::nothrow) Serial(*stream->ptr, bit_stream != 0);
        auto handle = new (std::nothrow) io_serial;
        if (handle) {
            handle->ptr = serial;
            handle->owns_stream = false;
        }
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" io_serial_t* io_serial_create_tcp_client(io_tcp_client_t* client, int bit_stream) {
    if (!client || !client->ptr) return nullptr;
    try {
        auto serial = new (std::nothrow) Serial(*client->ptr, bit_stream != 0);
        auto handle = new (std::nothrow) io_serial;
        if (handle) {
            handle->ptr = serial;
            handle->owns_stream = false;
        }
        return handle;
    } catch (...) {
        return nullptr;
    }
}

/* ============================================================================
 * HTTP Client
 * ============================================================================ */

static io_http_response_t make_http_response(const iostreams::http::http_response& response) {
    io_http_response_t result = {};
    result.protocol = copy_c_string(response.protocol);
    result.version = copy_c_string(response.version);
    result.status_code = copy_c_string(response.statusCode);
    result.status_text = copy_c_string(response.statusText);
    result.headers_json = copy_c_string(headers_to_json(response.headers));

    if (response.body.first > 0 && response.body.second) {
        size_t size = response.body.first;
        char* body_copy = (char*)malloc(size + 1);
        memcpy(body_copy, response.body.second.get(), size);
        body_copy[size] = '\0';
        result.body = body_copy;
        result.body_size = size;
    } else {
        result.body = copy_c_string("");
        result.body_size = 0;
    }

    return result;
}

static io_http_response_t make_http_error_response(const char* error_msg) {
    io_http_response_t result = {};
    result.protocol = copy_c_string("error");
    result.version = copy_c_string("");
    result.status_code = copy_c_string("0");
    result.status_text = copy_c_string(error_msg ? error_msg : "Unknown error");
    result.headers_json = copy_c_string("{}");
    result.body = copy_c_string("");
    result.body_size = 0;
    return result;
}

extern "C" io_http_response_t io_http_get(const char* url) {
    if (!url) return make_http_error_response("NULL URL");
    try {
        auto response = iostreams::http::http_client::restSync("GET", std::string(url));
        return make_http_response(response);
    } catch (const std::exception& e) {
        return make_http_error_response(e.what());
    } catch (...) {
        return make_http_error_response("Unknown error");
    }
}

extern "C" io_http_response_t io_http_post(const char* url, const void* body, size_t body_size) {
    if (!url) return make_http_error_response("NULL URL");
    try {
        iostreams::http::Body http_body;
        if (body && body_size > 0) {
            http_body.first = body_size;
            auto data = std::shared_ptr<int8_t>(new int8_t[body_size], std::default_delete<int8_t[]>());
            memcpy(data.get(), body, body_size);
            http_body.second = data;
        }
        auto response = iostreams::http::http_client::restSync("POST", std::string(url), {}, http_body);
        return make_http_response(response);
    } catch (const std::exception& e) {
        return make_http_error_response(e.what());
    } catch (...) {
        return make_http_error_response("Unknown error");
    }
}

extern "C" io_http_response_t io_http_put(const char* url, const void* body, size_t body_size) {
    if (!url) return make_http_error_response("NULL URL");
    try {
        iostreams::http::Body http_body;
        if (body && body_size > 0) {
            http_body.first = body_size;
            auto data = std::shared_ptr<int8_t>(new int8_t[body_size], std::default_delete<int8_t[]>());
            memcpy(data.get(), body, body_size);
            http_body.second = data;
        }
        auto response = iostreams::http::http_client::restSync("PUT", std::string(url), {}, http_body);
        return make_http_response(response);
    } catch (const std::exception& e) {
        return make_http_error_response(e.what());
    } catch (...) {
        return make_http_error_response("Unknown error");
    }
}

extern "C" io_http_response_t io_http_delete(const char* url) {
    if (!url) return make_http_error_response("NULL URL");
    try {
        auto response = iostreams::http::http_client::restSync("DELETE", std::string(url));
        return make_http_response(response);
    } catch (const std::exception& e) {
        return make_http_error_response(e.what());
    } catch (...) {
        return make_http_error_response("Unknown error");
    }
}

extern "C" io_http_response_t io_http_request(const char* method, const char* url, const io_http_header_t* headers, size_t header_count, const void* body, size_t body_size) {
    if (!method || !url) return make_http_error_response("NULL method or URL");
    try {
        iostreams::http::Headers http_headers;
        if (headers && header_count > 0) {
            for (size_t i = 0; i < header_count; ++i) {
                http_headers[std::string(headers[i].key)] = std::string(headers[i].value);
            }
        }

        iostreams::http::Body http_body;
        if (body && body_size > 0) {
            http_body.first = body_size;
            auto data = std::shared_ptr<int8_t>(new int8_t[body_size], std::default_delete<int8_t[]>());
            memcpy(data.get(), body, body_size);
            http_body.second = data;
        }

        auto response = iostreams::http::http_client::restSync(std::string(method), std::string(url), http_headers, http_body);
        return make_http_response(response);
    } catch (const std::exception& e) {
        return make_http_error_response(e.what());
    } catch (...) {
        return make_http_error_response("Unknown error");
    }
}

extern "C" void io_http_response_free(io_http_response_t* response) {
    if (!response) return;
    free((void*)response->protocol);
    free((void*)response->version);
    free((void*)response->status_code);
    free((void*)response->status_text);
    free((void*)response->headers_json);
    free((void*)response->body);
}

/* ============================================================================
 * Archive Stream
 * ============================================================================ */

extern "C" io_archive_t* io_archive_open(const char* path, io_archive_mode_t mode) {
    if (!path) return nullptr;
    try {
        std::ios::openmode openmode;
        if (mode == IO_ARCHIVE_READ) {
            openmode = std::ios::in;
        } else {
            openmode = std::ios::out;
        }
        auto archive = new (std::nothrow) iostreams::archive_stream(std::string(path), openmode);
        auto handle = new (std::nothrow) io_archive;
        if (handle) handle->ptr = archive;
        return handle;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_archive_close(io_archive_t* archive) {
    if (archive) {
        delete archive->ptr;
        delete archive;
    }
}

extern "C" char** io_archive_list_entries(io_archive_t* archive, size_t* out_count) {
    if (!archive || !archive->ptr) return nullptr;
    try {
        auto entries = archive->ptr->list_entries();
        if (out_count) *out_count = entries.size();
        if (entries.empty()) return nullptr;

        char** result = (char**)malloc(entries.size() * sizeof(char*));
        for (size_t i = 0; i < entries.size(); ++i) {
            result[i] = copy_c_string(entries[i]);
        }
        return result;
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_archive_entries_free(char** entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; ++i) {
        free(entries[i]);
    }
    free(entries);
}

extern "C" int io_archive_set_entry(io_archive_t* archive, const char* name) {
    if (!archive || !archive->ptr || !name) return IO_ERROR;
    try {
        archive->ptr->set_entry(std::string(name));
        return IO_SUCCESS;
    } catch (...) {
        return IO_ERROR_ARCHIVE;
    }
}

extern "C" char* io_archive_get_entry_string(io_archive_t* archive) {
    if (!archive || !archive->ptr) return nullptr;
    try {
        std::string str = archive->ptr->get_entry_string();
        return copy_c_string(str);
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_archive_string_free(char* str) {
    free(str);
}

extern "C" int64_t io_archive_write(io_archive_t* archive, const void* data, size_t size) {
    if (!archive || !archive->ptr || !data) return IO_ERROR;
    try {
        archive->ptr->write((const char*)data, size);
        return static_cast<int64_t>(size);
    } catch (...) {
        return IO_ERROR_ARCHIVE;
    }
}

extern "C" int64_t io_archive_read(io_archive_t* archive, void* buffer, size_t size) {
    if (!archive || !archive->ptr || !buffer) return IO_ERROR;
    try {
        archive->ptr->read((char*)buffer, size);
        return static_cast<int64_t>(archive->ptr->gcount());
    } catch (...) {
        return IO_ERROR_ARCHIVE;
    }
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

extern "C" char* io_get_local_ip(void) {
    try {
        std::string ip = iostreams::get_local_ip();
        return copy_c_string(ip);
    } catch (...) {
        return nullptr;
    }
}

extern "C" int io_string_is_ipv4(const char* str) {
    if (!str) return 0;
    return iostreams::string_is_ipv4(std::string(str)) ? 1 : 0;
}

extern "C" char* io_resolve_host_to_ip(const char* host) {
    if (!host) return nullptr;
    try {
        std::string ip = iostreams::resolve_host_or_ip_to_ip(std::string(host));
        return copy_c_string(ip);
    } catch (...) {
        return nullptr;
    }
}

extern "C" void io_string_free(char* str) {
    free(str);
}

/* ============================================================================
 * Version
 * ============================================================================ */

extern "C" const char* io_version_string(void) {
    return "iostreams 1.2.0.0";
}
