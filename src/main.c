#include "server.h"
#include "ubus_module.h"

#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

static int program_running = 1;

void signal_handler(int sig) 
{
        ubus_module_end_loop();
        program_running = 0;
}

int main()
{
        openlog("OpenVPN manager", LOG_PID, 0);
        syslog(LOG_INFO, "Starting program");

        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGQUIT, signal_handler);

        if (server_connect("127.0.0.1", 7505) != 0) {
                closelog();
                return 1;
        }

        pthread_t ubus_thread;
        if (pthread_create(&ubus_thread, NULL, ubus_module_init_thread, NULL) != 0) {
                syslog(LOG_ERR, "Failed creating ubus thread");
                server_disconnect();
                return 2;
        }

        while (program_running) {
                server_send_request(STATUS_COMMAND, NULL);
                sleep(10);        
        }

        pthread_kill(ubus_thread, SIGTERM);
        pthread_join(ubus_thread, NULL);
        
        server_disconnect();

        syslog(LOG_INFO, "Shutdown program");
        closelog();
        return 0;
}