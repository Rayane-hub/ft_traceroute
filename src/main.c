
#include <stdio.h>      //printf()
#include <string.h>     //strcmp()
#include <netdb.h>      //getaddrinfo()
#include <arpa/inet.h>  //inet_ntop()
#include <errno.h>      //errno
#include <unistd.h>     //close 

typedef struct s_data{
    char    *host;
    char    *host_two;
    int     send_sock;
    int     recv_sock;
    int     ttl;
    int     hops_max;
    int     seq;
}t_data;

void print_help()
{
    printf("Usage: ft_traceroute [--help] <destination>\n");
    printf("Trace route to an IPv4 host (address or hostname).\n\n");
    printf("Options:\n");
    printf("  --help    Display this help and exit\n");
}

int parse_arg(int ac, char **av, t_data *data)
{
    if (ac <= 1)
        return(printf("missing destination\n"), 1);
    for(int i = 1 ; i < ac ; i++)
    {
        if (av[i][0] == '-')
        {
            if (strcmp(av[i], "--help") == 0)
                return(print_help(), 0);
            else
                return(printf("Bad option `%s' (argc %d)\n", av[i], i), 1);
        }
        if (av[i] && data->host == NULL)
           data->host = av[i];
        else if (av[i] && data->host_two == NULL)
            data->host_two = av[i];
        else if (av[i] && data->host_two != NULL)
            return(printf("Extra arg `%s' (argc %d)\n", av[i], i), 1);
    }

    return 0;
}

int main(int ac, char **av)
{
/*************INIT & PARSING************************************** */
    t_data data;
    data.host = NULL;
    data.host_two = NULL;
    if (parse_arg(ac, av, &data))
        return 2;
/**************RESOLVE IPV4********************************************* */
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int ret = getaddrinfo(data.host, NULL, &hints, &res);
    if (ret != 0)
        return(fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret)));

    struct sockaddr_in dest = *(struct sockaddr_in *)res->ai_addr;
    dest.sin_port = htons(33434);
    freeaddrinfo(res);

    char ip_send[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, ip_send, sizeof(ip_send));
/***************SOCKET***************************************************** */

    data.send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (data.send_sock < 0)
        return(fprintf(stderr, "socket UDP: %s\n", strerror(errno)), 1);

    data.recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (data.recv_sock < 0)
        return(fprintf(stderr, "socket ICMP: %s\n", strerror(errno)), close(data.send_sock), 1);
/******************************************************************************************* */
    data.seq = 0;
    data.ttl = 1;
    data.hops_max = 30;

    /*while (data.ttl <= data.hops_max)
    {
        if (setsockopt(data.send_sock, IPPROTO_IP, IP_TTL, &data.ttl, sizeof(data.ttl)) < 0)
            return(perror("setsockopt IP_TTL"), 1);
        
    }*/

    if (setsockopt(data.send_sock, IPPROTO_IP, IP_TTL, &data.ttl, sizeof(data.ttl)) < 0)
        return(perror("setsockopt IP_TTL"), 1);
    
    char payload[32] = {0};
    int bytes_send = sendto(data.send_sock, payload, 32, 0, (struct sockaddr *)&dest, sizeof(dest));
    if (bytes_send < 0)
        perror("sendo ");
    printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", data.host, ip_send, data.hops_max, bytes_send);
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    unsigned char buf[1500];
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    int bytes_recv = recvfrom(data.recv_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
    if (bytes_recv < 0)
        perror("recv");
    printf("bytes recv = %d\n", bytes_recv);
    char ip_recv[INET_ADDRSTRLEN];
    inet_ntop(from.sin_family, &from.sin_addr, ip_recv, sizeof(ip_recv));
    printf("|%s|\n", ip_recv);
    return 0;
}
