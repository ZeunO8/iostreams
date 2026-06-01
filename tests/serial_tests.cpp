#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <iostreams/Serial.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

using Catch::Approx;

TEST_CASE("Serial serialize and deserialize primitives", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        int32_t i = 42;
        double d = 3.14159;
        float f = 1.5f;
        uint64_t u = 9999999999ULL;
        bool b = true;
        serial << i << d << f << u << b;
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        int32_t i = 0;
        double d = 0;
        float f = 0;
        uint64_t u = 0;
        bool b = false;
        serial >> i >> d >> f >> u >> b;
        REQUIRE(i == 42);
        REQUIRE(d == Approx(3.14159));
        REQUIRE(f == Approx(1.5f));
        REQUIRE(u == 9999999999ULL);
        REQUIRE(b == true);
    }
}

TEST_CASE("Serial serialize and deserialize string", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::string str = "hello world";
        serial << str;
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::string str;
        serial >> str;
        REQUIRE(str == "hello world");
    }
}

TEST_CASE("Serial serialize and deserialize vector", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::vector<int> vec = {1, 2, 3, 4, 5};
        serialize_vector(serial, vec);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::vector<int> vec;
        deserialize_vector(serial, vec);
        REQUIRE(vec.size() == 5);
        REQUIRE(vec[0] == 1);
        REQUIRE(vec[1] == 2);
        REQUIRE(vec[2] == 3);
        REQUIRE(vec[3] == 4);
        REQUIRE(vec[4] == 5);
    }
}

TEST_CASE("Serial deserialize vector into non-empty vector appends correctly", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::vector<int> vec = {10, 20};
        serialize_vector(serial, vec);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::vector<int> vec = {1, 2, 3};
        deserialize_vector(serial, vec);
        REQUIRE(vec.size() == 5);
        REQUIRE(vec[0] == 1);
        REQUIRE(vec[1] == 2);
        REQUIRE(vec[2] == 3);
        REQUIRE(vec[3] == 10);
        REQUIRE(vec[4] == 20);
    }
}

TEST_CASE("Serial serialize and deserialize unordered_map", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        map["one"] = 1;
        map["two"] = 2;
        serialize_unordered_map(serial, map);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        deserialize_unordered_map(serial, map);
        REQUIRE(map.size() == 2);
        REQUIRE(map["one"] == 1);
        REQUIRE(map["two"] == 2);
    }
}

TEST_CASE("Serial deserialize unordered_map into non-empty map", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        map["a"] = 100;
        serialize_unordered_map(serial, map);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        map["b"] = 200;
        deserialize_unordered_map(serial, map);
        REQUIRE(map.size() == 2);
        REQUIRE(map["a"] == 100);
        REQUIRE(map["b"] == 200);
    }
}

TEST_CASE("Serial serialize and deserialize map", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::map<std::string, int> map;
        map["x"] = 10;
        map["y"] = 20;
        serialize_map(serial, map);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::map<std::string, int> map;
        deserialize_map(serial, map);
        REQUIRE(map.size() == 2);
        REQUIRE(map["x"] == 10);
        REQUIRE(map["y"] == 20);
    }
}

TEST_CASE("Serial serialize and deserialize set", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::set<int> st = {5, 3, 1, 4, 2};
        serialize_set(serial, st);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::set<int> st;
        deserialize_set(serial, st);
        REQUIRE(st.size() == 5);
        REQUIRE(st.count(1));
        REQUIRE(st.count(2));
        REQUIRE(st.count(3));
        REQUIRE(st.count(4));
        REQUIRE(st.count(5));
    }
}

TEST_CASE("Serial serialize and deserialize unordered_set", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::unordered_set<int> st = {7, 8, 9};
        serialize_unordered_set(serial, st);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::unordered_set<int> st;
        deserialize_unordered_set(serial, st);
        REQUIRE(st.size() == 3);
        REQUIRE(st.count(7));
        REQUIRE(st.count(8));
        REQUIRE(st.count(9));
    }
}

TEST_CASE("Serial write and read position tracking", "[serial]")
{
    std::stringstream ss;
    Serial serial(ss);
    REQUIRE(serial.getWritePosition() == 0);
    serial << int32_t(42);
    REQUIRE(serial.getWritePosition() == 4);
}

TEST_CASE("Serial empty vector roundtrip", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::vector<int> vec;
        serialize_vector(serial, vec);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::vector<int> vec;
        deserialize_vector(serial, vec);
        REQUIRE(vec.empty());
    }
}

TEST_CASE("Serial empty map roundtrip", "[serial]")
{
    std::stringstream ss;
    {
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        serialize_unordered_map(serial, map);
        serial.synchronize();
    }
    {
        ss.seekg(0);
        Serial serial(ss);
        std::unordered_map<std::string, int> map;
        deserialize_unordered_map(serial, map);
        REQUIRE(map.empty());
    }
}
