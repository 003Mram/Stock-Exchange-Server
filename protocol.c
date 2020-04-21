#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "debug.h"
#include "protocol.h"
#include "csapp.h"

char *packet_types[] = {
    "No packet",
    /* Client-to-server*/
    "LOGIN", "STATUS",
    "DEPOSIT", "WITHDRAW",
    "ESCROW", "RELEASE",
    "BUY", "SELL", "CANCEL",
    /* Server-to-client responses (synchronous) */
    "ACK", "NACK",
    /* Server-to_client notifications (asynchronous) */
    "BOUGHT", "SOLD",
    "POSTED", "CANCELED", "TRADED"
};

int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload){

    int write_bytes = 0;
    struct timespec send_time;

    clock_gettime(CLOCK_MONOTONIC, &send_time);

    hdr->timestamp_sec = htonl(send_time.tv_sec);
    hdr->timestamp_nsec = htonl(send_time.tv_nsec);

    write_bytes = rio_writen(fd,hdr,sizeof(BRS_PACKET_HEADER));
    if(write_bytes<sizeof(BRS_PACKET_HEADER)){
        return -1;
    }
    uint16_t size = ntohs(hdr->size);
    if(size>0){
        if((rio_writen(fd,payload,size)) <= 0){
            return -1;
        }
    }

    printf("=>%u.%u, type=%s, size=%hi\n",ntohl(hdr->timestamp_sec),ntohl(hdr->timestamp_nsec), packet_types[hdr->type],ntohs(hdr->size));

    return 0;
}

int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp){

    int read_bytes = 0;
    read_bytes = rio_readn(fd,hdr,sizeof(BRS_PACKET_HEADER));
    if(read_bytes <= 0){
        debug("EOF on fd %d",fd);
        return -1;
    }
    uint16_t size = ntohs(hdr->size);
    if(size>0){
        *payloadp = malloc(sizeof(char) * size);
        read_bytes = rio_readn(fd,*payloadp,size);
        if(read_bytes <= 0){
            return -1;
        }
    }else{
        *payloadp = NULL;
    }

    printf("Receiving:<=%u.%u, type=%s, size=%hi\n",ntohl(hdr->timestamp_sec),ntohl(hdr->timestamp_nsec), packet_types[hdr->type],ntohs(hdr->size));

    return 0;

}