#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

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

    //initialize buf
    buf.index = 0;
    buf.size = 0;
    buf.data = NULL;

    //test open
    sstore_device = open("/dev/sstore0", O_RDWR);
    if (sstore_device < 0)
        perror("open");


    //test write with arbitrary values
    buf.index = 7;
    buf.size = 38;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(39);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }
    strncpy(buf.data, "A string of random characters this be.", 39);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\namount written = %d\n", bytes_written);


    //test read with arbitrary values
    buf.index = 7;
    buf.size = 38;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(39);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }   
    bytes_read = read(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_read < 0)
        perror("read");
    buf.data[38] = '\0';
    printf("\namount read = %d, data read = %s\n", bytes_read, buf.data);


    //test write with arbitrary values
    buf.index = 1;
    buf.size = 10;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(11);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }
    strncpy(buf.data, "abcdefghi\0", 10);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\namount written = %d\n", bytes_written);


    //test read with arbitrary values
    buf.index = 1;
    buf.size = 38;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(39);
    if (!buf.data) {
        printf("\nError in malloc: sstore_test.c\n");
        return 0;
    }   
    bytes_read = read(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_read < 0)
        perror("read");
    buf.data[9] = '\0';
    printf("\namount read = %d, data read = %s\n", bytes_read, buf.data);


    //test close
    if (sstore_device == 0)
    {
        close(sstore_device);
        perror("close");
    }

    return 0;
}
