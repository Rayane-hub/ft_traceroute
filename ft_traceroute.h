#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#define USEC_PER_SEC 1000000

typedef struct s_data {
    char                *host;
    int                 send_sock;
    int                 recv_sock;
    int                 ttl;
    int                 hops_max;
    uint16_t            start_port;
    struct sockaddr_in  dest;
    char                ip_str[INET_ADDRSTRLEN];
}   t_data;

int     parse_arg(int ac, char **av, t_data *data);
int     init_env(t_data *data);
void    traceroute_loop(t_data *data);