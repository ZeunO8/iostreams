#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <iostreams/Serial.hpp>
#include <sstream>
#include <vector>
#include <cstdint>

using Catch::Approx;

TEST_CASE("Serial bitstream write and read bits", "[serial][bitstream]")
{
    std::vector<char> buffer(256);
    {
        Serial serial(buffer.size(), buffer.data());
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeBit(true);
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeBit(false);
        serial.writeBit(true);
        serial.writeBit(false);
        serial.synchronize();
    }
    {
        Serial serial(buffer.size(), buffer.data());
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
    }
}

TEST_CASE("Serial bitstream write and read bytes mixed", "[serial][bitstream]")
{
    std::vector<char> buffer(256);
    {
        Serial serial(buffer.size(), buffer.data());
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeBit(true);
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeBit(false);
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeByte(0x55);
        serial.synchronize();
    }
    {
        Serial serial(buffer.size(), buffer.data());
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        char byte = serial.readByte();
        REQUIRE((unsigned char)byte == 0x55);
    }
}

TEST_CASE("Serial bitstream writeBits and readBits", "[serial][bitstream]")
{
    std::vector<char> buffer(256);
    {
        Serial serial(buffer.size(), buffer.data(), true);
        std::vector<uint8_t> bits = {1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 0};
        serial.writeBits(bits, 0, bits.size());
        serial.synchronize();
    }
    {
        Serial serial(buffer.size(), buffer.data(), true);
        std::vector<uint8_t> bits(12);
        serial.readBits(bits, 0, 12);
        REQUIRE(bits[0] == 1);
        REQUIRE(bits[1] == 0);
        REQUIRE(bits[2] == 1);
        REQUIRE(bits[3] == 1);
        REQUIRE(bits[4] == 0);
        REQUIRE(bits[5] == 0);
        REQUIRE(bits[6] == 1);
        REQUIRE(bits[7] == 0);
        REQUIRE(bits[8] == 0);
        REQUIRE(bits[9] == 1);
        REQUIRE(bits[10] == 1);
        REQUIRE(bits[11] == 0);
    }
}

TEST_CASE("Serial bitstream bool serialization", "[serial][bitstream]")
{
    std::vector<char> buffer(256);
    {
        Serial serial(buffer.size(), buffer.data());
        serial << true << false << true << false;
        serial.synchronize();
    }
    {
        Serial serial(buffer.size(), buffer.data());
        bool a, b, c, d;
        serial >> a >> b >> c >> d;
        REQUIRE(a == true);
        REQUIRE(b == false);
        REQUIRE(c == true);
        REQUIRE(d == false);
    }
}

TEST_CASE("Serial buffer mode write and read", "[serial][buffer]")
{
    char buffer[256];
    Serial write_serial(256, buffer);
    write_serial << int32_t(42);
    write_serial << std::string("hello");

    Serial read_serial(256, buffer);
    int32_t val = 0;
    std::string str;
    read_serial >> val >> str;

    REQUIRE(val == 42);
    REQUIRE(str == "hello");
}

TEST_CASE("Serial buffer mode position tracking", "[serial][buffer]")
{
    char buffer[256];
    Serial serial(256, buffer);

    REQUIRE(serial.getWritePosition() == 0);
    REQUIRE(serial.getReadPosition() == 0);

    serial << int32_t(12345);

    REQUIRE(serial.getWritePosition() > 0);

    serial.setReadPosition(0);
    REQUIRE(serial.getReadPosition() == 0);
}

TEST_CASE("Serial count mode tracks write length", "[serial][count]")
{
    Serial serial(true);
    serial << int32_t(42);
    serial << std::string("test");
    serial << double(3.14);

    REQUIRE(serial.getWritePosition() > 0);
    REQUIRE(serial.getWriteLength() > 0);
}

TEST_CASE("Serial position seek and reread", "[serial][position]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        serial << int32_t(100);
        serial << int32_t(200);
        serial << int32_t(300);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        int32_t a, b, c;
        serial >> a >> b >> c;
        REQUIRE(a == 100);
        REQUIRE(b == 200);
        REQUIRE(c == 300);

        serial.setReadPosition(0);
        int32_t x, y, z;
        serial >> x >> y >> z;
        REQUIRE(x == 100);
        REQUIRE(y == 200);
        REQUIRE(z == 300);
    }
}

