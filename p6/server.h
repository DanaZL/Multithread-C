#include "database.h"

int check_error(int value, std::string err_mes="");

class server {
    database key_value;
    char buf[MAX_KEY_SIZE + MAX_VALUE_SIZE + 1];

    int server;
    std::set <int> clients;

    int epoll_fd;
    struct epoll_event *events;
public:
    server();
    ~server();

    void* set(void* arg);
    void* get(void* arg);
    void* delete_key(void* arg);
    void* ttl_run(void* arg);
};

struct Args {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
    int time_to_live;
    int client;
    server* this_class;
};

void* call_set(void*);
void* call_get(void*);
void* call_delete_key(void*);
void* call_ttl_run(void* arg);

