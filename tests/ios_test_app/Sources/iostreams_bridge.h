#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* protocol;
    const char* version;
    const char* statusCode;
    const char* statusText;
    const char* headers_json;
    const char* body;
    double elapsed_ms;
    long body_size;
} IOHttpResponse;

IOHttpResponse io_http_get(const char* url);
void io_http_response_free(IOHttpResponse* response);
const char* io_version_string(void);

#ifdef __cplusplus
}
#endif
