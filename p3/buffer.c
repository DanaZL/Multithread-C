#include "buffer.h"

// struct buffer
// {
//     char *buf, *prepare_buf;
//     int buffer_size, data_size;
//     int reciever_fd, sender_fd;
//     int begin;
//     int closed;
// };
int min(int a, int b)
{
    return a < b ? a : b;
}

buffer* buffer_init(int size_param, 
                    int reciever_fd, int sender_fd) 
{

    buffer *new_buffer = (buffer*) calloc(1, sizeof(*new_buffer));
    new_buffer->buf = (char*) calloc(size_param, sizeof(char));
    new_buffer->prepare_buf = (char*) calloc(size_param, sizeof(char));

    
    new_buffer->buffer_size = size_param;
    new_buffer->data_size = 0;

    new_buffer->reciever_fd = reciever_fd;
    new_buffer->sender_fd = sender_fd;

    new_buffer->begin = 0;
    new_buffer->closed = 0;

    return new_buffer;
}

void buffer_destructor(buffer *for_del) 
{
    free(for_del->buf);
    free(for_del->prepare_buf);
    free(for_del);
}

//recieve message from sender_fd to buffer
int buffer_read(buffer *buf, int size) 
{
    int free = min(size, buf->buffer_size - buf->data_size);

    if (free == 0) {
        return 0;
    }

    int readed = recv(buf->sender_fd, buf->prepare_buf, free, MSG_NOSIGNAL);
    
    if (readed <= 0) {
        return readed;
    }

    int start = (buf->begin + buf->data_size) % buf->buffer_size;

    memcpy(&buf->buf[start], &buf->prepare_buf[0],
            min(readed, buf->buffer_size - start));

    if (min(readed, buf->buffer_size - start) != readed) {
        memcpy(&buf->buf[0], &buf->prepare_buf[buf->buffer_size - start],
                readed - (buf->buffer_size - start));
    }

    buf->data_size += readed;
    return readed;
}


//send message from buffer to reciever_fd
int buffer_write(buffer *buf, int size) 
{
    int tmp = min(size, buf->data_size);
    if (tmp == 0)
        return 0;

    memcpy(&buf->prepare_buf[0], &buf->buf[buf->begin],
            min(tmp, buf->buffer_size - buf->begin));

    if (min(tmp, buf->buffer_size - buf->begin) != tmp) {
        memcpy(&buf->prepare_buf[buf->buffer_size - buf->begin],
                &buf->buf[0],
                tmp - (buf->buffer_size - buf->begin));
    }

    int wrote = send(buf->reciever_fd, buf->prepare_buf, tmp, MSG_NOSIGNAL);
    if (wrote <= 0)
        return wrote;
    buf->begin = (buf->begin + wrote) % buf->buffer_size;
    buf->data_size -= wrote;
    return wrote;
}
