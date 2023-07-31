#include "ubus_module.h"

#include "client.h"
#include "server.h"

#include <syslog.h>
#include <string.h>
#include <libubus.h>
#include <pthread.h>
#include <libubox/blobmsg_json.h>

int get_client_list();
int disconnect_client();

enum {
	CLIENT_ADDRESS,
	__DISCONNECT_CLIENT_MAX
};

static const struct blobmsg_policy disconnect_client_policy[] = {
	[CLIENT_ADDRESS] = { .name = "address", .type = BLOBMSG_TYPE_STRING }
};

static const struct ubus_method server_methods[] = {
	UBUS_METHOD_NOARG("clients", get_client_list),
	UBUS_METHOD("disconnect_client", disconnect_client, disconnect_client_policy)
};

static struct ubus_object_type server_object_type = UBUS_OBJECT_TYPE("openvpn_server", server_methods);

static struct ubus_object server_object = {
        .name   	= "openvpn_server",
        .type   	= &server_object_type,
	.methods	= server_methods,
	.n_methods 	= ARRAY_SIZE(server_methods)
};

int get_client_list(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		    const char *method, struct blob_attr *msg)
{
	void *array;
	void *table;
	struct blob_buf clients_buf = { 0 };

	blob_buf_init(&clients_buf, 0);

	pthread_mutex_lock(&clients_lock);

	array = blobmsg_open_array(&clients_buf, "clients");
	for (int i = 0; i < server_get_clients_count(); i++) {
		table = blobmsg_open_table(&clients_buf, NULL);

		struct Client *client = server_get_client(i);
		
		blobmsg_add_string(&clients_buf, "common_name", client->common_name);
		blobmsg_add_string(&clients_buf, "address", client->address);
		blobmsg_add_u64(&clients_buf, "bytes_received", client->bytes_received);
		blobmsg_add_u64(&clients_buf, "bytes_sent", client->bytes_sent);
		blobmsg_add_string(&clients_buf, "connected_since", client->connected_since);

		blobmsg_close_table(&clients_buf, table);
	}
	blobmsg_close_array(&clients_buf, array);
	ubus_send_reply(ctx, req, clients_buf.head);
	blob_buf_free(&clients_buf);

	pthread_mutex_unlock(&clients_lock);

	return 0;
}

int disconnect_client(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		      const char *method, struct blob_attr *msg)
{
	int len = 32;
	char address[len];
	struct blob_attr *tb[__DISCONNECT_CLIENT_MAX];

	blobmsg_parse(disconnect_client_policy, __DISCONNECT_CLIENT_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[CLIENT_ADDRESS])
		return UBUS_STATUS_INVALID_ARGUMENT;
	
	strncpy(address, blobmsg_get_string(tb[CLIENT_ADDRESS]), len - 1);
	address[len - 1] = '\0'; 

	server_send_request(DISCONNECT_COMMAND, address);

	return 0;
}

int ubus_module_init_thread()
{
	syslog(LOG_INFO, "Starting ubus thread");

	struct ubus_context *ctx;
	uloop_init();

	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return 1;
	}

	ubus_add_uloop(ctx);
	ubus_add_object(ctx, &server_object);
	uloop_run();

	ubus_free(ctx);
	uloop_done();

	syslog(LOG_INFO, "Exiting ubus thread");

	pthread_exit(NULL);
	return 0;
}

void ubus_module_end_loop()
{
	uloop_end();
}