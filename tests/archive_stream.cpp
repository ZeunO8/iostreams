#include <iostreams/archive_stream.hpp>
#include <iostream>
using namespace iostreams;
int main()
{
    {
        archive_stream stream("archive.tar.gz", std::ios::out);
        stream.set_entry("test.txt");
        stream << "Hello, archive!";
        stream.set_entry("test2.txt");
        stream << "Hello, value: " + std::to_string(5.28);
    }
    {
        archive_stream stream("archive.tar.gz", std::ios::in);
        for (auto& name : stream.list_entries())
        {
            stream.set_entry(name);
            auto contents = stream.get_entry_string();
            std::cout << name << ":" << std::endl << contents << std::endl;
        }
    }
    return 0;
}