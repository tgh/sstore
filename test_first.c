/*
 * Tyler Hayes - sstore device driver test program 1 of 2.
 *
 * There are two in order to test concurrency.  There are breaks in between
 * operations so that you can check /proc/sstore/data and /proc/sstore/stats
 * from another terminal if you want to.
 */

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

void printContinuePrompt();

//this struct acts as a buffer to be sent in to read/write calls
struct readWriteBuffer {
    int index;      //index into the blob list to read from/write to
    int size;       //amount of data (bytes) to read/write
    char * data;    //the actual buffer that holds the data
};

int main ()
{
    int sstore_device;          //file descriptor for the device
    struct readWriteBuffer buf; //buffer to be passed in through read and write
    int bytes_read = 0;
    int bytes_written = 0;
    int ioctl_return = 0;

    //initialize buf
    buf.index = 0;
    buf.size = 0;
    buf.data = NULL;


    //open the device
    printf("\nOpening device.");
    sstore_device = open("/dev/sstore0", O_RDWR);
    if (sstore_device < 0)
        perror("open");
    

    printContinuePrompt();


    //write to the device using an index of 7
    printf("\nWriting to index 7.");
    buf.index = 7;
    buf.size = 39;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(40);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }
    strncpy(buf.data, "A string of random characters this be.\0", 40);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\n\tAmount written = %d\n", bytes_written);


    printContinuePrompt();


    //write to the device using an index of 5
    printf("\nWriting to index 5.");
    buf.index = 5;
    buf.size = 15;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(16);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }
    strncpy(buf.data, "Another string\0", 16);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\n\tAmount written = %d\n", bytes_written);


    printContinuePrompt();


    //write to the device using an index of 2
    printf("\nWriting to index 2.");
    buf.index = 2;
    buf.size = 10;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(11);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }
    strncpy(buf.data, "Password:\0", 11);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\n\tAmount written = %d\n", bytes_written);


    printContinuePrompt();


    //read from the device using an index of 7
    printf("\nReading from index 7.");
    buf.index = 7;
    buf.size = 38;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(39);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }   
    bytes_read = read(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_read < 0)
        perror("read");
    buf.data[38] = '\0';
    printf("\n\tAmount read = %d, data read = %s.\n", bytes_read, buf.data);


    printContinuePrompt();


    //write to the device using an index of 4
    printf("\nWriting to index 4.");
    buf.index = 4;
    buf.size = 21;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(22);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }
    strncpy(buf.data, "Linux Device Drivers\0", 22);
    bytes_written = write(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_written < 0)
        perror("write");
    printf("\n\tAmount written = %d\n", bytes_written);


    printContinuePrompt();


    //read from the device using an index of 1
    printf("\nReading from index 1.");
    buf.index = 1;
    buf.size = 38;
    if (buf.data)
        free(buf.data);
    buf.data = (char *) malloc(39);
    if (!buf.data) {
        printf("\nError in malloc: test_first.c\n");
        return 0;
    }
    bytes_read = read(sstore_device, &buf, sizeof (struct readWriteBuffer));
    if (bytes_read < 0)
        perror("read");
    buf.data[9] = '\0';
    printf("\namount read = %d, data read = %s.\n", bytes_read, buf.data);


    printContinuePrompt();


    //delete the blob in the device at index 3
    printf("\nDeleting blob at index 3.");
    ioctl_return = ioctl(sstore_device, SSTORE_IOCTL_DELETE, 3);
    if (ioctl_return < 0)
        perror("ioctl");


    printContinuePrompt();


    //close the device
    printf("\nClosing the device.");
    if (sstore_device == 0)
        close(sstore_device);


    printContinuePrompt();


    return 0;
}



//user must enter 'c' to continue on
void printContinuePrompt() {
    char user_input[2];
    char ch;

    printf("\n\nYou can now check /proc files (from another process).");
    do {
        printf("\n Press 'c' to continue. >");
        fgets(user_input, 2, stdin);
        if (user_input[0] != 'c')
            printf("\nTry again.");
        //clear input buffer
        while ((ch = getchar()) != '\n' && ch != EOF);
    }while (user_input[0] != 'c');
}
