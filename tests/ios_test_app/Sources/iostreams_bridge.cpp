#include "iostreams_bridge.h"
#include <iostreams/http_client.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>

static char* copy_string(const std::string& str) {
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
    for (const auto& [key, value] : headers) {
        if (!first) json += ",";
        json += "\"" + key + "\":\"" + value + "\"";
        first = false;
    }
    json += "}";
    return json;
}

extern "C" {

IOHttpResponse io_http_get(const char* url) {
    IOHttpResponse result = {};
    auto start = std::chrono::high_resolution_clock::now();

    try {
        auto response = iostreams::http::http_client::restSync(
            "GET",
            std::string(url)
        );

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        result.protocol = copy_string(response.protocol);
        result.version = copy_string(response.version);
        result.statusCode = copy_string(response.statusCode);
        result.statusText = copy_string(response.statusText);
        result.headers_json = copy_string(headers_to_json(response.headers));

        if (response.body.first > 0 && response.body.second) {
            long size = static_cast<long>(response.body.first);
            char* body_copy = (char*)malloc(size + 1);
            memcpy(body_copy, response.body.second.get(), size);
            body_copy[size] = '\0';
            result.body = body_copy;
            result.body_size = size;
        } else {
            result.body = copy_string("");
            result.body_size = 0;
        }

        result.elapsed_ms = elapsed.count();
    } catch (const std::exception& e) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        result.protocol = copy_string("error");
        result.version = copy_string("");
        result.statusCode = copy_string("0");
        result.statusText = copy_string(e.what());
        result.headers_json = copy_string("{}");
        result.body = copy_string("");
        result.body_size = 0;
        result.elapsed_ms = elapsed.count();
    }

    return result;
}

void io_http_response_free(IOHttpResponse* response) {
    if (!response) return;
    free((void*)response->protocol);
    free((void*)response->version);
    free((void*)response->statusCode);
    free((void*)response->statusText);
    free((void*)response->headers_json);
    free((void*)response->body);
}

const char* io_version_string(void) {
    return "iostreams 1.2.0.0";
}

}
