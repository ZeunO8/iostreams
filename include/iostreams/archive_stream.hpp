#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <ios>

struct archive;
struct archive_entry;

namespace iostreams
{
    class archive_stream : public std::iostream
    {
    public:
        archive_stream(const std::string& archivePath, std::ios::openmode mode);

        ~archive_stream();

        std::vector<std::string> list_entries();

        void set_entry(const std::string& name);

        std::string get_entry_string();

    private:
        struct MemoryBuffer : public std::streambuf
        {
            std::vector<char> data;
            std::size_t pos = 0;
            bool dirty = true;

            MemoryBuffer();

            int_type overflow(int_type ch) override;

            int_type underflow() override;

            void reset_read();
        };

        void init_reader();

        void init_writer();

        void flush_write();

    private:
        struct archive* archive;
        struct archive_entry* entry;
        std::ios::openmode mode;
        std::string archivePath;
        std::string entryName;
        MemoryBuffer buffer;
        std::size_t pos;
    };
}