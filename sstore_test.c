#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

struct readWriteBuffer {
    int index;
    int size;
    char * data;
};

int main ()
{
    int sstore_device;
    char buffer_container[1024];
    struct readWriteBuffer buf;
    int bytes_read = 0;
    int bytes_written = 0;

    //test open
    sstore_device = open("/dev/sstore0", O_RDONLY);
    if (sstore_device < 0)
        perror("open");

    //test read
    buf.index = 2;
    buf.size = 10;
    buf.data = (char *) malloc(11);
    bytes_read = read(sstore_device, &buf, 10);
    perror("read");
    printf("\namount read = %d, data read = %s\n", bytes_read, buf.data);

    //test write

    //test close
    if (sstore_device == 0)
    {
        close(sstore_device);
        perror("close");
    }

    return 0;
}
