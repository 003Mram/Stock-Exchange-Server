#include <stdlib.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "client_registry.h"
#include "debug.h"

typedef struct client_registry{
    int* fds_array;
    int n;
    int connected;
    int first;
    int rear;
    sem_t slots;
    sem_t mutex;
    sem_t empty;
    sem_t items;
}CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init(){
    debug("Initialize client registry");
    CLIENT_REGISTRY *creg = malloc(sizeof(CLIENT_REGISTRY));
    creg->n = FD_SETSIZE;
    creg->connected = 0;
    creg->fds_array = malloc(sizeof(int) * creg->n);
    int *x = creg->fds_array;
    for(int i=0;i<creg->n;i++){
        *(x+i) = -1;
    }
    sem_init(&creg->mutex,0,1);
    sem_init(&creg->slots,0,creg->n);
    sem_init(&creg->items,0,0);
    sem_init(&creg->empty,0,1);
    return creg;
}

void creg_fini(CLIENT_REGISTRY *cr){
    free(cr->fds_array);
    free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd){
    sem_wait(&cr->slots);
    sem_wait(&cr->mutex);
    // int i=0;
    // while(i<(cr->n) && cr->fds_array[i] != -1){
    //     i++;
    // }

    // if(i!= cr->n){
    //     cr->fds_array[i] = fd;
    // }

    cr->fds_array[fd] = fd;
    cr->connected++;
    if(cr->connected == 1){
        sem_wait(&cr->empty);
    }
    debug("Total connected: %d",cr->connected);
    sem_post(&cr->mutex);
    sem_post(&cr->items);
    return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd){
    sem_wait(&cr->items);
    sem_wait(&cr->mutex);
    // int i=0;
    // int output=0;
    // while(i<(cr->n) && cr->fds_array[i] != fd){
    //     i++;
    // }

    // if (i == cr->n)
    // {
    //     output = -1;/* code */
    // }else{
    //     cr->fds_array[i] = -1;
    //     cr->connected--;
    // }
    int output=0;
    if(cr->fds_array[fd] != -1){
        cr->fds_array[fd] = -1;
        cr->connected--;
    }else{
        output = -1;
    }
    debug("Total connected: %d",cr->connected);

    sem_post(&cr->mutex);
    sem_post(&cr->slots);
    if(cr->connected == 0){
        sem_post(&cr->empty);
    }
    return output;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    debug("Client Registry empty");
    sem_wait(&cr->empty);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
    sem_wait(&cr->mutex);
    for(int i=0;i<cr->n;i++){
        if(cr->fds_array[i] != -1){
            debug("Shutting down client %d",cr->fds_array[i]);
            shutdown(cr->fds_array[i],SHUT_RD);
        }
    }
    sem_post(&cr->mutex);
}