TEST_CASE("Serial write position seek", "[serial][position]")
{
    std::stringstream ss;
    Serial serial(ss);

    serial << int32_t(111);
    auto pos1 = serial.getWritePosition();
    serial << int32_t(222);
    auto pos2 = serial.getWritePosition();

    REQUIRE(pos2 > pos1);

    serial.setWritePosition(pos1);
    serial << int32_t(333);

    REQUIRE(serial.getWritePosition() == pos2);
}

TEST_CASE("Serial is_read_eof and is_read_empty", "[serial][state]")
{
    std::stringstream ss;
    Serial serial(ss);

    REQUIRE(serial.is_read_eof() == false);
    REQUIRE(serial.is_read_empty() == false);
}

TEST_CASE("Serial get_last_bytes_read", "[serial][state]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        serial << std::string("hello");
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        char buf[10];
        auto ret = serial.readBytes(buf, 5);
        REQUIRE(ret == 5);
        REQUIRE(serial.get_last_bytes_read() == 5);
    }
}

TEST_CASE("Serial did_not_read_whole_size", "[serial][state]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        serial.writeBytes("hi", 2);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        char buf[10];
        serial.readBytes(buf, 10);
        REQUIRE(serial.did_not_read_whole_size() == true);
    }
}

TEST_CASE("Serial clearRead resets error state", "[serial][state]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        serial.writeBytes("hi", 2);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        char buf[10];
        serial.readBytes(buf, 10);
        REQUIRE(serial.did_not_read_whole_size() == true);
        serial.clearRead();
        REQUIRE(serial.last_did_not_read_whole_size() == false);
    }
}

TEST_CASE("Serial serialize and deserialize nested vector", "[serial][nested]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::vector<std::vector<int>> nested = {{1, 2}, {3, 4, 5}, {6}};
        serial << nested.size();
        for (auto& inner : nested)
        {
            serialize_vector(serial, inner);
        }
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        size_t outer_size = 0;
        serial >> outer_size;
        REQUIRE(outer_size == 3);

        std::vector<int> v1, v2, v3;
        deserialize_vector(serial, v1);
        deserialize_vector(serial, v2);
        deserialize_vector(serial, v3);

        REQUIRE(v1 == std::vector<int>{1, 2});
        REQUIRE(v2 == std::vector<int>{3, 4, 5});
        REQUIRE(v3 == std::vector<int>{6});
    }
}

TEST_CASE("Serial serialize and deserialize map with string keys", "[serial][map]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::map<std::string, int> map;
        map["alpha"] = 1;
        map["beta"] = 2;
        map["gamma"] = 3;
        serialize_map(serial, map);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::map<std::string, int> map;
        deserialize_map(serial, map);
        REQUIRE(map.size() == 3);
        REQUIRE(map["alpha"] == 1);
        REQUIRE(map["beta"] == 2);
        REQUIRE(map["gamma"] == 3);
    }
}

TEST_CASE("Serial serialize and deserialize set", "[serial][set]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::set<std::string> st = {"apple", "banana", "cherry"};
        serialize_set(serial, st);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::set<std::string> st;
        deserialize_set(serial, st);
        REQUIRE(st.size() == 3);
        REQUIRE(st.count("apple"));
        REQUIRE(st.count("banana"));
        REQUIRE(st.count("cherry"));
    }
}

TEST_CASE("Serial synchronize flushes pending bits", "[serial][bitstream]")
{
    std::vector<char> buffer(256);
    {
        Serial serial(buffer.size(), buffer.data());
        serial.writeBit(true);
        serial.writeBit(false);
        serial.writeBit(true);
        serial.synchronize();
    }
    {
        Serial serial(buffer.size(), buffer.data());
        REQUIRE(serial.readBit() == true);
        REQUIRE(serial.readBit() == false);
        REQUIRE(serial.readBit() == true);
    }
}

TEST_CASE("Serial getWriteLength and getReadLength", "[serial][length]")
{
    std::stringstream ss;
    Serial serial(ss);

    REQUIRE(serial.getWriteLength() == 0);
    REQUIRE(serial.getReadLength() == 0);

    serial << std::string("hello world");
    serial.synchronize();

    REQUIRE(serial.getWriteLength() > 0);

    ss.seekg(0);
    serial.readStreamPointer = &ss;
    REQUIRE(serial.getReadLength() > 0);
}
