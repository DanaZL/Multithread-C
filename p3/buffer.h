#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

struct buffer
{
    char *buf, *prepare_buf;
    int buffer_size, data_size;
    int reciever_fd, sender_fd;
    int begin;
    int closed;
};

buffer* buffer_init(int size_param, int recv_fd, int send_fd);

void buffer_destructor(buffer *recv);

int buffer_read(buffer *recv, int size);

int buffer_write(buffer *recv, int size);