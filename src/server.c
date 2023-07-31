#include "server.h"

#include "client.h"

#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 16
#define SIZE 4096

static char buffer[SIZE];
static int sockfd = -1;
static struct Client *clients[MAX_CLIENTS];
static size_t clients_count;

static struct Client *parse_client(char *client_info);
static int parse_clients_list();
static int delete_clients();
static int collect_garbage();

int server_connect(const char *ip, int port)
{
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                syslog(LOG_ERR, "Error creating socket");
                return 1;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                syslog(LOG_ERR, "Error connecting to the server");
                server_disconnect();
                return 2;
        }

        collect_garbage();

        return 0;
}

int server_disconnect()
{
        delete_clients();

        if (close(sockfd) != 0) {
                syslog(LOG_ERR, "Failed closing socket");
                return 1;
        }
        return 0;
}

int server_send_request(const char *command, const char *arguments)
{
        pthread_mutex_lock(&clients_lock);

        collect_garbage();

        char request[256] = "";
        strcat(request, command);
        if (arguments != NULL) {
                strcat(request, " ");
                strcat(request, arguments);
        }
        strcat(request, "\n");

        if (sockfd < 0) {
                syslog(LOG_ERR, "Trying to send request with uninitialized socket");
                return 1;
        }

        if (send(sockfd, request, strlen(request), 0) != strlen(request)) {
                syslog(LOG_ERR, "Failed sending %s request to the server", request);
                return 2;
        }

        ssize_t bytes_read;
        if ((bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) <= 0) {
                syslog(LOG_ERR, "Failed retrieving response from the server");
                return 3;
        } 
        buffer[bytes_read] = '\0';

        if (strcmp(command, DISCONNECT_COMMAND) == 0) {
                pthread_mutex_unlock(&clients_lock);
                return server_send_request(STATUS_COMMAND, NULL);
        }

        if (strcmp(command, STATUS_COMMAND) == 0) {
                delete_clients();
                parse_clients_list();
        }
        pthread_mutex_unlock(&clients_lock);
        return 0;
}

int server_get_clients_count()
{
        return clients_count;
}

struct Client *server_get_client(int index)
{
        if (index < 0 || index >= clients_count) {
                return NULL;
        }
        return clients[index];
}

static struct Client *parse_client(char *client_info)
{

        char *common_name = client_info;
        char *end = strchr(common_name, ',');
        *end = '\0';
        char *address = end + 1;

        end = strchr(address, ',');
        *end = '\0';
        char *bytes_received_str = end + 1;

        end = strchr(bytes_received_str, ',');
        *end = '\0';
        char *bytes_sent_str = end + 1;

        end = strchr(bytes_sent_str, ',');
        *end = '\0';
        char *connected_since = end + 1;

        if (common_name == NULL 
         || address == NULL 
         || connected_since == NULL 
         || bytes_received_str == NULL
         || bytes_sent_str == NULL) {
                syslog(LOG_ERR, "Failed parsing client info");
                return NULL;
        }

        size_t bytes_received = atoi(bytes_received_str);
        size_t bytes_sent = atoi(bytes_sent_str);

        return client_load(bytes_received, bytes_sent, common_name, address, connected_since);
}

static int parse_clients_list()
{
        const char *delimiter = "\r\n";

        char *line = strtok(buffer, delimiter);
        if (line == NULL || strcmp(line, "OpenVPN CLIENT LIST") != 0) {
                syslog(LOG_ERR, "Failed parsing status info");
                return 1;
        }
        line = strtok(NULL, delimiter);
        line = strtok(NULL, delimiter);

        while ((line = strtok(NULL, delimiter)) != NULL && strcmp(line, "ROUTING TABLE") != 0) {
                if (clients_count >= MAX_CLIENTS)
                        break;

                char *copy = strdup(line);
                struct Client *client = parse_client(copy);
                free(copy);     

                clients[clients_count] = client;
                clients_count++;
        }

        return 0;
}

static int delete_clients()
{
        int rc = 0;
        for (int i = 0; i < clients_count; i++)
                rc = client_delete(&clients[i]);

        clients_count = 0;

        return rc;
}

static int collect_garbage()
{
        int bytes;
        int bytes_collected = 0;
        while ((bytes = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT)) > 0)
                bytes_collected += bytes;

        return bytes_collected;
}