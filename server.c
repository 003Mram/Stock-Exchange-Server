#include <pthread.h>
#include <sys/select.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "server.h"
#include "client_registry.h"
#include "protocol.h"
#include "trader.h"
#include "debug.h"
#include "helper_functions.h"


void *brs_client_service(void *arg){
    int fd = *((int *)arg);
    free(arg);
    pthread_detach(pthread_self());

    fd_set read_set, ready_set;
    void *payload;
    TRADER *trader = NULL;
    BRS_PACKET_HEADER* hdr = malloc(sizeof(BRS_PACKET_HEADER));
    BRS_STATUS_INFO info;
    memset(&info,0,sizeof(BRS_STATUS_INFO));
    debug("[%d] Starting client service",fd);
    creg_register(client_registry,fd);

    FD_ZERO(&read_set); /* Clear read set */
    FD_SET(fd, &read_set); /* Add stdin to read set */
    while(1){
        ready_set = read_set;
        select(fd+1,&ready_set,NULL,NULL,NULL);
        if(proto_recv_packet(fd, hdr, &payload) < 0){
            free(hdr);
            if(trader != NULL){
                debug("[%d] Logging out trader", fd);
                trader_logout(trader);
            }
            debug("[%d] Ending client service", fd);
            creg_unregister(client_registry,fd);
            close(fd);
            return NULL;
        }else if(trader==NULL && hdr->type != BRS_LOGIN_PKT){
            // hdr->type = BRS_NACK_PKT;
            // hdr->size = htons(0);
            // proto_send_packet(fd,hdr,NULL);
            debug("Login Required");
        }else{
            switch(hdr->type){
                case BRS_LOGIN_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    if(trader == NULL){
                        char *avatar = malloc(sizeof(char) * (ntohs(hdr->size)+1));
                        memcpy(avatar,payload,ntohs(hdr->size));
                        *(avatar + ntohs(hdr->size)) = '\0';
                        trader = trader_login(fd, avatar);
                        if(trader==NULL){
                            hdr->type = BRS_NACK_PKT;
                            hdr->size = htons(0);
                            proto_send_packet(fd,hdr,NULL);
                        }else{
                            hdr->type = BRS_ACK_PKT;
                            hdr->size = htons(0);
                            trader_send_ack(trader, NULL);
                        }
                    }else{
                        debug("[%d] Already logged in (trader %p)",fd,trader);
                        trader_send_nack(trader);
                    }
                    free(payload);
                    break;
                }

                case BRS_STATUS_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    exchange_get_status(exchange, &info);
                    trader_send_ack(trader, &info);
                    break;
                }

                case BRS_DEPOSIT_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    BRS_FUNDS_INFO* funds = (BRS_FUNDS_INFO *) payload;
                    uint32_t amount = ntohl(funds->amount);
                    trader_increase_balance(trader, amount);
                    exchange_get_status(exchange, &info);
                    trader_send_ack(trader, &info);
                    free(funds);
                    break;
                }

                case BRS_WITHDRAW_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    BRS_FUNDS_INFO* funds = (BRS_FUNDS_INFO *) payload;
                    funds_t amount = ntohl(funds->amount);
                    if(trader_decrease_balance(trader, amount) == 0){
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader, &info);
                    }else{
                        trader_send_nack(trader);
                    }

                    free(funds);
                    break;
                }

                case BRS_ESCROW_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    BRS_ESCROW_INFO* inventory = (BRS_ESCROW_INFO *) payload;
                    quantity_t quantity = ntohl(inventory->quantity);
                    trader_increase_inventory(trader, quantity);
                    exchange_get_status(exchange, &info);
                    trader_send_ack(trader, &info);
                    free(inventory);
                    break;
                }

                case BRS_RELEASE_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    BRS_ESCROW_INFO* inventory = (BRS_ESCROW_INFO *) payload;
                    quantity_t quantity = ntohl(inventory->quantity);
                    if(trader_decrease_inventory(trader, quantity) == 0){
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader, &info);
                    }else{
                        trader_send_nack(trader);
                    }
                    free(inventory);
                    break;
                }

                case BRS_BUY_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    orderid_t orderid;
                    BRS_ORDER_INFO* order = (BRS_ORDER_INFO *)payload;
                    funds_t price = ntohl(order->price);
                    quantity_t quantity = ntohl(order->quantity);
                    if((orderid = exchange_post_buy(exchange, trader, quantity, price))>0){
                        info.orderid = htonl(orderid);
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader, &info);
                    }else{
                        trader_send_nack(trader);
                    }
                    free(order);
                    break;
                }

                case BRS_SELL_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    orderid_t orderid;
                    BRS_ORDER_INFO* order = (BRS_ORDER_INFO *)payload;
                    funds_t price = ntohl(order->price);
                    quantity_t quantity = ntohl(order->quantity);
                    if((orderid = exchange_post_sell(exchange, trader, quantity, price))>0){
                        info.orderid = htonl(orderid);
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader, &info);
                    }else{
                        trader_send_nack(trader);
                    }
                    free(order);
                    break;
                }

                case BRS_CANCEL_PKT:{
                    debug("[%d] %s packet received",fd,packet_types[hdr->type]);
                    BRS_CANCEL_INFO *cancel = (BRS_CANCEL_INFO *)payload;
                    quantity_t quantity;
                    if(exchange_cancel(exchange, trader, ntohl(cancel->order), &quantity) == 0){
                        info.orderid = cancel->order;
                        info.quantity = htonl(quantity);
                        exchange_get_status(exchange, &info);
                        trader_send_ack(trader, &info);
                    }else{
                        trader_send_nack(trader);
                    }
                    free(cancel);
                    break;
                }
            }
        }

    }
}