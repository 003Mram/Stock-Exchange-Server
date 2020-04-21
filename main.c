#include <stdlib.h>
#include <signal.h>

#include "client_registry.h"
#include "exchange.h"
#include "trader.h"
#include "debug.h"
#include "csapp.h"
#include "server.h"

extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;
int *connfdp;

static void terminate(int status);

/*
 * "Bourse" exchange server.
 *
 * Usage: bourse <port>
 */

void sighup_handler(int sig){
    free(connfdp);
    terminate(EXIT_SUCCESS);
}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    int listenfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if(argc != 3){
        fprintf(stderr, "usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = Open_listenfd(argv[2]);

    // Perform required initializations of the client_registry,
    // maze, and player modules.
    client_registry = creg_init();
    exchange = exchange_init();
    trader_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function brs_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    // fprintf(stderr, "You have to finish implementing main() "
	   //  "before the Bourse server will function.\n");
    struct sigaction action, old_action;
    action.sa_handler = sighup_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    sigaction(SIGHUP, &action, &old_action);

    debug("Bourse server listening on port %s",argv[2]);
    while(1){
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        pthread_create(&tid, NULL, brs_client_service, connfdp);
    }

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}
