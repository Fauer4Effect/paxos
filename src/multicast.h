
extern int NUM_PEERS;
extern char *PEERS[];
extern char *PORT;
extern int LOG_LEVEL;
extern int MY_SERVER_ID;

void multicast(unsigned char *header_buf, uint32_t header_size, 
                unsigned char *msg_buf, uint32_t msg_size);

void send_to_single_host(unsigned char *header_buf, uint32_t header_size,
                        unsigned char *msg_buf, uint32_t msg_size, uint32_t server_id);