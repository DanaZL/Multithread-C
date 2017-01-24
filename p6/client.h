#include "database.h"

class client {
    int client;
    struct pollfd poll_fd;
    char buf[BUF_SIZE];  
    void receive();

public:
    client();
    ~client();

    int set(std::string key, std::string value, int ttl);
    int get(std::string key, std::string return_value);
    int delete_key(std::string key);
};
