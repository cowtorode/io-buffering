#include <iostream>
#include <chrono>
#include <sys/uio.h>
#include <climits>
#include <unistd.h>
#include "writebuffer.hpp"
#include "readbuffer.hpp"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;

int main()
{
    std::cout << "IOV_MAX: " << IOV_MAX << std::endl;

    char buf[1024];
    int fds[2];
    pipe(fds);

    int write_fd = fds[1];
    int read_fd = fds[0];

    // TESTING START NOW

    // WRITE START
    WriteBuffer out{10};

    int length = 256;
    int packet_id = 0;

    out.write_varint(length);
    out.write_varint(packet_id);

    out.write_bytes("Hello World", 11);
    out.write_bytes("Hello World", 11);

    iovec* iov = out.iov();

    writev(write_fd, iov, out.iov_size());

    // WRITE END

    writev(1, iov, out.iov_size());
    std::cout << std::endl;

    ssize_t res = read(fds[0], buf, sizeof(buf));

    // READ (parse) START

    ReadBuffer in;

    in.feed(buf, res);

    length = in.read_varint();
    packet_id = in.read_varint();

    std::cout << "length: " << length << std::endl;
    std::cout << "packet_id: " << packet_id << std::endl;

    // READ END

    return 0;
}
