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
    sstore_device = open("/dev/sstore0", "r");
    //test read

    //test write
    buf.index = 5;
    buf.size = 10;
    buf.data = "Hello world";

    //test close
    close(sstore_device);

    return 0;
}
