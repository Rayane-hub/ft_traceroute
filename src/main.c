#include <sys/types.h> // Types système pour compléter les dépendances
#include <sys/socket.h> // Pour les structures socket et addrinfo
#include <netinet/in.h> // Pour les structures réseau comme sockaddr_in
#include <netinet/ip.h> // Pour struct ip
#include <netinet/ip_icmp.h> // Pour struct icmp
#include <netinet/udp.h> // struct udphdr
#include <arpa/inet.h> // inet_ntop()
#include <netdb.h> // Pour struct addrinfo et NI_MAXHOST
#include <errno.h> // errno
#include <unistd.h> // close
#include <string.h> // strcmp()
#include <stdio.h> // printf()
#include <sys/time.h> // timeval
#include <string.h>
#include <stdlib.h>


typedef struct s_data
{
    char *host;
    int send_sock;
    int recv_sock;
    int ttl;
    int hops_max;
    int seq;
    char ip_last_recv[INET_ADDRSTRLEN];
} t_data;

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
        return (printf("missing destination\n"), 1);
    for (int i = 1; i < ac; i++)
    {
        if (av[i][0] == '-')
        {
            if (strcmp(av[i], "--help") == 0){
                print_help();
                exit(0);
            }
            else
                return (printf("Bad option `%s' (argc %d)\n", av[i], i), 1);
        }
        if (av[i] && data->host == NULL)
            data->host = av[i];
        else if (data->host && av[i])
            return (printf("Cannot handle packetlen cmdline arg `%s' on position %d (argc %d)\n", av[i], i, i), 1);
    }

    return 0;
}

int main(int ac, char **av)
{
/*************INIT & PARSING********************************************** */
    t_data data;
    data.host = NULL;
    if (parse_arg(ac, av, &data))
        return 2;
/**************RESOLVE IPV4*********************************************** */
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int ret = getaddrinfo(data.host, NULL, &hints, &res);
    if (ret != 0)
        return (fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret)));

    struct sockaddr_in dest = *(struct sockaddr_in *)res->ai_addr;
    dest.sin_port = htons(33434);
    freeaddrinfo(res);

    char ip_send[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, ip_send, sizeof(ip_send));
/***************SOCKET*********************************************************************** */

    data.send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (data.send_sock < 0)
        return (fprintf(stderr, "socket UDP: %s\n", strerror(errno)), 1);

    data.recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (data.recv_sock < 0)
        return (fprintf(stderr, "socket ICMP: %s\n", strerror(errno)), close(data.send_sock), 1);

/************************************************************************************************** */
    data.seq = 1;
    data.ttl = 0;
    data.hops_max = 30;
    int packet_size = 32;
    /* Print header once before the send/recv loop */
    printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", data.host, ip_send, data.hops_max, packet_size);
    int probe;
    struct timeval tv1, tv2, tv_nblock;
    tv_nblock.tv_sec = 2;
    tv_nblock.tv_usec = 0;
    if (setsockopt(data.recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_nblock, sizeof(tv_nblock)) < 0)
        return (perror("setsockopt NBLOCK"), 1);
    char ip_recv[INET_ADDRSTRLEN];
    while (++data.ttl <= data.hops_max && strcmp(ip_send, ip_recv) != 0)
    {
        probe = 0;
        if (setsockopt(data.send_sock, IPPROTO_IP, IP_TTL, &data.ttl, sizeof(data.ttl)) < 0)
            return (perror("setsockopt IP_TTL"), 1);
        do
        {
            if (!probe)
            {
                if (data.seq <= 9)
                    printf(" %d  ", data.seq);
                else
                    printf("%d  ", data.seq);
            }
            gettimeofday(&tv1, NULL);

            char payload[packet_size];
            memset(payload, 0, sizeof(payload));

            // Change le port UDP pour chaque probe -> dest.sin_port = htons(33434 + data.seq + probe);

            int bytes_send = sendto(data.send_sock, payload, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest));
            if (bytes_send < 0)
                perror("sendto");

            unsigned char buf[1500];
            struct sockaddr_in from;
            socklen_t len = sizeof(from);
            int bytes_recv = recvfrom(data.recv_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
            gettimeofday(&tv2, NULL);
            if (bytes_recv < 0)
            {
                printf("* ");
                if(++probe == 3)
                {   
                    data.seq++;
                    printf("\n");
                }
                continue;
            }

            inet_ntop(from.sin_family, &from.sin_addr, ip_recv, sizeof(ip_recv));
            if (probe == 1)
                strcpy(data.ip_last_recv, ip_recv);
            char host[NI_MAXHOST];
            if (!probe)
            {
                if (getnameinfo((struct sockaddr *)&from, sizeof(from), host, sizeof(host), NULL, 0, 0))
                    snprintf(host, sizeof(host), "%s", ip_recv);
                printf("%s (%s) ", host, ip_recv);
            }

            float rtt = (tv2.tv_sec - tv1.tv_sec) * 1000.0 + (tv2.tv_usec - tv1.tv_usec) / 1000.0;
            printf("%.3f ms ", rtt);

            if (++probe == 3)
            {
                data.seq++;
                printf("\n");
            }
        } while (probe < 3);
    }

    return 0;
}
