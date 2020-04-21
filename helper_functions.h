#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#ifdef DEBUG
static const char *packet_types[] = {
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
#endif

// void send_traded_packets(BRS_PACKET_HEADER *hdr, BRS_NOTIFY_INFO *info,uint8_t type, funds_t quantity, funds_t price, int seller_id, int buyer_id);
#endif