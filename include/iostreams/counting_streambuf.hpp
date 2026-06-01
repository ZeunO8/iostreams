#pragma once
#include <streambuf>
#include <ios>
#include <cstring>

namespace iostreams
{
    struct counting_streambuf : public std::streambuf
    {
        using base_type = std::streambuf;

        char* buffer = nullptr;
        size_t buffer_size = 0;
        std::streamoff write_pos = 0;
        std::streamoff read_pos = 0;
        std::streamoff length = 0;
        std::streamsize total_bytes_written = 0;
        std::streamsize total_bytes_read = 0;

        counting_streambuf() = default;

        counting_streambuf(char* buf, size_t size)
            : buffer(buf), buffer_size(size)
        {
            char* end = buf + size;
            setp(buf, end);
            setg(buf, buf, end);
        }

        std::streamsize bytes_written() const { return total_bytes_written; }
        std::streamsize bytes_read() const { return total_bytes_read; }

        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            total_bytes_written += n;
            write_pos += n;
            if (write_pos > length)
                length = write_pos;
            return n;
        }

        int_type overflow(int_type ch) override
        {
            if (ch != traits_type::eof())
            {
                ++write_pos;
                ++total_bytes_written;
                if (write_pos > length)
                    length = write_pos;
            }
            return ch;
        }

        int_type underflow() override
        {
            if (read_pos < static_cast<std::streamoff>(buffer_size))
            {
                if (buffer)
                {
                    char* gstart = buffer + read_pos;
                    char* gend = buffer + buffer_size;
                    setg(gstart, gstart, gend);
                    return traits_type::to_int_type(*gptr());
                }
                return traits_type::to_int_type(' ');
            }
            return traits_type::eof();
        }

        std::streamsize xsgetn(char* s, std::streamsize n) override
        {
            std::streamsize total = 0;
            while (total < n)
            {
                if (gptr() >= egptr())
                {
                    if (underflow() == traits_type::eof())
                        break;
                }
                std::streamsize avail = egptr() - gptr();
                std::streamsize need = n - total;
                std::streamsize take = avail < need ? avail : need;
                std::memcpy(s + total, gptr(), static_cast<size_t>(take));
                gbump(static_cast<int>(take));
                read_pos += take;
                total_bytes_read += take;
                total += take;
            }
            return total;
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir way,
                         std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
        {
            if (which & std::ios_base::out)
            {
                switch (way)
                {
                case std::ios_base::beg: write_pos = off; break;
                case std::ios_base::cur: write_pos += off; break;
                case std::ios_base::end: write_pos = length + off; break;
                }
                if (write_pos > length) length = write_pos;
                return write_pos;
            }
            if (which & std::ios_base::in)
            {
                switch (way)
                {
                case std::ios_base::beg: read_pos = off; break;
                case std::ios_base::cur: read_pos += off; break;
                case std::ios_base::end: read_pos = length + off; break;
                }
                return read_pos;
            }
            return -1;
        }

        pos_type seekpos(pos_type sp,
                         std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
        {
            return seekoff(sp, std::ios_base::beg, which);
        }
    };
}
