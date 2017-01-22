#include <stdio.h>
#include <iostream>
#include <string.h>
#include <ev.h>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "buffer.h"
#include "sockets_create.h"

#define BUFFER_SIZE 1024
#define IP_SIZE 32
#define MAX_PORTS 10
#define BUFFER_SIZE 1024

char localhost[] = "127.0.0.1";

std::map<int, std::vector<std::pair<char*, int>>> config_map;
std::map<int, buffer*> sender_buffer;
std::map<int, int> senders_map;


void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    
    int fd = senders_map[watcher->fd];
    buffer *buf = sender_buffer[fd];

    if (buf == NULL) {
        std::cout << strerror(errno) << std::endl;
    }

    int wrote = buffer_write(buf, BUFFER_SIZE);

    if (wrote == -1 || (buf->data_size == 0)) {
        ev_io_stop(loop, watcher);
        senders_map.erase(watcher->fd);
        free(watcher);

        if (wrote == -1 || buf->closed) {
            buffer_destructor(buf);
            sender_buffer[fd] = NULL;
            sender_buffer.erase(fd);
        }
    }
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    buffer *buf = sender_buffer[watcher->fd];
    if (buf == NULL) {
        ev_io_stop(loop, watcher);
        free(watcher);
        sender_buffer.erase(watcher->fd);
        return;
    }

    int was_empty = 0;
    if (buf->data_size != 0) {
        was_empty = 1;
    }

    if (buf->buffer_size == buf->data_size)
        return;

    int readed = buffer_read(buf, BUFFER_SIZE);
    if (readed <= 0) {
        ev_io_stop(loop, watcher);
        free(watcher);
        buf->closed = 1;
        if (buf->data_size == 0) {
            buffer_destructor(buf);
            sender_buffer[watcher->fd] = NULL;
            sender_buffer.erase(watcher->fd);
        }
        return;
    }

    int buffer_id = buf->reciever_fd;

    senders_map[buffer_id] = watcher->fd;
    if (was_empty) {
        struct ev_io *write_watcher = (struct ev_io *) calloc(1, sizeof(*write_watcher));
        ev_io_init(write_watcher, write_cb, buffer_id, EV_WRITE);
        ev_io_start(loop, write_watcher);
    }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
    std::cout << "Accept \n";

    //client - proxy - server
    int client_socket = create_client_socket(watcher->fd);
    check_error(client_socket);

    int rand_number = rand() % config_map[watcher->fd].size();
    char* ip = config_map[watcher->fd][rand_number].first;
    int port = config_map[watcher->fd][rand_number].second;

    //server - proxy - client
    int server_socket = create_server_socket(ip, port);
    if (soft_check_error(server_socket)) {
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        return;
    }

    buffer *buf_serv, *buf_client;

    buf_client = buffer_init(BUFFER_SIZE, client_socket, server_socket);
    buf_serv = buffer_init(BUFFER_SIZE, server_socket, client_socket);
    sender_buffer[client_socket] = buf_serv;
    sender_buffer[server_socket] = buf_client;

    struct ev_io *client_watcher = (struct ev_io *) calloc(1, sizeof(*client_watcher));
    ev_io_init(client_watcher, read_cb, client_socket, EV_READ);
    ev_io_start(loop, client_watcher);

    struct ev_io *server_watcher = (struct ev_io *) calloc(1, sizeof(*server_watcher));
    ev_io_init(server_watcher, read_cb, server_socket, EV_READ);
    ev_io_start(loop, server_watcher);

}

std::vector<int> read_config(const char* config_name,
                             std::map<int, std::vector<std::pair<char*, int>>> &config_map) {
    
    FILE *config = fopen(config_name, "r");

    if (not(config)) {
        std::cout << strerror(errno) << std::endl;
    }

    char buf_config[BUFFER_SIZE];
    int port;

    std::vector<int> proxy_sockets;

    while(fgets(buf_config, sizeof(buf_config), config) != NULL) {
        int offset = 0;
        sscanf(buf_config, "%d %n", &port, &offset);
        offset += 1;

        int proxy_socket = create_proxy_socket(localhost, port);
        config_map[proxy_socket] = std::vector<std::pair<char*, int>>();

        int new_offset;
        char ip[IP_SIZE];
        while (sscanf(buf_config + offset, "%s : %d %n", ip, &port, &new_offset) == 2) {
            offset += new_offset + 1;
            config_map[proxy_socket].push_back(std::make_pair(ip, port));
        }
        proxy_sockets.push_back(proxy_socket);
    }

    std::cout << "Configuration file readed" <<std::endl; 
    fclose(config);
    return proxy_sockets;
}

int main(int argc, char ** argv)
{
    if (argc < 2) {
        std::cout << "Need name of configuration file\n";
        return -1;
    }
    std::string config_file = argv[1];

    struct ev_loop *loop = ev_default_loop(0);
    ev_io accept_watchers[MAX_PORTS];
    int current_watcher = 0;
    std::vector<int> master_sockets = read_config(config_file.c_str(), config_map);

    for (auto it = master_sockets.begin();it != master_sockets.end(); it++) {
        ev_io_init(&accept_watchers[current_watcher], accept_cb, *it, EV_READ);
        ev_io_start(loop, &accept_watchers[current_watcher]);
        current_watcher++;
    }
    ev_run(loop);
    return 0;
}
