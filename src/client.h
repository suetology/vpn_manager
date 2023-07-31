#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>

#define BUFFER_SIZE 32

struct Client {
        size_t bytes_received;
        size_t bytes_sent;
        char common_name[BUFFER_SIZE];
        char address[BUFFER_SIZE];
        char connected_since[BUFFER_SIZE];
};

struct Client *client_load(size_t bytes_received, 
                           size_t bytes_sent,
                           const char *common_name, 
                           const char *address, 
                           const char *connected_since);
int client_delete(struct Client **client);

#endif