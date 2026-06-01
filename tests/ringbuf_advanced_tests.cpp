#include <catch2/catch_test_macros.hpp>
#include <iostreams/ring_streambuf.hpp>
#include <iostreams/memory_streambuf.hpp>
#include <thread>
#include <chrono>
#include <array>
#include <vector>

using namespace iostreams;

TEST_CASE("ringbuf available_write decreases after write", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    size_t initial_write = rb.available_write();

    const char* data = "test";
    rb.sputn(data, 4);

    REQUIRE(rb.available_write() == initial_write - 4);
}

TEST_CASE("ringbuf available_read increases after write", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    REQUIRE(rb.available_read() == 0);

    const char* data = "test";
    rb.sputn(data, 4);

    REQUIRE(rb.available_read() == 4);
}

TEST_CASE("ringbuf available_read decreases after read", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    const char* data = "hello";
    rb.sputn(data, 5);
    REQUIRE(rb.available_read() == 5);

    char buf[3];
    rb.sgetn(buf, 3);
    REQUIRE(rb.available_read() == 2);
}

TEST_CASE("ringbuf wraps around correctly", "[ringbuf]")
{
    constexpr size_t total_size = 64;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);

    const char* data1 = "first_write";
    rb.sputn(data1, 11);

    char buf1[12];
    rb.sgetn(buf1, 11);
    buf1[11] = '\0';
    REQUIRE(std::string(buf1) == "first_write");

    const char* data2 = "second_write";
    rb.sputn(data2, 12);

    char buf2[13];
    rb.sgetn(buf2, 12);
    buf2[12] = '\0';
    REQUIRE(std::string(buf2) == "second_write");
}

TEST_CASE("ringbuf full buffer behavior", "[ringbuf]")
{
    constexpr size_t total_size = 64;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);

    const char* data = "12345678";
    rb.sputn(data, 8);

    REQUIRE(rb.available_read() == 8);
    REQUIRE(rb.available_write() > 0);
}

TEST_CASE("ringbuf non-blocking mode returns 0 when empty", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    std::iostream stream(&rb);

    char buf[10] = {0};
    stream.read(buf, 10);
    REQUIRE(stream.gcount() == 0);
}

TEST_CASE("ringbuf move constructor transfers state", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb1(mem.data(), total_size, true);
    const char* data = "move test";
    rb1.sputn(data, 9);

    ringbuf rb2(std::move(rb1));

    REQUIRE(rb2.buffer == mem.data());
    REQUIRE(rb2.buffer_size == total_size);
    REQUIRE(rb1.buffer == nullptr);
    REQUIRE(rb2.available_read() == 9);
}

TEST_CASE("ringbuf move assignment transfers state", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb1(mem.data(), total_size, true);
    const char* data = "move assign";
    rb1.sputn(data, 11);

    ringbuf rb2(nullptr, 0, true);
    rb2 = std::move(rb1);

    REQUIRE(rb2.buffer == mem.data());
    REQUIRE(rb2.buffer_size == total_size);
    REQUIRE(rb1.buffer == nullptr);
    REQUIRE(rb2.available_read() == 11);
}

TEST_CASE("ringbuf read after partial read", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    std::iostream stream(&rb);

    const char* data = "hello world";
    stream.write(data, 11);

    char buf1[6];
    stream.read(buf1, 5);
    buf1[5] = '\0';
    REQUIRE(std::string(buf1) == "hello");

    stream.seekg(0);

    char buf2[12];
    stream.read(buf2, 11);
    buf2[11] = '\0';
    REQUIRE(std::string(buf2) == "hello world");
}

TEST_CASE("memory_streambuf write and read with stream", "[memory]")
{
    std::vector<char> mem(256);
    memory_streambuf msb(mem.data(), mem.size());
    std::iostream stream(&msb);

    const char* data = "hello memory";
    stream.write(data, 14);
    stream.seekg(0);

    char buf[15] = {0};
    stream.read(buf, 14);
    REQUIRE(std::string(buf) == "hello memory");
}

TEST_CASE("memory_streambuf partial read", "[memory]")
{
    std::vector<char> mem(256);
    memory_streambuf msb(mem.data(), mem.size());
    std::iostream stream(&msb);

    const char* data = "partial read test";
    stream.write(data, 17);
    stream.seekg(0);

    char buf1[8];
    stream.read(buf1, 7);
    buf1[7] = '\0';
    REQUIRE(std::string(buf1) == "partial");

    char buf2[6];
    stream.read(buf2, 5);
    buf2[5] = '\0';
    REQUIRE(std::string(buf2) == " read");
}

TEST_CASE("memory_streambuf seek operations", "[memory]")
{
    std::vector<char> mem(256);
    memory_streambuf msb(mem.data(), mem.size());
    std::iostream stream(&msb);

    const char* data = "seek test data";
    stream.write(data, 14);

    stream.seekg(5);
    char buf[5];
    stream.read(buf, 4);
    buf[4] = '\0';
    REQUIRE(std::string(buf) == "test");
}

TEST_CASE("ringbuf producer consumer pattern", "[ringbuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    ringbuf rb(mem.data(), total_size, true);
    std::iostream stream(&rb);

    std::thread producer([&]()
    {
        for (int i = 0; i < 10; i++)
        {
            std::string msg = "msg" + std::to_string(i) + "\n";
            stream.write(msg.data(), msg.size());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::string all_data;
    char buf[128];
    while (all_data.size() < 50)
    {
        stream.read(buf, sizeof(buf));
        auto count = stream.gcount();
        if (count > 0)
        {
            all_data.append(buf, count);
        }
        if (stream.fail() && count > 0)
            stream.clear();
    }

    producer.join();
    REQUIRE(all_data.size() > 0);
}
