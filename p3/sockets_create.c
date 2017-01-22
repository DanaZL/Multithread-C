#include "sockets_create.h"

int set_nonblock(int fd) {
    int flags;
#if defined(O_NONBLOCK)
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int check_error(int value) {
    if (value < 0) {
        std::cout << strerror(errno) << std::endl;
        exit(1);
    }
    return 0;
}

int soft_check_error(int value) {
    if (value < 0) {
        std::cout << strerror(errno) << std::endl;
        return 1;
    }
    return 0;
}

int create_proxy_socket(char *ip, int port) {
    int proxy_socket = socket(AF_INET, SOCK_STREAM, 0);

    check_error(proxy_socket);
    set_nonblock(proxy_socket);

    struct sockaddr_in sock_adr;
    memset(&sock_adr, 0, sizeof(sock_adr));
    sock_adr.sin_family = AF_INET;
    sock_adr.sin_port = htons(port);
    inet_aton(ip, &sock_adr.sin_addr);
    
    int optval = 1;
    check_error(setsockopt(proxy_socket, SOL_SOCKET, SO_REUSEADDR, 
                &optval, sizeof(optval)));

    check_error(bind(proxy_socket, (struct sockaddr *) &sock_adr,
                sizeof(sock_adr)));
    
    check_error(listen(proxy_socket, 0));

    return proxy_socket;
}


int create_server_socket(char *ip, int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    check_error(server_socket);

    struct sockaddr_in sock_adr;
    memset(&sock_adr, 0, sizeof(sock_adr));
    sock_adr.sin_family = AF_INET;
    sock_adr.sin_port = htons(port);
    inet_aton(ip, &sock_adr.sin_addr);

    if (soft_check_error(connect(server_socket, (struct sockaddr*)&sock_adr, 
                                sizeof(sock_adr)))) {
        close(server_socket);
        return -1;
    }

    return server_socket;
}


int create_client_socket(int proxy_socket) {
    int client_socket = accept(proxy_socket, 0, 0);

    check_error(client_socket);
    set_nonblock(client_socket);

    return client_socket;
}
