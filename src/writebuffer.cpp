//
// Created by cory on 4/15/25.
//

#include "writebuffer.hpp"

#define EXTRA_CAPACITY (16)

WriteBuffer::WriteBuffer(unsigned long size) : _iov_size(1), _iov(new iovec[1]), iov_cursor(0),
                                               buf(new char[size]), end(buf + size), sector_start(buf), cursor(buf)
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
    iovec* resized = new iovec[size];

    for (int i = 0; i < _iov_size; ++i)
    {
        resized[i] = _iov[i];
    }

    delete[] _iov;

    _iov_size = size;
    _iov = resized;
}

void WriteBuffer::write_byte(char x)
{
    ensure_capacity(sizeof(char));

    *cursor = x;
    cursor += sizeof(char);
}

void WriteBuffer::write_int_le(int x)
{
    ensure_capacity(sizeof(int));

    *reinterpret_cast<int*>(cursor) = x;
    cursor += sizeof(int);
}

void WriteBuffer::write_int(int x)
{
    write_int_le(static_cast<int>(__builtin_bswap32(x)));
}

void WriteBuffer::write_varint(int x)
{
    while (true)
    {
        if ((x & 0xffffff80) == 0)
        {
            write_byte(static_cast<char>(x));
            return;
        }

        write_byte(static_cast<char>((x & 0x7f) | 0x80));

        x >>= 7;
    }
}

void WriteBuffer::write_iovector(char* bytes, size_t size)
{
    iovec* vec = _iov + iov_cursor;
    vec->iov_base = bytes;
    vec->iov_len = size;
    // sector was written, we're looking at the next vector now
    ++iov_cursor;

    // create new sector
    sector_start = cursor;
}

void WriteBuffer::write_bytes(char* bytes, int size)
{
    unsigned long sector_len = sector_size();
    bool sector_exists = sector_len > 0;

    if (iov_cursor >= _iov_size - sector_exists - 1)
    {
        // if there needs to be a resize to fit the incoming sector, lets do it
        iov_resize(iov_cursor + 2);
    }

    if (sector_exists)
    {
        write_iovector(sector_start, sector_len);
    }

    write_iovector(bytes, size);
}

void WriteBuffer::flush_buffer()
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

        write_iovector(sector_start, sector_len);
    }
}

iovec* WriteBuffer::iov()
{
    flush_buffer();

    return _iov;
}
