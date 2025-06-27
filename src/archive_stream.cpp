#include <archive.h>
#include <archive_entry.h>
#include <iostreams/archive_stream.hpp>
using namespace iostreams;

archive_stream::archive_stream(const std::string& archivePath, std::ios::openmode mode):
    std::iostream(nullptr),
    archive(nullptr),
    entry(nullptr),
    mode(mode),
    archivePath(archivePath),
    pos(0)
{
    if (mode & std::ios::in)
    {
        init_reader();
    }
    if (mode & std::ios::out)
    {
        init_writer();
    }

    this->rdbuf(&buffer);
}

archive_stream::~archive_stream()
{
    if ((mode & std::ios::out) && entry)
    {
        flush_write();
    }

    if (archive)
    {
        if (mode & std::ios::in)
        {
            archive_read_close(archive);
            archive_read_free(archive);
        }
        else if (mode & std::ios::out)
        {
            archive_write_close(archive);
            archive_write_free(archive);
        }
    }

    if (entry)
    {
        archive_entry_free(entry);
    }
}

std::vector<std::string> archive_stream::list_entries()
{
    if (!(mode & std::ios::in))
    {
        throw std::runtime_error("list_entries is only available in read mode");
    }

    std::vector<std::string> entries;
    struct archive* arc = archive_read_new();
    archive_read_support_format_all(arc);
    archive_read_support_filter_all(arc);

    if (archive_read_open_filename(arc, archivePath.c_str(), 10240) != ARCHIVE_OK)
    {
        throw std::runtime_error(archive_error_string(arc));
    }

    struct archive_entry* e;
    while (archive_read_next_header(arc, &e) == ARCHIVE_OK)
    {
        entries.emplace_back(archive_entry_pathname(e));
        archive_read_data_skip(arc);
    }

    archive_read_close(arc);
    archive_read_free(arc);

    return entries;
}

void archive_stream::set_entry(const std::string& name)
{
    // Flush current entry before switching
    if ((mode & std::ios::out) && entry)
    {
        flush_write();
        archive_entry_free(entry);
        entry = nullptr;
    }

    buffer.data.clear();
    buffer.pos = 0;
    buffer.dirty = true;
    entryName = name;

    if (mode & std::ios::in)
    {
        struct archive* arc = archive_read_new();
        archive_read_support_format_all(arc);
        archive_read_support_filter_all(arc);

        if (archive_read_open_filename(arc, archivePath.c_str(), 10240) != ARCHIVE_OK)
        {
            throw std::runtime_error(archive_error_string(arc));
        }

        struct archive_entry* e;
        while (archive_read_next_header(arc, &e) == ARCHIVE_OK)
        {
            if (entryName == archive_entry_pathname(e))
            {
                size_t size = archive_entry_size(e);
                buffer.data.resize(size);
                archive_read_data(arc, buffer.data.data(), size);
                archive_read_close(arc);
                archive_read_free(arc);
                return;
            }
            archive_read_data_skip(arc);
        }

        archive_read_close(arc);
        archive_read_free(arc);
        throw std::runtime_error("Entry not found in archive: " + entryName);
    }

    if (mode & std::ios::out)
    {
        entry = archive_entry_new();
        archive_entry_set_pathname(entry, entryName.c_str());
    }
}

std::string archive_stream::get_entry_string()
{
    return std::string(
        (std::istreambuf_iterator<char>(*this)),
        std::istreambuf_iterator<char>()
    );
}

archive_stream::MemoryBuffer::MemoryBuffer()
{
    data.reserve(4096);
}

iostreams::archive_stream::int_type archive_stream::MemoryBuffer::overflow(int_type ch)
{
    if (ch != EOF)
    {
        data.push_back(static_cast<char>(ch));
    }
    return ch;
}

iostreams::archive_stream::int_type archive_stream::MemoryBuffer::underflow()
{
    if (gptr() == nullptr || dirty)
    {
        dirty = false;
        if (data.empty())
        {
            return traits_type::eof();
        }

        setg(data.data(), data.data(), data.data() + data.size());
        return traits_type::to_int_type(*gptr());
    }

    if (gptr() >= egptr())
    {
        return traits_type::eof();
    }

    return traits_type::to_int_type(*gptr());
}

void archive_stream::MemoryBuffer::reset_read()
{
    dirty = true;
}

void archive_stream::init_reader()
{
    archive = archive_read_new();
    archive_read_support_format_all(archive);
    archive_read_support_filter_all(archive);
}

void archive_stream::init_writer()
{
    archive = archive_write_new();
    archive_write_set_format_pax_restricted(archive);
    archive_write_add_filter_gzip(archive);

    if (archive_write_open_filename(archive, archivePath.c_str()) != ARCHIVE_OK)
    {
        throw std::runtime_error(archive_error_string(archive));
    }
}

void archive_stream::flush_write()
{
    archive_entry_set_size(entry, buffer.data.size());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);

    archive_write_header(archive, entry);
    archive_write_data(archive, buffer.data.data(), buffer.data.size());
}