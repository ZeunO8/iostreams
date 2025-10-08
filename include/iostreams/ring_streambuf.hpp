#pragma once

#include <atomic>
#include <streambuf>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <thread> // for std::this_thread::yield

namespace iostreams
{
    struct ring_header
    {
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
    };

    struct ringbuf : std::streambuf
    {
        char *buffer = nullptr;
        size_t buffer_size = 0;
        size_t ringbuffer_size = 0;
        char *ringbuffer = nullptr;
        ring_header *header = nullptr;
        std::atomic<size_t> *head_ptr = nullptr;
        std::atomic<size_t> *tail_ptr = nullptr;
        bool non_blocking = false;

        ringbuf() = default;

        ringbuf(char *mem, size_t total_size, bool non_blocking = false):
            buffer(mem),
            buffer_size(total_size),
            header(reinterpret_cast<ring_header *>(mem)),
            ringbuffer(mem + sizeof(ring_header)),
            ringbuffer_size(total_size - sizeof(ring_header)),
            head_ptr(&header->head),
            tail_ptr(&header->tail),
            non_blocking(non_blocking)
        {
        }

        ringbuf(const ringbuf& other):
            buffer(other.buffer),
            buffer_size(other.buffer_size),
            header(other.header),
            ringbuffer(other.ringbuffer),
            ringbuffer_size(other.ringbuffer_size),
            head_ptr(other.head_ptr),
            tail_ptr(other.tail_ptr),
            non_blocking(other.non_blocking)
        {}

        ringbuf& operator=(const ringbuf& other)
        {
            buffer = other.buffer;
            buffer_size = other.buffer_size;
            header = other.header;
            ringbuffer = other.ringbuffer;
            ringbuffer_size = other.ringbuffer_size;
            head_ptr = other.head_ptr;
            tail_ptr = other.tail_ptr;
            non_blocking = other.non_blocking;
            return *this;
        }

        size_t available_read() const noexcept
        {
            size_t h = (*head_ptr);
            size_t t = (*tail_ptr);
            return (h + ringbuffer_size - t) % ringbuffer_size;
        }

        size_t available_write() const noexcept
        {
            size_t h = (*head_ptr);
            size_t t = (*tail_ptr);
            return (ringbuffer_size - 1 + t - h) % ringbuffer_size;
        }

        size_t write_internal(const char *s, size_t n)
        {
            size_t written = 0;
            while (written < n)
            {
                size_t avail = available_write();
                if (avail == 0)
                {
                    if (non_blocking)
                        break;
                    std::this_thread::yield();
                    continue;
                }

                size_t chunk = (std::min)(avail, n - written);
                size_t h = (*head_ptr);
                size_t end = (std::min)(chunk, ringbuffer_size - h);

                std::memcpy(ringbuffer + h, s + written, end);
                if (chunk > end)
                    std::memcpy(ringbuffer, s + written + end, chunk - end);

                (*head_ptr) = (h + chunk) % ringbuffer_size;
                written += chunk;
            }
            return written;
        }

        size_t read_internal(char *s, size_t n)
        {
            size_t read_bytes = 0;
            while (read_bytes < n)
            {
                size_t avail = available_read();
                if (avail == 0)
                {
                    if (non_blocking)
                        break;
                    std::this_thread::yield();
                    continue;
                }

                size_t chunk = (std::min)(avail, n - read_bytes);
                size_t t = (*tail_ptr);
                size_t end = (std::min)(chunk, ringbuffer_size - t);

                std::memcpy(s + read_bytes, ringbuffer + t, end);
                if (chunk > end)
                    std::memcpy(s + read_bytes + end, ringbuffer, chunk - end);

                (*tail_ptr) = ((t + chunk) % ringbuffer_size);
                read_bytes += chunk;
            }
            return read_bytes;
        }

        std::streamsize xsputn(const char *s, std::streamsize n) override
        {
            return static_cast<std::streamsize>(write_internal(s, static_cast<size_t>(n)));
        }

        std::streamsize xsgetn(char *s, std::streamsize n) override
        {
            return static_cast<std::streamsize>(read_internal(s, static_cast<size_t>(n)));
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override
        {
            const size_t cap = ringbuffer_size;

            if (which & std::ios_base::out)
            {
                size_t pos = (*head_ptr);
                switch (way)
                {
                case std::ios_base::beg:
                    pos = static_cast<size_t>(off) % cap;
                    break;
                case std::ios_base::cur:
                    pos = (pos + off) % cap;
                    break;
                case std::ios_base::end:
                    pos = (*tail_ptr);
                    break;
                }
                (*head_ptr) = pos;
                return static_cast<pos_type>(pos);
            }

            if (which & std::ios_base::in)
            {
                size_t pos = (*tail_ptr);
                switch (way)
                {
                case std::ios_base::beg:
                    pos = static_cast<size_t>(off) % cap;
                    break;
                case std::ios_base::cur:
                    pos = (pos + off) % cap;
                    break;
                case std::ios_base::end:
                    pos = (*head_ptr);
                    break;
                }
                (*tail_ptr) = pos;
                return static_cast<pos_type>(pos);
            }

            return pos_type(off_type(-1));
        }

        pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
        {
            return seekoff(static_cast<off_type>(sp), std::ios_base::beg, which);
        }
    };
}
