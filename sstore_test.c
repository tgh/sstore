#include <stdio.h>
#include <fcntl.h>

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

    //test open
    sstore_device = open("/dev/sstore0", O_RDONLY);
    if (sstore_device < 0)
        perror("open");

    //test read

    //test write
    buf.index = 5;
    buf.size = 10;
    buf.data = "Hello world";

    //test close
    if (sstore_device == 0)
    {
        close(sstore_device);
        perror("close");
    }

    return 0;
}
