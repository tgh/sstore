#include <stdio.h>

struct readWriteBuffer {
    int index;
    int size;
    char * data;
}

int main ()
{
    char buffer_container[1024];
    struct readWriteBuffer buf;

    buf.index = 5;
    buf.size = 10;
    buf->data = "Hello world";

    

    return 0;
}
