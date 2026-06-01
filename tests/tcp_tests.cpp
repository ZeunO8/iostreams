#include <catch2/catch_test_macros.hpp>
#include <iostreams/tcp_server.hpp>
#include <iostreams/tcp_client.hpp>
#include <iostreams/Serial.hpp>
#include <thread>
#include <chrono>
#include <vector>

using namespace iostreams;

TEST_CASE("tcp_server creates and closes", "[tcp]")
{
    tcp_server server(18920);
    REQUIRE(server.close() == true);
    REQUIRE(server.close() == false);
}

TEST_CASE("tcp_server accepts client connection", "[tcp]")
{
    tcp_server server(18921);

    std::thread client_thread([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        try {
            tcp_client client("127.0.0.1", 18921);
            client << std::string("hello");
            client.flush();
        } catch (...) {}
    });

    tcp_server::ClientTuple* tuple = nullptr;
    int fd = server.acceptOne(&tuple);
    if (fd > 0 && tuple != nullptr) {
        auto& [client_fd, serial, iostream] = *tuple;
        std::string msg;
        *serial >> msg;
        REQUIRE(msg == "hello");
        server.closeClient(1);
    }
    server.close();
    client_thread.join();
}

TEST_CASE("tcp_server returns -1 when closed", "[tcp]")
{
    tcp_server server(18924);
    server.close();

    tcp_server::ClientTuple* tuple = nullptr;
    int fd = server.acceptOne(&tuple);
    REQUIRE(fd == -1);
    REQUIRE(tuple == nullptr);
}

TEST_CASE("tcp_server closeClient removes client", "[tcp]")
{
    tcp_server server(18923);

    std::thread client_thread([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        try {
            tcp_client client("127.0.0.1", 18923);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } catch (...) {}
    });

    tcp_server::ClientTuple* tuple = nullptr;
    int fd = server.acceptOne(&tuple);
    if (fd > 0) {
        REQUIRE(server.closeClient(1) == true);
        REQUIRE(server.closeClient(1) == false);
    }
    server.close();
    client_thread.join();
}

TEST_CASE("tcp_server with bitstream mode", "[tcp]")
{
    tcp_server server(18930, true);

    std::thread client_thread([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        try {
            tcp_client client("127.0.0.1", 18930);
            client << int32_t(12345);
            client.flush();
        } catch (...) {}
    });

    tcp_server::ClientTuple* tuple = nullptr;
    int fd = server.acceptOne(&tuple);
    if (fd > 0 && tuple != nullptr) {
        int32_t val = 0;
        auto& [client_fd, serial, iostream] = *tuple;
        *serial >> val;
        REQUIRE(val == 12345);
    }
    server.close();
    client_thread.join();
}

TEST_CASE("tcp_streambuf wait_readable timeout", "[tcp]")
{
    tcp_server server(18928);

    std::thread client_thread([&]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        try {
            tcp_client client("127.0.0.1", 18928);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            client << std::string("data");
            client.flush();
        } catch (...) {}
    });

    tcp_server::ClientTuple* tuple = nullptr;
    int fd = server.acceptOne(&tuple);
    if (fd > 0 && tuple != nullptr) {
        auto& [client_fd, serial, iostream] = *tuple;
        auto* buf = dynamic_cast<iostreams::streams::tcp_streambuf*>(iostream->rdbuf());
        REQUIRE(buf != nullptr);
        REQUIRE(buf->wait_readable(50) == false);
    }
    server.close();
    client_thread.join();
}
