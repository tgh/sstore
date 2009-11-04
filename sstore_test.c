#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>  //man page open says is needed, but compiles without it
#include <sys/stat.h>   // "
#include <unistd.h>     //man page read/write says needed, but compiles without
#include </home/tgh/src/linux-source-2.6.28/include/asm-generic/ioctl.h>


#define SSTORE_IOCTL_MAGIC 0xFF
#define SSTORE_IOCTL_DELETE _IO(SSTORE_IOCTL_MAGIC, 0)


//this struct acts as a buffer to be sent in to read/write calls
struct readWriteBuffer {
    int index;      //index into the blob list to read from/write to
    int size;       //amount of data (bytes) to read/write
    char * data;    //the actual buffer that holds the data
};

int main ()
{
/*
    printf("\n\n");
    printf("**TEST program for TYLER HAYES' SSTORE device driver.\n");
    printf("**Use at your own risk.\n\n");
*/
    int sstore_device;          //file descriptor for the device
    struct readWriteBuffer buf; //buffer to be passed in through read and write
    int bytes_read = 0;
    int bytes_written = 0;
    int ioctl_return = 0;

    //initialize buf
    buf.index = 0;
    buf.size = 0;
    buf.data = NULL;


    //test open
    sstore_device = open("/dev/sstore0", O_RDWR);
    if (sstore_device < 0)
        perror("open");


    //test ioctl (before data)
    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 1);
    printf("\nioctl_return 0 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");


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


    //test ioctl
    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 1);
    printf("\nioctl_return 1 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");

    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 7);
    printf("\nioctl_return 2 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");

    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 6);
    printf("\nioctl_return 3 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");

    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 2);
    printf("\nioctl_return 4 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");

    ioctl_return = ioctl(sstore_device, 4643457, 1);
    printf("\nioctl_return 5 = %d", ioctl_return);
    if (ioctl_return == -ENOBLOB)
        printf("\nThere is no blob to delete at that index.\n");

    //test close
    if (sstore_device == 0)
    {
        close(sstore_device);
        perror("close");
    }

    return 0;
}
