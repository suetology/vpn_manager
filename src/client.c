#include "client.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

struct Client *client_load(size_t bytes_received, 
                           size_t bytes_sent,
                           const char *common_name, 
                           const char *address, 
                           const char *connected_since)
{
        struct Client *client = (struct Client *)malloc(sizeof(struct Client));
        if (client == NULL) {
                syslog(LOG_ERR, "Failed allocating memory for client info");
                return NULL;
        }

        strncpy(client->common_name, common_name, BUFFER_SIZE - 1);
        client->common_name[BUFFER_SIZE - 1] = '\0';
        strncpy(client->address, address, BUFFER_SIZE - 1);
        client->address[BUFFER_SIZE - 1] = '\0';
        strncpy(client->connected_since, connected_since, BUFFER_SIZE - 1);
        client->connected_since[BUFFER_SIZE - 1] = '\0';
        
        client->bytes_received = bytes_received;
        client->bytes_sent = bytes_sent;

        return client;
}

int client_delete(struct Client **client)
{
        if (client == NULL)
                return 1;
        if (*client == NULL)
                return 2;
        
        free(*client);
        *client = NULL;

        return 0;
}

