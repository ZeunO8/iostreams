#include <catch2/catch_test_macros.hpp>
#include <iostreams/archive_stream.hpp>
#include <fstream>
#include <cstdio>
#include <map>

using namespace iostreams;

static const std::string test_archive_path = "test_archive.tar.gz";

TEST_CASE("archive_stream creates write archive", "[archive]")
{
    std::remove(test_archive_path.c_str());
    archive_stream stream(test_archive_path, std::ios::out);
}

TEST_CASE("archive_stream writes single entry", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream stream(test_archive_path, std::ios::out);
        stream.set_entry("hello.txt");
        stream << "Hello, World!";
    }
    REQUIRE(std::ifstream(test_archive_path).good());
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream writes multiple entries", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream stream(test_archive_path, std::ios::out);
        stream.set_entry("file1.txt");
        stream << "Content of file 1";
        stream.set_entry("file2.txt");
        stream << "Content of file 2";
        stream.set_entry("file3.txt");
        stream << "Content of file 3";
    }
    REQUIRE(std::ifstream(test_archive_path).good());
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream list_entries returns correct names", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("alpha.txt");
        write_stream << "alpha content";
        write_stream.set_entry("beta.txt");
        write_stream << "beta content";
        write_stream.set_entry("gamma.txt");
        write_stream << "gamma content";
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        auto entries = read_stream.list_entries();
        REQUIRE(entries.size() == 3);
        REQUIRE(std::find(entries.begin(), entries.end(), "alpha.txt") != entries.end());
        REQUIRE(std::find(entries.begin(), entries.end(), "beta.txt") != entries.end());
        REQUIRE(std::find(entries.begin(), entries.end(), "gamma.txt") != entries.end());
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream read entry content", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("test.txt");
        write_stream << "This is test content";
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        read_stream.set_entry("test.txt");
        auto content = read_stream.get_entry_string();
        REQUIRE(content == "This is test content");
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream read multiple entries content", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("one.txt");
        write_stream << "First file content";
        write_stream.set_entry("two.txt");
        write_stream << "Second file content";
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        read_stream.set_entry("one.txt");
        REQUIRE(read_stream.get_entry_string() == "First file content");

        read_stream.set_entry("two.txt");
        REQUIRE(read_stream.get_entry_string() == "Second file content");
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream set_entry throws for missing entry", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("exists.txt");
        write_stream << "content";
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        REQUIRE_THROWS_AS(read_stream.set_entry("does_not_exist.txt"), std::runtime_error);
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream list_entries throws in write mode", "[archive]")
{
    std::remove(test_archive_path.c_str());
    archive_stream stream(test_archive_path, std::ios::out);
    REQUIRE_THROWS_AS(stream.list_entries(), std::runtime_error);
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream write and read roundtrip with integers", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("data.bin");
        int32_t val = 42;
        write_stream.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        read_stream.set_entry("data.bin");
        int32_t val = 0;
        read_stream.read(reinterpret_cast<char*>(&val), sizeof(val));
        REQUIRE(val == 42);
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream write and read roundtrip with large content", "[archive]")
{
    std::remove(test_archive_path.c_str());
    std::string large_content(10000, 'X');

    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("large.txt");
        write_stream << large_content;
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        read_stream.set_entry("large.txt");
        auto content = read_stream.get_entry_string();
        REQUIRE(content == large_content);
        REQUIRE(content.size() == 10000);
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream write and read roundtrip with empty entry", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("empty.txt");
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        read_stream.set_entry("empty.txt");
        auto content = read_stream.get_entry_string();
        REQUIRE(content.empty());
    }
    std::remove(test_archive_path.c_str());
}

TEST_CASE("archive_stream iterate all entries", "[archive]")
{
    std::remove(test_archive_path.c_str());
    {
        archive_stream write_stream(test_archive_path, std::ios::out);
        write_stream.set_entry("a.txt");
        write_stream << "content a";
        write_stream.set_entry("b.txt");
        write_stream << "content b";
        write_stream.set_entry("c.txt");
        write_stream << "content c";
    }

    {
        archive_stream read_stream(test_archive_path, std::ios::in);
        auto entries = read_stream.list_entries();
        std::map<std::string, std::string> contents;
        for (auto& name : entries)
        {
            read_stream.set_entry(name);
            contents[name] = read_stream.get_entry_string();
        }

        REQUIRE(contents["a.txt"] == "content a");
        REQUIRE(contents["b.txt"] == "content b");
        REQUIRE(contents["c.txt"] == "content c");
    }
    std::remove(test_archive_path.c_str());
}
