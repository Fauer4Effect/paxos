#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>

#include "logging.h"
#include "messages.h"
#include "multicast.h"

// Not reliable but paxos assumption doesn't require it
// Messages may be lost, reordered, or duplicated.
void multicast(unsigned char *header_buf, uint32_t header_size,
               unsigned char *msg_buf, uint32_t msg_size)
{
    int i;
    for (i = 0; i < NUM_PEERS; i++)
    {
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        if ((rv = getaddrinfo(PEERS[i], PORT, &hints, &servinfo)) != 0)
        {
            logger(1, LOG_LEVEL, MY_SERVER_ID, "getaddrinfo: %s\n", gai_strerror(rv));
        }

        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
                continue;
            break;
        }
        if (p == NULL)
        {
            logger(1, LOG_LEVEL, MY_SERVER_ID, "Failed to connect to %s\n", PEERS[i]);
            exit(1);
        }

        sendto(sockfd, header_buf, header_size, 0, p->ai_addr, p->ai_addrlen);
        sendto(sockfd, msg_buf, msg_size, 0, p->ai_addr, p->ai_addrlen);

        freeaddrinfo(servinfo);
        close(sockfd);
    }
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Multicast message successfully\n");
}

void send_to_single_host(unsigned char *header_buf, uint32_t header_size,
                         unsigned char *msg_buf, uint32_t msg_size, uint32_t server_id)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(PEERS[server_id], PORT, &hints, &servinfo)) != 0)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        break;
    }
    if (p == NULL)
    {
        logger(1, LOG_LEVEL, MY_SERVER_ID, "Failed to connect to %s\n", PEERS[server_id]);
        exit(1);
    }

    sendto(sockfd, header_buf, header_size, 0, p->ai_addr, p->ai_addrlen);
    sendto(sockfd, msg_buf, msg_size, 0, p->ai_addr, p->ai_addrlen);

    freeaddrinfo(servinfo);
    close(sockfd);
    logger(0, LOG_LEVEL, MY_SERVER_ID, "Sent message to server %d\n", server_id);
}
