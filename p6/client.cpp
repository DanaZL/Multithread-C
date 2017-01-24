#include "client.h"

client::client() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (check_error(connect(client, (struct sockaddr *) &addr, sizeof(addr)))) {
        exit(-1);
    }
    set_nonblock(client);

    poll_fd.fd = client;
    poll_fd.events = POLLIN;
}

client::~client() {
    shutdown(client, SHUT_RDWR);
    close(client);
}

void client::receive() {
    while (1) {
        sleep(1);
        poll(&poll_fd, 1, 1);
        if (poll_fd.revents & poll_fd.events) {
                
            int message_len = recv(poll_fd.fd, buf, BUF_SIZE, 0);
            if (message_len <= 0) {
                shutdown(client, SHUT_RDWR);
                close(client);
            }
            
            return;
        }
    }
}

int client::set(std::string key, std::string value, int ttl) {

    buf[0] = SET;
    memcpy(buf + 1, key.c_str(), KEY_SIZE);
    memcpy(buf + 1 + MAX_KEY_SIZE, value.c_str(), MAX_VALUE_SIZE);
    memcpy(buf + 1 + MAX_KEY_SIZE + MAX_VALUE_SIZE, &ttl, TTL_SIZE);

    if (check_error(send(poll_fd.fd, buf, BUF_SIZE, 0))) {
        return -1;
    }   

    receive();

    printf("%s\n", buf + 1);
    fflush(stdout);
    if (buf[0] == ERROR) {
        return -1;
    }
    memset(buf, 0, BUF_SIZE);
    return 0;
}

int client::get(std::string key, std::string return_value) {

    buf[0] = GET;
    memcpy(buf + 1, key.c_str(), KEY_SIZE);

    if (check_error(send(poll_fd.fd, buf, strlen(buf), MSG_NOSIGNAL))) {
        return -1;
    }

    receive();

    printf("%s\n", buf + 1);
    fflush(stdout);
    if (buf[0] == ERROR) {
        return -1;
    }
    memset(buf, 0, BUF_SIZE);
    return 0;
}

int client::delete_key(std::string key) {

    buf[0] = DELETE;
    memcpy(buf + 1, key.c_str(), KEY_SIZE);

    if (check_error(send(poll_fd.fd, buf, strlen(buf), MSG_NOSIGNAL))) {
        return -1;
    }

    receive();

    printf("%s\n", buf + 1);
    fflush(stdout);
    if (buf[0] == ERROR) {
        return -1;
    }
    memset(buf, 0, BUF_SIZE);
    return 0;
}
