//
// Created by cory on 4/15/25.
//

#ifndef CPPTEST_WRITEBUFFER_HPP
#define CPPTEST_WRITEBUFFER_HPP


#include <bits/types/struct_iovec.h>

class WriteBuffer
{
public:
    explicit WriteBuffer(unsigned long size);

    ~WriteBuffer();

    void write_bool(bool x);

    void write_byte(char x);

    void write_ubyte(unsigned char x);

    void write_short(short x);

    void write_ushort(unsigned short x);

    /**
     * Writes a signed 32-bit little-endian integer (defined for little-endian systems.)
     */
    void write_int_le(int x);

    /**
     * Writes a signed 32-bit big-endian integer (defined for little-endian systems.)
     */
    void write_int(int x);

    void write_long(long x);

    void write_float(float x);

    void write_double(double x);

    void write_varint(int x);

    void write_varlong(int x);

    void write_bytes(char* bytes, int size);

    [[nodiscard]] inline int iov_size() const noexcept { return iov_cursor; }

    iovec* iov();
private:
    /**
     * @return Measurement of how many bytes are allocated in the byte buffer.
     */
    [[nodiscard]] inline unsigned long buffer_size() const;

    /**
     * @return Measurement of how many bytes are written to.
     */
    [[nodiscard]] inline unsigned long utilized_size() const;

    /**
     * @return Measurement of how many bytes are in the current sector.
     */
    [[nodiscard]] inline unsigned long sector_size() const;

    /**
     * @return Measurement of how many bytes the write cursor has until it runs out of space,
     *         computed as the distance between the cursor and the end of the buffer.
     */
    [[nodiscard]] inline unsigned long sector_remaining() const;

    /**
     * Writes the given bytes of the given size to the iov at the current iov_cursor position.
     */
    void write_iovector(char* bytes, size_t size);

    void ensure_capacity(size_t bytes);
    
    void buffer_resize(size_t bytes);

    void iov_resize(int size);

    /**
     * Writes the current sector to the _iov at iov_cursor, dynamically resizing the _iov
     * if necessary to fill it. This also establishes a new sector for future writes.
     */
    void flush_buffer();

    int _iov_size;
    iovec* _iov;
    int iov_cursor;

    /**
     * Pointer to the beginning of the allocated buffer owned by this write
     * buffer. All internally stored data is written into this memory, and
     * it is freed when the buffer is destroyed.
     *
     * This should only change if the buffer is resized for any reason.
     */
    char* buf;
    /**
     * Points to one past the last byte in the internal buffer (buf + size).
     * Used to ensure that writes do not overflow the allocated memory region.
     *
     * This should also only be changed if the buffer is resized for any reason,
     * otherwise it is to remain constant.
     */
    char* end;
    /**
     * Marks the start of the current sector in the internal buffer. Each iovec
     * entry that refers to internal storage begins from this pointer, and spans
     * up to the current cursor position at the time the sector is closed.
     */
    char* sector_start;
    /**
     * Pointer to the current write position within the internal buffer. As
     * owned data is written, this advances forward. May be used to construct
     * iovec entries for buffer-backed writes.
     */
    char* cursor;
};


#endif //CPPTEST_WRITEBUFFER_HPP
