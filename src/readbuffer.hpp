//
// Created by cory on 4/18/25.
//

#ifndef CPPTEST_READBUFFER_HPP
#define CPPTEST_READBUFFER_HPP


#include <string>
#include "uuid.hpp"

class ReadBuffer
{
public:
    ReadBuffer();

    [[nodiscard]] inline bool good() const { return _good; }

    void feed(char* buffer, unsigned long size);

    char read_char();

    unsigned long read_ulong();

    unsigned short read_ushort();

    UUID read_uuid();

    int read_varint();

    std::string read_string();
private:
    char* _buffer;
    char* _end;
    bool _good;
};


#endif //CPPTEST_READBUFFER_HPP
