#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>

#define STATUS_COMMAND "status"
#define DISCONNECT_COMMAND "kill"

pthread_mutex_t clients_lock;

int server_connect(const char *ip, int port);
int server_disconnect();
int server_send_request(const char *command, const char *arguments);
int server_get_clients_count();
struct Client *server_get_client(int index);

#endif