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
    int sstore_device;          //file descriptor for the device
    struct readWriteBuffer buf; //buffer to be passed in through read and write
    int bytes_read = 0;
    int bytes_written = 0;

    //test open
    sstore_device = open("/dev/sstore0", O_RDONLY);
    if (sstore_device < 0)
        perror("open");

    //test read with arbitrary values
    buf.index = 2;
    buf.size = 10;
    buf.data = (char *) malloc(11);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }   
    bytes_read = read(sstore_device, &buf, buf.size);
    if (bytes_read < 0)
        perror("read");
    buf.data[10] = '\0';
    printf("\namount read = %d, data read = %s\n", bytes_read, buf.data);

/*
    //test write with arbitrary values
    buf.index = 7;
    buf.size = 21;
    free(buf.data);
    buf.data = (char *) malloc(22);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }
    bytes_written = write(sstore_device, &buf, buf.size);
    perror("write");
    printf("\namount written = %d\n", bytes_written);
*/

    //test close
    if (sstore_device == 0)
    {
        close(sstore_device);
        perror("close");
    }

    return 0;
}
