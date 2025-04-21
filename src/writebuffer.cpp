//
// Created by cory on 4/15/25.
//

#include "writebuffer.hpp"

WriteBuffer::WriteBuffer(unsigned long size) : _iov_size(1), _iov(new iovec[1]), iov_cursor(0), packet_length(0),
                                               buf(new char[size + 5]), end(buf + size + 5), sector_start(buf + 5), cursor(sector_start)
{}

WriteBuffer::~WriteBuffer()
{
    delete[] buf;
    delete[] _iov;
}

unsigned long WriteBuffer::buffer_size() const
{
    return end - buf;
}

unsigned long WriteBuffer::utilized_size() const
{
    return cursor - buf;
}

unsigned long WriteBuffer::sector_size() const
{
    return cursor - sector_start;
}

unsigned long WriteBuffer::sector_remaining() const
{
    return end - cursor;
}

void WriteBuffer::buffer_resize(unsigned long bytes)
{
    char* resized = new char[bytes];
    unsigned long to_copy = utilized_size();

    if (bytes < to_copy)
    {
        // if the new size is less than to_copy, we're not going to copy what doesn't fit in the new buffer
        to_copy = bytes;
    }

    for (unsigned long i = 0; i < to_copy; ++i)
    {
        // copy the current buffer to the new one
        resized[i] = buf[i];
    }

    end = resized + bytes;
    sector_start = sector_start - buf + resized;
    cursor = cursor - buf + resized;
    delete[] buf;
    buf = resized;
}

#define EXTRA_CAPACITY (32)

void WriteBuffer::ensure_capacity(size_t bytes)
{
    if (sector_remaining() < bytes)
    {
        // todo: figure out something better for resizing?
        buffer_resize(buffer_size() + bytes + EXTRA_CAPACITY);
    }
}

void WriteBuffer::iov_resize(int size)
{
    // todo check for max iovec size
    auto* resized = new iovec[size];

    for (int i = 0; i < _iov_size; ++i)
    {
        resized[i] = _iov[i];
    }

    delete[] _iov;

    _iov_size = size;
    _iov = resized;
}

#define WRITE(type, x) ensure_capacity(sizeof(type)); *reinterpret_cast<type*>(cursor) = x; cursor += sizeof(type);

void WriteBuffer::write_bool(bool x)
{
    WRITE(bool, x)
}

void WriteBuffer::write_byte(char x)
{
    WRITE(char, x)
}

void WriteBuffer::write_short_le(short x)
{
    WRITE(short, x)
}

void WriteBuffer::write_short(short x)
{
    write_short_le(static_cast<short>(__builtin_bswap16(x)));
}

void WriteBuffer::write_int_le(int x)
{
    WRITE(int, x)
}

void WriteBuffer::write_int(int x)
{
    write_int_le(static_cast<int>(__builtin_bswap32(x)));
}

void WriteBuffer::write_long_le(long x)
{
    WRITE(long, x)
}

void WriteBuffer::write_long(long x)
{
    write_long_le(static_cast<long>(__builtin_bswap64(x)));
}

void WriteBuffer::write_float_le(float x)
{
    WRITE(float, x)
}

void WriteBuffer::write_float(float x)
{
    write_float_le(static_cast<float>(__builtin_bswap32(*reinterpret_cast<int*>(&x))));
}

void WriteBuffer::write_double_le(double x)
{
    WRITE(double, x)
}

void WriteBuffer::write_double(double x)
{
    write_double_le(static_cast<double>(__builtin_bswap64(*reinterpret_cast<long*>(&x))));
}

#define WRITE_VARINT(type, x)                             \
    while (true)                                          \
    {                                                     \
        if ((x & ~0x7f) == 0)                             \
        {                                                 \
            write_byte(static_cast<char>(x));             \
            return;                                       \
        }                                                 \
        write_byte(static_cast<char>((x & 0x7f) | 0x80)); \
        x >>= 7;                                          \
    }                                                     \

void WriteBuffer::write_varint(int x)
{
    WRITE_VARINT(int, x)
}

void WriteBuffer::write_varlong(long x)
{
    WRITE_VARINT(long, x)
}

void WriteBuffer::write_iov(char* bytes, int size)
{
    packet_length += size;

    iovec* vec = _iov + iov_cursor;
    vec->iov_base = bytes;
    vec->iov_len = size;
    // sector was written, we're looking at the next vector now
    ++iov_cursor;

    // create new sector
    sector_start = cursor;
}

void WriteBuffer::write_bytes(char* bytes, size_t size)
{
    unsigned long sector_len = sector_size();
    bool sector_exists = sector_len > 0;

    // ensure the size is big enough for our two writes to the iovector
    if (iov_cursor >= _iov_size - sector_exists - 1)
    {
        // if there needs to be a resize to fit the incoming sector, lets do it
        iov_resize(iov_cursor + 2);
    }

    if (sector_exists)
    {
        write_iov(sector_start, sector_len);
    }

    write_iov(bytes, size);
}

void WriteBuffer::write_string(const std::string& str)
{
    write_varint(static_cast<int>(str.size()));
    write_bytes(const_cast<char*>(str.c_str()), str.size());
}

bool WriteBuffer::flush_buffer()
{
    size_t sector_len = sector_size();

    if (sector_len > 0)
    {
        // if the buffer is not empty
        // resize _iov if necessary
        if (iov_cursor >= _iov_size - 1)
        {
            // if there needs to be a resize to fit the incoming sector, lets do it
            iov_resize(iov_cursor + 1);
        }

        write_iov(sector_start, sector_len);
        return true;
    }
    return false;
}

iovec* WriteBuffer::finalize()
{
    if (iov_cursor || flush_buffer())
    {
        // if the buffer was flushed, we can guarentee at least one iovec present

        // write packet_length to len
        char len[5];
        char* curs = len;
        int x = packet_length;

        while (true)
        {
            if ((x & ~0x7f) == 0)
            {
                *curs++ = static_cast<char>(x);
                break;
            }
            *curs++ = static_cast<char>((x & 0x7f) | 0x80);
            x >>= 7;
        }

        // length of the length
        // we need to rewrite the first sector pointer to account for the length of the length that's getting written
        long len_length = curs - len;

        char* start = reinterpret_cast<char*>(_iov[0].iov_base) - len_length;
        _iov[0].iov_base = start;
        _iov[0].iov_len += len_length;

        // copy the written length from len to the start of the buffer
        for (int i = 0; i < len_length; ++i)
        {
            start[i] = len[i];
        }
    }

    return _iov;
}

void WriteBuffer::reset()
{
    sector_start = buf;
    cursor = buf;
    iov_cursor = 0;
}
