#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Handle Types
 * ============================================================================ */

typedef struct io_tcp_client     io_tcp_client_t;
typedef struct io_tcp_server     io_tcp_server_t;
typedef struct io_tcp_stream     io_tcp_stream_t;
typedef struct io_udp_client     io_udp_client_t;
typedef struct io_udp_server     io_udp_server_t;
typedef struct io_udp_stream     io_udp_stream_t;
typedef struct io_udpmc_sender   io_udpmc_sender_t;
typedef struct io_udpmc_receiver io_udpmc_receiver_t;
typedef struct io_serial         io_serial_t;
typedef struct io_ssl_ctx        io_ssl_ctx_t;
typedef struct io_archive        io_archive_t;

/* ============================================================================
 * Common Types and Enums
 * ============================================================================ */

typedef enum {
    IO_SUCCESS = 0,
    IO_ERROR = -1,
    IO_ERROR_INVALID_ARG = -2,
    IO_ERROR_CONNECTION_FAILED = -3,
    IO_ERROR_TIMEOUT = -4,
    IO_ERROR_EOF = -5,
    IO_ERROR_SSL = -6,
    IO_ERROR_ARCHIVE = -7
} io_result_t;

typedef enum {
    IO_ARCHIVE_READ = 0,
    IO_ARCHIVE_WRITE = 1
} io_archive_mode_t;

/* ============================================================================
 * Socket Initialization
 * ============================================================================ */

void io_socket_init(void);

/* ============================================================================
 * SSL Context Factory
 * ============================================================================ */

io_ssl_ctx_t* io_ssl_create_client_ctx(const char* host);
io_ssl_ctx_t* io_ssl_create_server_ctx(void);
io_ssl_ctx_t* io_ssl_create_server_ctx_with_certs(const char* cert_path, const char* key_path);
void io_ssl_free_ctx(io_ssl_ctx_t* ctx);

/* ============================================================================
 * TCP Client
 * ============================================================================ */

io_tcp_client_t* io_tcp_client_create(const char* host, int port);
io_tcp_client_t* io_tcp_client_create_ssl(const char* host, int port, io_ssl_ctx_t* ssl_ctx, int verify_certs);
void io_tcp_client_destroy(io_tcp_client_t* client);

int64_t io_tcp_client_write(io_tcp_client_t* client, const void* data, size_t size);
int64_t io_tcp_client_read(io_tcp_client_t* client, void* buffer, size_t size);
int io_tcp_client_ssl_upgrade(io_tcp_client_t* client, io_ssl_ctx_t* ctx, io_ssl_ctx_t* ssl);
int io_tcp_client_close(io_tcp_client_t* client);
int io_tcp_client_wait_readable(io_tcp_client_t* client, int timeout_ms);
int io_tcp_client_wait_writable(io_tcp_client_t* client, int timeout_ms);
int io_tcp_client_bytes_available(io_tcp_client_t* client);

/* ============================================================================
 * TCP Server
 * ============================================================================ */

io_tcp_server_t* io_tcp_server_create(int port);
io_tcp_server_t* io_tcp_server_create_ssl(int port, io_ssl_ctx_t* ssl_ctx, int enable_non_blocking);
io_tcp_server_t* io_tcp_server_create_bitstream(int port, int bit_stream, io_ssl_ctx_t* ssl_ctx, int enable_non_blocking);
void io_tcp_server_destroy(io_tcp_server_t* server);

io_tcp_stream_t* io_tcp_server_accept(io_tcp_server_t* server, size_t* out_client_id);
int io_tcp_server_close_client(io_tcp_server_t* server, size_t client_id);
int io_tcp_server_close(io_tcp_server_t* server);
int io_tcp_server_upgrade_ssl(io_tcp_server_t* server, io_ssl_ctx_t* ssl_ctx);

/* ============================================================================
 * TCP Stream (accepted client connection)
 * ============================================================================ */

void io_tcp_stream_destroy(io_tcp_stream_t* stream);

int64_t io_tcp_stream_write(io_tcp_stream_t* stream, const void* data, size_t size);
int64_t io_tcp_stream_read(io_tcp_stream_t* stream, void* buffer, size_t size);
int io_tcp_stream_ssl_upgrade(io_tcp_stream_t* stream, io_ssl_ctx_t* ctx, io_ssl_ctx_t* ssl);
int io_tcp_stream_close(io_tcp_stream_t* stream);
int io_tcp_stream_wait_readable(io_tcp_stream_t* stream, int timeout_ms);
int io_tcp_stream_wait_writable(io_tcp_stream_t* stream, int timeout_ms);

/* ============================================================================
 * UDP Client
 * ============================================================================ */

io_udp_client_t* io_udp_client_create(const char* host, int port);
void io_udp_client_destroy(io_udp_client_t* client);

int64_t io_udp_client_write(io_udp_client_t* client, const void* data, size_t size);
int64_t io_udp_client_read(io_udp_client_t* client, void* buffer, size_t size);
int io_udp_client_close(io_udp_client_t* client);

/* ============================================================================
 * UDP Server
 * ============================================================================ */

io_udp_server_t* io_udp_server_create(int port);
io_udp_server_t* io_udp_server_create_bitstream(int port, int bit_stream);
void io_udp_server_destroy(io_udp_server_t* server);

