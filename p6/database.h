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
#include <sys/epoll.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <set>
#include <list>
#include <map>
#include <vector>

#define MAX_CNT_KEYS  2000
#define SERVER_PORT 9001
#define EPOLL_SIZE 1024
#define MAX_KEY_SIZE 1024
#define MAX_VALUE_SIZE 1024
#define TTL_SIZE 4
#define BUF_SIZE MAX_KEY_SIZE + MAX_VALUE_SIZE + TTL_SIZE

int set_nonblock(int fd);

int check_error(int value, std::string err_mes="");

enum { SET, GET, DELETE };

enum { UPDATED, ADDED, FOUND, DELETED, ERROR };

struct value_t {
    char value[VALUE_SIZE];
    int time_create;
    int ttl;
};

typedef struct value_t value_t;

class database {
private:
    int cnt_keys = 0;
    std::map<int, value_t*> key_value_table;
    std::map<int, int> mutex_id;
    pthread_mutex_t mutexes[MAX_KEY];
    
    std::vector<int> list_empty_mutex;
    pthread_mutex_t mutex_create_delete;
    std::hash<std::string> hash_fn;
public:
    database();
    ~database();

    int set(std::string key, std::string value, int ttl);
    int get(std::string key, std::string& return_value);
    int delete_key(std::string key);
    void delete_ttl();

};
