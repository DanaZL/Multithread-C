#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <list>
#include <time.h>
#include <netinet/in.h>
#include <errno.h>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <map>

#define BUF_SIZE 1024
#define SERVER_PORT 3100
#define TCP 6
#define EPOLL_SIZE 1024

const char welcome_msg[] = "Welcome to Hell!\n";
const char accept_log[] = "accepted connection\n";
const char terminate_log[] = "connection terminated\n";


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

void check_error (int value) {
   if (value < 0) {
        std::cout << strerror(errno) << std::endl;
        exit(1);   
   }
}

int main(int argc, char * argv[])
{
    // setvbuf(stdout, NULL, _IOLBF, 0);
    char msg_buf[1024];

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    int listener;
    listener = socket(AF_INET, SOCK_STREAM, 0); 
    check_error(listener);
    set_nonblock(listener);

    int optval = 1;
    setsockopt(listener, TCP, SO_REUSEADDR, &optval, sizeof(optval));

    check_error(bind(listener, (struct sockaddr *) &addr, sizeof(addr)));

    check_error(listen(listener, EPOLL_SIZE));

    int epoll_fd = epoll_create1(0);
    check_error(epoll_fd);

    struct epoll_event ev;
    ev.data.fd = listener;
    ev.events = EPOLLIN;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &ev);
    
    std::set <int> clients;
    struct epoll_event *events;
    // int printed_begin_msg = 0;
    events = (struct epoll_event*) calloc(EPOLL_SIZE, sizeof(*events));

    while (1) {

        int events_cnt = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);

        for (int i = 0; i < events_cnt; ++i) {
            if(events[i].data.fd == listener) {
                int client = accept(listener, 0, 0);
                // std::cout <<"\t\tCLIENT  " << client<< std::endl;
                check_error(client);
                set_nonblock(client);

                fprintf(stdout, accept_log);
                fflush(stdout);

                struct epoll_event ev;
                ev.data.fd = client;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);

                clients.insert(client);
                // clients[client] = 0;
                send(client, welcome_msg, sizeof(welcome_msg), MSG_NOSIGNAL);

            } else {
                memset(msg_buf, 0, sizeof(msg_buf));
                int received = recv(events[i].data.fd, msg_buf, sizeof(msg_buf), MSG_NOSIGNAL);
                
                if (received <= 0) {

                    // if (printed_begin_msg == 1) {
                    //     fprintf(stdout, "\n");
                    //     printed_begin_msg = 0;
                    // }

                    fprintf(stdout, terminate_log);
                    fflush(stdout);

                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);
                    clients.erase(events[i].data.fd);
                   
                    continue;
                }

                // if (msg_buf[received - 1] != '\n') {
                //     printed_begin_msg = 1;
                // }

                msg_buf[received] = 0;
                fprintf(stdout, "%s", msg_buf);
                fflush(stdout);

                for (auto p = clients.begin(); p != clients.end(); p++) {
                    send(*p, msg_buf, received, MSG_NOSIGNAL);
                }
            }
        }
    }
    shutdown(listener, SHUT_RDWR);
    check_error(close(listener));
    return 0;
}