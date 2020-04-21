
#include "protocol.h"
#include "trader.h"
#include "exchange.h"
#include <semaphore.h>
typedef enum {
    //Order types
    CANCEL_ORDER,BUY_ORDER,SELL_ORDER
} ORDER_TYPE;

typedef struct ORDER
{
    orderid_t id;
    TRADER *trader_ref;
    int type;
    quantity_t quant;
    funds_t price;
    struct ORDER *next;
    struct ORDER *prev;
    sem_t mutex;
}ORDER;

typedef struct exchange
{
    ORDER *buy_orders;
    ORDER *buy_orders_tail;
    ORDER *sell_orders;
    ORDER *sell_orders_tail;
    funds_t last_price;
    orderid_t orderid;
    funds_t bid;
    funds_t ask;
    pthread_t tid;
    int check;
    sem_t mutex;
    sem_t matcher_mutex;
    sem_t matcher_wait;
}EXCHANGE;

void list_insert(ORDER **head, ORDER **tail, ORDER *node);


void list_remove(ORDER **head, ORDER **tail, ORDER *node);

void send_traded_packets(BRS_PACKET_HEADER *hdr, BRS_NOTIFY_INFO *info,uint8_t type, funds_t quantity, funds_t price, int seller_id, int buyer_id);

void printExchange(EXCHANGE *xchg);

void match_order(EXCHANGE *xchg);


        // sell_order = exchange->sell_orders;
        // buy_order = exchange->buy_orders_tail;
        // if(sell_order != NULL && exchange->orderid-1 == sell_order->id){
        //     while(buy_order != NULL){
        //         if(sell_order->price <= buy_order->price){
        //             price = (sell_order->price > exchange->last_price) ? sell_order->price : exchange->last_price;
        //             price = (buy_order->price < price) ? buy_order->price : price;
        //             quantity = sell_order->quant < buy_order->quant ? sell_order->quant : buy_order->quant;

        //             debug("Matchmaker %ld executing trade for quantity %u at price %u (sell order %d, buy order %d)",
        //                 pthread_self(),quantity,price,sell_order->id,buy_order->id);

        //             trader_increase_balance(sell_order->trader_ref, quantity * price);
        //             trader_increase_inventory(buy_order->trader_ref, quantity);

        //             //Sending bought packet
        //             send_traded_packets(&hdr, &info, BRS_BOUGHT_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_send_packet(buy_order->trader_ref,&hdr, &info);
        //             //Sending sold packet
        //             send_traded_packets(&hdr, &info, BRS_SOLD_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_send_packet(sell_order->trader_ref,&hdr, &info);
        //             //Broadcasting traded packet
        //             send_traded_packets(&hdr, &info, BRS_TRADED_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_broadcast_packet(&hdr, &info);

        //             sell_order->quant -= quantity;
        //             buy_order->quant -= quantity;
        //             exchange->last_price = price;
        //         }

        //         if(buy_order->quant == 0){
        //             removeFromLinkedList(&exchange->buy_orders,buy_order);
        //             trader_unref(buy_order->trader_ref,"in order being freed");
        //             temp = buy_order->next;
        //             free(buy_order);
        //             buy_order = temp;
        //         }else{
        //             buy_order = buy_order->next;
        //         }

        //         if(sell_order->quant == 0){
        //             removeFromLinkedList(&exchange->sell_orders,sell_order);
        //             trader_unref(sell_order->trader_ref,"in order being freed");
        //             free(sell_order);
        //             printExchange(exchange);
        //             break;
        //         }

        //         printExchange(exchange);
        //     }

        // }else if(buy_order != NULL && exchange->orderid-1 == buy_order->id){
        //     while(sell_order != NULL){
        //         if(sell_order->price <= buy_order->price){
        //             price = (sell_order->price > exchange->last_price) ? sell_order->price : exchange->last_price;
        //             price = (buy_order->price < price) ? buy_order->price : price;
        //             quantity = sell_order->quant < buy_order->quant ? sell_order->quant : buy_order->quant;

        //             debug("Matchmaker %ld executing trade for quantity %u at price %u (sell order %d, buy order %d)",
        //                 pthread_self(),quantity,price,sell_order->id,buy_order->id);

        //             trader_increase_balance(sell_order->trader_ref, quantity * price);
        //             trader_increase_inventory(buy_order->trader_ref, quantity);
        //             if(price < (buy_order->price)){
        //                 trader_increase_balance(buy_order->trader_ref, quantity * (buy_order->price - price));
        //             }

        //             //Sending bought packet
        //             send_traded_packets(&hdr, &info, BRS_BOUGHT_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_send_packet(buy_order->trader_ref,&hdr, &info);
        //             //Sending sold packet
        //             send_traded_packets(&hdr, &info, BRS_SOLD_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_send_packet(sell_order->trader_ref,&hdr, &info);
        //             //Broadcasting traded packet
        //             send_traded_packets(&hdr, &info, BRS_TRADED_PKT, quantity, price, sell_order->id, buy_order->id);
        //             trader_broadcast_packet(&hdr, &info);

        //             sell_order->quant -= quantity;
        //             buy_order->quant -= quantity;
        //             exchange->last_price = price;
        //         }

        //         if(sell_order->quant == 0){
        //             removeFromLinkedList(&exchange->sell_orders,sell_order);
        //             trader_unref(sell_order->trader_ref,"in order being freed");
        //             temp = sell_order->next;
        //             free(sell_order);
        //             sell_order = temp;
        //         }else{
        //             sell_order = sell_order->next;
        //         }

        //         if(buy_order->quant == 0){
        //             removeFromLinkedList(&exchange->buy_orders,buy_order);
        //             trader_unref(buy_order->trader_ref,"in order being freed");
        //             free(buy_order);
        //             printExchange(exchange);
        //             break;
        //         }
        //         printExchange(exchange);
        //     }
        // }
