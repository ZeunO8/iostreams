#include <catch2/catch_test_macros.hpp>
#include <iostreams/ring_streambuf.hpp>
#include <iostreams/memory_streambuf.hpp>
#include <iostreams/counting_streambuf.hpp>
#include <sstream>
#include <thread>
#include <vector>
#include <array>

TEST_CASE("ring_streambuf write and read", "[streambuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    iostreams::ringbuf rb(mem.data(), total_size, true);
    std::iostream stream(&rb);

    const char* data = "hello ring buffer";
    stream.write(data, 17);
    stream.seekg(0);

    char buf[18] = {0};
    stream.read(buf, 17);
    REQUIRE(std::string(buf) == "hello ring buffer");
}

TEST_CASE("ring_streambuf non-blocking mode returns immediately when empty", "[streambuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    iostreams::ringbuf rb(mem.data(), total_size, true);
    std::iostream stream(&rb);

    char buf[10] = {0};
    stream.read(buf, 10);
    REQUIRE(stream.gcount() == 0);
}

TEST_CASE("ring_streambuf copy is deleted", "[streambuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    iostreams::ringbuf rb1(mem.data(), total_size, true);
    static_assert(!std::is_copy_constructible_v<iostreams::ringbuf>, "ringbuf should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<iostreams::ringbuf>, "ringbuf should not be copy assignable");
}

TEST_CASE("ring_streambuf move works correctly", "[streambuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    iostreams::ringbuf rb1(mem.data(), total_size, true);
    iostreams::ringbuf rb2(std::move(rb1));

    REQUIRE(rb2.buffer == mem.data());
    REQUIRE(rb2.buffer_size == total_size);
    REQUIRE(rb1.buffer == nullptr);
}

TEST_CASE("memory_streambuf write and read", "[streambuf]")
{
    std::vector<char> mem(256);
    iostreams::memory_streambuf msb(mem.data(), mem.size());
    std::iostream stream(&msb);

    const char* data = "hello memory";
    stream.write(data, 14);
    stream.seekg(0);

    char buf[15] = {0};
    stream.read(buf, 14);
    REQUIRE(std::string(buf) == "hello memory");
}

TEST_CASE("counting_streambuf counts writes", "[streambuf]")
{
    std::vector<char> mem(256);
    iostreams::counting_streambuf csb(mem.data(), mem.size());
    std::ostream stream(&csb);

    stream.write("hello", 5);
    stream.write(" world", 6);

    REQUIRE(csb.bytes_written() == 11);
}

TEST_CASE("counting_streambuf counts reads", "[streambuf]")
{
    std::vector<char> mem = {'a', 'b', 'c', 'd', 'e'};
    iostreams::counting_streambuf csb(mem.data(), mem.size());
    std::istream stream(&csb);

    char buf[3];
    stream.read(buf, 3);
    REQUIRE(csb.bytes_read() == 3);

    stream.read(buf, 2);
    REQUIRE(csb.bytes_read() == 5);
}

TEST_CASE("ring_streambuf available_read and available_write", "[streambuf]")
{
    constexpr size_t total_size = 1024;
    std::array<char, total_size> mem{};

    iostreams::ringbuf rb(mem.data(), total_size, true);

    REQUIRE(rb.available_read() == 0);
    REQUIRE(rb.available_write() > 0);

    const char* data = "test";
    rb.sputn(data, 4);

    REQUIRE(rb.available_read() == 4);
}
