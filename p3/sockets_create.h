#include <iostream>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <map>
#include <vector>

int set_nonblock(int fd);

int check_error(int value);

int soft_check_error(int value);

int create_proxy_socket(char *ip, int port);

int create_client_socket(int master_socket);

int create_server_socket(char *ip, int port);
