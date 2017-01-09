#include <stdio.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define BUF_SIZE 1024
#define SERVER_PORT 3100
#define TCP 6
#define TIMEOUT 1

void check_error (int value) {
   if (value < 0) {
        std::cout << strerror(errno) << std::endl;
        exit(1);   
   }
}

int set_nonblock(int fd) {                                                                     
    int flags;                                                        
#if defined(O_NONBLOCK)                                               
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))                        
        flags = 0;                                                    
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);                    
#else                                                                 
    flags = 1;                                                        
    return ioctl(fd, FIOBIO, &flags);                                 
#endif                                                                
} 

int main(int argc, char **argv)
{
    char msg_buf[BUF_SIZE];

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    int client;
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    int optval = 1;
    setsockopt(client, TCP, SO_REUSEADDR, &optval, sizeof(optval));
    
    check_error(connect(client, (struct sockaddr *) &addr, sizeof(addr)));  
 
    struct pollfd poll_fd;
    poll_fd.fd = client;
    poll_fd.events = POLLIN;
    
    while(1) {
        poll(&poll_fd, 1, TIMEOUT);
        
        if (poll_fd.revents & POLLIN) {
            int readed = 0;
            readed = recv(poll_fd.fd, msg_buf, sizeof(msg_buf), MSG_NOSIGNAL);
            
            if (readed <= 0) {
                shutdown(client, SHUT_RDWR);
                close(client);
            }

            msg_buf[readed] = 0;
            printf("%s", msg_buf);
            fflush(stdout);
        }

        if (fgets(msg_buf, sizeof(msg_buf), stdin)) {
            check_error(send(poll_fd.fd, msg_buf, strlen(msg_buf), MSG_NOSIGNAL));
        }
        
    }
    shutdown(client, SHUT_RDWR);
    close(client);
    return 0;
}