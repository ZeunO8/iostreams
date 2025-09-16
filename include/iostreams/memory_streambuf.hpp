#pragma once
#include <streambuf>
#include <ios>

namespace iostreams
{
    class memory_streambuf : public std::streambuf
    {
    public:
        memory_streambuf(char* data, std::size_t size)
        {
            char* base = data;
            char* end  = data + size;

            // readable area
            setg(base, base, end);

            // writable area
            setp(base, end);
        }

    protected:
        // Write multiple characters
        std::streamsize xsputn(const char* s, std::streamsize n) override
        {
            std::streamsize avail = epptr() - pptr();
            if (n > avail)
                n = avail;

            std::memcpy(pptr(), s, static_cast<std::size_t>(n));
            pbump(static_cast<int>(n));
            return n;
        }

        // Read multiple characters
        std::streamsize xsgetn(char* s, std::streamsize n) override
        {
            std::streamsize avail = egptr() - gptr();
            if (n > avail)
                n = avail;

            std::memcpy(s, gptr(), static_cast<std::size_t>(n));
            gbump(static_cast<int>(n));
            return n;
        }

        // Write single character (when put area is full or overflow triggered)
        int_type overflow(int_type ch) override
        {
            if (ch == traits_type::eof())
                return traits_type::eof();

            if (pptr() < epptr())
            {
                *pptr() = traits_type::to_char_type(ch);
                pbump(1);
                return ch;
            }
            return traits_type::eof(); // no space left
        }

        // Read single character
        int_type underflow() override
        {
            if (gptr() < egptr())
                return traits_type::to_int_type(*gptr());
            return traits_type::eof();
        }

        // Seek relative
        pos_type seekoff(off_type off, std::ios_base::seekdir way,
                         std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
        {
            char* newpos = nullptr;

            if (which & std::ios_base::in)
            {
                switch (way)
                {
                case std::ios_base::beg: newpos = eback() + off; break;
                case std::ios_base::cur: newpos = gptr() + off; break;
                case std::ios_base::end: newpos = egptr() + off; break;
                default: return pos_type(off_type(-1));
                }
                if (newpos < eback() || newpos > egptr())
                    return pos_type(off_type(-1));
                setg(eback(), newpos, egptr());
                return pos_type(newpos - eback());
            }

            if (which & std::ios_base::out)
            {
                switch (way)
                {
                case std::ios_base::beg: newpos = pbase() + off; break;
                case std::ios_base::cur: newpos = pptr() + off; break;
                case std::ios_base::end: newpos = epptr() + off; break;
                default: return pos_type(off_type(-1));
                }
                if (newpos < pbase() || newpos > epptr())
                    return pos_type(off_type(-1));
                setp(pbase(), epptr());
                pbump(static_cast<int>(newpos - pbase()));
                return pos_type(newpos - pbase());
            }

            return pos_type(off_type(-1));
        }

        // Seek absolute
        pos_type seekpos(pos_type sp,
                         std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
        {
            return seekoff(off_type(sp), std::ios_base::beg, which);
        }
    };
}
