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
    WriteBuffer wbuf{10};

    wbuf.write_varint(256);
    wbuf.write_string("Hello World");
    wbuf.write_bytes("Hello World", 11);

    iovec* iov = wbuf.finalize();

    writev(write_fd, iov, wbuf.iov_size());

    wbuf.reset();

    // WRITE END

    writev(1, iov, wbuf.iov_size());
    std::cout << std::endl;

    ssize_t res = read(read_fd, buf, sizeof(buf));

    // READ (parse) START

    ReadBuffer rbuf;

    rbuf.feed(buf, res);

    std::cout << "length: " << rbuf.read_varint() << std::endl;
    std::cout << "varint: " << rbuf.read_varint() << std::endl;
    std::cout << "string: " << rbuf.read_string() << std::endl;

    // READ END

    return 0;
}
