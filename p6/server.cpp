#include "server.h"

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

int check_error(int value, std::string err_mes) {
    if (value < 0) {
        std::string message = err_mes.length() == 0 ? strerror(errno) : err_mes;
        std::cout << message << std::endl;
        return 1;
    }
    return 0;
}

server::server() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (check_error(server)) {
        exit(-1);
    }
    set_nonblock(server);

    int optval = 1;
    setsockopt(server, 6, SO_REUSEADDR, &optval, sizeof(optval));
    if (check_error(bind(server, (struct sockaddr *) &addr, sizeof(addr)))) {
        exit(-1);
    }

    if (check_error(listen(server, EPOLL_SIZE))) {
        exit(-1);
    }

    epoll_fd = epoll_create1(0);

    if (check_error(epoll_fd)) {
        exit(-1);
    }

    struct epoll_event ev;
    ev.data.fd = server;
    ev.events = EPOLLIN;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server, &ev);
    events = (struct epoll_event*) calloc(EPOLL_SIZE, sizeof(*events));

    struct Args args;
    args.this_class = this;
    pthread_t ttl_thread;
    pthread_create(&ttl_thread, NULL, call_ttl_run, (void*) &args);

     while (1) {
        int events_cnt = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
        for (int i = 0; i < events_cnt; i++) {
            if(events[i].data.fd == server) {
                    int client = accept(server, 0, 0);
                    if (check_error(client)) {
                        exit(-1);
                    }
                    set_nonblock(client);

                    struct epoll_event ev;
                    ev.data.fd = client;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &ev);
                    clients.insert(client);
            } else {
                memset(buf, 0, BUF_SIZE);
                int mes_len = recv(events[i].data.fd, buf, BUF_SIZE, MSG_NOSIGNAL);
                if (mes_len <= 0) {
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);
                    clients.erase(events[i].data.fd);
                    continue;
                } 

                struct Args* args = (struct Args*) calloc (1, sizeof(struct Args));
                memcpy(args->key, buf + 1, KEY_SIZE);
                memcpy(args->value, buf + 1 + KEY_SIZE, VALUE_SIZE);
                sscanf(buf + 1 + KEY_SIZE + VALUE_SIZE, "%d", &args->time_to_live);
                args->client = events[i].data.fd;
                args->this_class = this;

                pthread_t thread;
                if (buf[0] == SET) {
                    pthread_create(&thread, NULL, call_set, (void*) args);
                } else if (buf[0] == GET) {
                    pthread_create(&thread, NULL, call_get, (void*) args);
                } else if (buf[0] == DELETE) {
                    pthread_create(&thread, NULL, call_delete_key, (void*) args);
                }
            }
        }
    }
}

void* server::set(void* arg) {
    struct Args* args = (struct Args*) arg;
    std::string key = args->key;
    std::string value = args->value;
    int time_to_live = args->time_to_live;
    int client = args->client;

    int result = key_value.set(key, value, time_to_live);
    std::string mes;

    char buf[KEY_SIZE + VALUE_SIZE + 1];
    if (result == ERROR) {
        buf[0] = result;
        mes = "Don't have free memory";
    } else if (result == UPDATED || result == ADDED) {
        buf[0] = result;
        if (result == UPDATED) {
            mes = "Updated: ";
        } else {
            mes = "Added: ";
        }
        mes += "key = ";
        mes += key;
        mes += ", value = ";
        mes += value;
    }
    memcpy(buf + 1, mes.c_str(), BUF_SIZE);
    send(client, buf, BUF_SIZE, MSG_NOSIGNAL);
    free(args);
    return NULL;  
}

void* server::get(void* arg) {
    struct Args* args = (struct Args*) arg;
    std::string key = args->key;
    int client = args->client;

    std::string value;
    int result = key_value.get(key, value);
    std::string mes;

    char buf[KEY_SIZE + VALUE_SIZE + 1];
    if (result == ERROR) {
        buf[0] = result;
        message = "Key not found";
    } else if (result == FOUND) {
        buf[0] = result;
        message = "Find:\n";
        message += "Key = ";
        message += key;
        message += ", Value = ";
        message += value;
    }
    memcpy(buf + 1, message.c_str(), BUF_SIZE);
    send(client, buf, BUF_SIZE, MSG_NOSIGNAL);
    return NULL;  
}

void* server::delete_key(void* arg) {
    struct Args* args = (struct Args*) arg;
    std::string key = args->key;
    int client = args->client;

    int result = key_value.delete_key(key);
    std::string mes;

    char buf[KEY_SIZE + VALUE_SIZE + 1];
    if (result == ERROR) {
        buf[0] = result;
        message = "Can't delete key: ";
        message += key;
        message += " - key not find";
    } else if (result == DELETED) {
        buf[0] = result;
        message = "Deleted:\n";
        message += "key = ";
        message += key;
    }
    memcpy(buf + 1, message.c_str(), BUF_SIZE);
    send(client, buf, BUF_SIZE, MSG_NOSIGNAL);
    return NULL;  
}

void* server::ttl_run(void* arg) {
    while (1) {
        sleep(6);
        key_value.delete_ttl();
    }
    return NULL;
}

server::~server() {
    shutdown(server, SHUT_RDWR);
    if (check_error(close(server))) {
        exit(-1);
    }
}

void* call_set(void* arg) {
    struct Args*  args = (struct Args*) arg;
    (Server*)(args->this_class)->set(arg);
    return NULL;
}

void* call_get(void* arg) {
    struct Args*  args = (struct Args*) arg;
    (Server*)(args->this_class)->get(arg);
    return NULL;
}

void* call_delete_key(void* arg) {
    struct Args*  args = (struct Args*) arg;
    (Server*)(args->this_class)->delete_key(arg);
    return NULL;
}

void* call_ttl_run(void* arg) {
    struct Args*  args = (struct Args*) arg;
    (Server*)(args->this_class)->ttl_run(arg);
    return NULL;
}
