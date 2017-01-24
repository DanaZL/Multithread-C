#include "database.h"

database::database() {
    for (int i = MAX_CNT_KEYS - 1; i >= 0; i--) {
        list_empty_mutex.push_back(i);
        pthread_mutex_init(&mutexes[i], NULL);
    }
    pthread_mutex_init(&mutex_create_delete, NULL);
}

database::~database() {};

int database::set(std::string key, std::string value, int ttl) {

    pthread_mutex_lock(&mutex_create_delete);

    int id_key = hash_fn(key);
    if (key_value_table.find(id_key) == key_value_table.end()) {

        if (cnt_keys == MAX_CNT_KEYS) {

            pthread_mutex_unlock(&mutex_create_delete);
            return ERROR;
        } else {
                value_t* new_value = (value_t*) calloc (1, sizeof(value_t));
                memcpy(new_value->value, value.c_str(), VALUE_SIZE);
                new_value->time_create = clock();
                new_value->ttl = ttl;

                mutex_id.insert(std::pair<int, int>(id_key, *(list_empty_mutex.end() - 1)));                
                list_empty_mutex.pop_back();
                key_value_table.insert(std::pair<int, value_t*>(id_key, new_value));
                cnt_keys++;
                
            pthread_mutex_unlock(&mutex_create_delete);
            return ADDED;
        }

    } else  {
        pthread_mutex_lock(&mutexes[mutex_id[id_key]]);

        value_t* founded = key_value_table[id_key];
        memcpy(founded->value, value.c_str(), VALUE_SIZE);
        founded->time_create = clock();
        founded->ttl = ttl;

        pthread_mutex_unlock(&mutexes[mutex_id[id_key]]);
        pthread_mutex_unlock(&mutex_create_delete);
        return UPDATED;
    }
}

int database::get(std::string key, std::string& return_string) {

    pthread_mutex_lock(&mutex_create_delete);
    int id_key = hash_fn(key);
    if (key_value_table.find(id_key) == key_value_table.end()) {
        pthread_mutex_unlock(&mutex_create_delete);
        return ERROR;
    } else {
        pthread_mutex_lock(&mutexes[mutex_id[id_key]]);
            return_string = key_value_table[id_key]->value;
        pthread_mutex_unlock(&mutexes[mutex_id[id_key]]);
        pthread_mutex_unlock(&mutex_create_delete);
        return FOUND;
    }
}

int database::delete_key(std::string key) {
    pthread_mutex_lock(&mutex_create_delete);
    int id_key = hash_fn(key);

    if (key_value_table.find(id_key) == key_value_table.end()) {

        pthread_mutex_unlock(&mutex_create_delete);
        return ERROR;
    } else {
        pthread_mutex_lock(&mutexes[mutex_id[id_key]]);

            free(key_value_table[id_key]);
            key_value_table.erase(id_key);
            list_empty_mutex.push_back(id_key);
            cnt_keys--;

        pthread_mutex_unlock(&mutexes[mutex_id[id_key]]);

            mutex_id.erase(id_key);

        pthread_mutex_unlock(&mutex_create_delete);
        return DELETED;
    }
}

void database::delete_ttl() {
    pthread_mutex_lock(&mutex_create_delete);

        for (auto p = key_value_table.cbegin(); p != key_value_table.cend(); ) {

            value_t* cur_value = p->second;
            if ((clock() - cur_value->time_create)  / (double)CLOCKS_PER_SEC > cur_value->ttl) {

                int id_key = p->first;
                pthread_mutex_lock(&mutexes[mutex_id[id_key]]);

                    std::cout << "Deleted\n: Value = " << std::string(cur_value->value) << " – ttl passed" << std::endl;
                    free(key_value_table[id_key]);
                    key_value_table.erase(p++);
                    list_empty_mutex.push_back(id_key);
                    cnt_keys--;

                pthread_mutex_unlock(&mutexes[mutex_id[id_key]]);
                    mutex_id.erase(id_key);

            } else {
                ++p;
            }
        }

    pthread_mutex_unlock(&mutex_create_delete);
}