io_udp_stream_t* io_udp_server_receive(io_udp_server_t* server, int non_block, unsigned int timeout_us);
void io_udp_server_close(io_udp_server_t* server);

/* ============================================================================
 * UDP Stream (received packet)
 * ============================================================================ */

void io_udp_stream_destroy(io_udp_stream_t* stream);

int64_t io_udp_stream_write(io_udp_stream_t* stream, const void* data, size_t size);
int64_t io_udp_stream_read(io_udp_stream_t* stream, void* buffer, size_t size);
int io_udp_stream_close(io_udp_stream_t* stream);

/* ============================================================================
 * UDP Multicast Sender
 * ============================================================================ */

io_udpmc_sender_t* io_udpmc_sender_create(const char* host, int port);
void io_udpmc_sender_destroy(io_udpmc_sender_t* sender);

int64_t io_udpmc_sender_write(io_udpmc_sender_t* sender, const void* data, size_t size);
void io_udpmc_sender_close(io_udpmc_sender_t* sender);

/* ============================================================================
 * UDP Multicast Receiver
 * ============================================================================ */

io_udpmc_receiver_t* io_udpmc_receiver_create(const char* host, int port);
void io_udpmc_receiver_destroy(io_udpmc_receiver_t* receiver);

int64_t io_udpmc_receiver_read(io_udpmc_receiver_t* receiver, void* buffer, size_t size);
void io_udpmc_receiver_close(io_udpmc_receiver_t* receiver);

/* ============================================================================
 * Serial (Bitstream)
 * ============================================================================ */

io_serial_t* io_serial_create(void);
io_serial_t* io_serial_create_buffer(int64_t size, char* buffer);
io_serial_t* io_serial_create_count(int count);
void io_serial_destroy(io_serial_t* serial);

int64_t io_serial_write_bytes(io_serial_t* serial, const void* src, size_t size);
int64_t io_serial_read_bytes(io_serial_t* serial, void* dest, size_t size);

void io_serial_write_byte(io_serial_t* serial, char byte);
char io_serial_read_byte(io_serial_t* serial);

void io_serial_write_bit(io_serial_t* serial, int bit);
int io_serial_read_bit(io_serial_t* serial);

void io_serial_write_bits(io_serial_t* serial, const uint8_t* bits, size_t index, size_t count);
void io_serial_read_bits(io_serial_t* serial, uint8_t* bits, size_t index, size_t count);

int64_t io_serial_get_write_position(io_serial_t* serial);
int64_t io_serial_get_read_position(io_serial_t* serial);
void io_serial_set_write_position(io_serial_t* serial, size_t index);
void io_serial_set_read_position(io_serial_t* serial, size_t index);

int64_t io_serial_get_write_length(io_serial_t* serial);
int64_t io_serial_get_read_length(io_serial_t* serial);

void io_serial_synchronize(io_serial_t* serial);

int io_serial_is_read_eof(io_serial_t* serial);
int io_serial_is_read_empty(io_serial_t* serial);
int io_serial_did_not_read_whole_size(io_serial_t* serial);
size_t io_serial_get_last_bytes_read(io_serial_t* serial);

void io_serial_clear_read(io_serial_t* serial);

/* ============================================================================
 * Serial over TCP Stream
 * ============================================================================ */

io_serial_t* io_serial_create_tcp(io_tcp_stream_t* stream, int bit_stream);
io_serial_t* io_serial_create_tcp_client(io_tcp_client_t* client, int bit_stream);

/* ============================================================================
 * HTTP Client
 * ============================================================================ */

typedef struct {
    const char* protocol;
    const char* version;
    const char* status_code;
    const char* status_text;
    const char* headers_json;
    const char* body;
    size_t body_size;
} io_http_response_t;

typedef struct {
    const char* key;
    const char* value;
} io_http_header_t;

io_http_response_t io_http_get(const char* url);
io_http_response_t io_http_post(const char* url, const void* body, size_t body_size);
io_http_response_t io_http_put(const char* url, const void* body, size_t body_size);
io_http_response_t io_http_delete(const char* url);
io_http_response_t io_http_request(const char* method, const char* url, const io_http_header_t* headers, size_t header_count, const void* body, size_t body_size);

void io_http_response_free(io_http_response_t* response);

/* ============================================================================
 * Archive Stream
 * ============================================================================ */

io_archive_t* io_archive_open(const char* path, io_archive_mode_t mode);
void io_archive_close(io_archive_t* archive);

char** io_archive_list_entries(io_archive_t* archive, size_t* out_count);
void io_archive_entries_free(char** entries, size_t count);

int io_archive_set_entry(io_archive_t* archive, const char* name);
char* io_archive_get_entry_string(io_archive_t* archive);
void io_archive_string_free(char* str);

int64_t io_archive_write(io_archive_t* archive, const void* data, size_t size);
int64_t io_archive_read(io_archive_t* archive, void* buffer, size_t size);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

char* io_get_local_ip(void);
int io_string_is_ipv4(const char* str);
char* io_resolve_host_to_ip(const char* host);
void io_string_free(char* str);

/* ============================================================================
 * Version
 * ============================================================================ */

const char* io_version_string(void);

#ifdef __cplusplus
}
#endif
