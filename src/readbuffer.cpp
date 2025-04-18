//
// Created by cory on 4/18/25.
//

#include "readbuffer.hpp"

#define SEGMENT_BITS (0b01111111)
#define CONTINUE_BIT (0b10000000)

ReadBuffer::ReadBuffer() : _buffer(nullptr), _end(nullptr), _good(false)
{}

void ReadBuffer::feed(char *buffer, unsigned long size)
{
    _buffer = buffer;
    _end = buffer + size;
    _good = true;
}

char ReadBuffer::read_char()
{
    if (_buffer >= _end)
    {
        _good = false;
        return '\0';
    }

    return *_buffer++;
}

int ReadBuffer::read_varint()
{
    int value = 0;
    int position = 0;
    char c;

    while (true)
    {
        c = read_char();
        // pos = 0     pos = 7     pos = 14    pos = 21    pos = 28   pos = 35 (ERR)
        // 0b10000001, 0b10000001, 0b10000001, 0b10000001, 0b00000001
        // for the last byte, the most significant half byte has to be 0000.
        value |= (c & SEGMENT_BITS) << position;

        if (!(c & CONTINUE_BIT))
        {
            break;
        }

        position += 7;

        if (position > 21)
        {
            // we're reading the very last char, so we need to check for malformed data
            c = read_char();
            if ((c & 0xf0))
            {
                /* The varint is trying to compile more than 5 bytes or data
                   in the last byte which is too large to fit into an int */
                _good = false;
            }

            value |= (c & SEGMENT_BITS) << position;
            break;
        }
    }

    return value;
}

unsigned long ReadBuffer::read_ulong()
{
    if (_end - _buffer < sizeof(unsigned long))
    {
        _good = false;
        return 0;
    }

    unsigned long rax = __builtin_bswap64(*reinterpret_cast<unsigned long*>(_buffer));
    _buffer += sizeof(unsigned long);
    return rax;
}

unsigned short ReadBuffer::read_ushort()
{
    if (_end - _buffer < sizeof(unsigned short))
    {
        _good = false;
        return 0;
    }

    unsigned short rax = __builtin_bswap16(*reinterpret_cast<unsigned short*>(_buffer));
    _buffer += sizeof(unsigned short);
    return rax;
}

std::string ReadBuffer::read_string()
{
    int size = read_varint();

    if (!_good)
    {
        return "";
    }

    if (_end - _buffer < size)
    {
        _good = false;
        return "";
    }

    std::string rax = std::string(_buffer, size);
    _buffer += size;
    return rax;
}

UUID ReadBuffer::read_uuid()
{
    return {read_ulong(), read_ulong()};
}