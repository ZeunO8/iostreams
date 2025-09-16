#pragma once
#include <streambuf>
#include <ios>

namespace iostreams
{
    struct counting_streambuf : public std::streambuf
    {
        using base_type = std::streambuf;

        std::streamoff write_pos = 0;   // current write position
        std::streamoff read_pos = 0;    // current read position
        std::streamoff length = 0;      // maximum written

        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
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
                if (write_pos > length)
                    length = write_pos;
            }
            return ch;
        }

        int_type underflow() override
        {
            if (read_pos < length)
                return traits_type::to_int_type(' '); // dummy
            return traits_type::eof();
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
