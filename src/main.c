
#include <sys/time.h>  //timeval
#include <stdio.h>     //printf()
#include <string.h>    //strcmp()
#include <netdb.h>     //getaddrinfo() getnameinfo(0)
#include <arpa/inet.h> //inet_ntop()
#include <errno.h>     //errno
#include <unistd.h>    //close
/*
int main()
{
    struct timeval tv1;
    gettimeofday(&tv1, NULL);
    printf("sec = %ld\n", tv1.tv_sec);
    sleep(2);
    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    printf("sec = %ld\n", tv2.tv_sec);
    printf("sec diff = %ld\n", tv2.tv_sec - tv1.tv_sec);
    //tv.tv_usec;

}
*/

typedef struct s_data
{
    char *host;
    char *host_two;
    int send_sock;
    int recv_sock;
    int ttl;
    int hops_max;
    int seq;
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
            if (strcmp(av[i], "--help") == 0)
                return (print_help(), 0);
            else
                return (printf("Bad option `%s' (argc %d)\n", av[i], i), 1);
        }
        if (av[i] && data->host == NULL)
            data->host = av[i];
        else if (av[i] && data->host_two == NULL)
            data->host_two = av[i];
        else if (av[i] && data->host_two != NULL)
            return (printf("Extra arg `%s' (argc %d)\n", av[i], i), 1);
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
        return (fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret)));

    struct sockaddr_in dest = *(struct sockaddr_in *)res->ai_addr;
    dest.sin_port = htons(33434);
    freeaddrinfo(res);

    char ip_send[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dest.sin_addr, ip_send, sizeof(ip_send));
    /***************SOCKET***************************************************** */

    data.send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (data.send_sock < 0)
        return (fprintf(stderr, "socket UDP: %s\n", strerror(errno)), 1);

    data.recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (data.recv_sock < 0)
        return (fprintf(stderr, "socket ICMP: %s\n", strerror(errno)), close(data.send_sock), 1);
    /******************************************************************************************* */
    data.seq = 0;
    data.ttl = 1;
    data.hops_max = 30;
    int packet_size = 32;
    /* Print header once before the send/recv loop */
    printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", data.host, ip_send, data.hops_max, packet_size);
    int probe;
    while (data.ttl <= data.hops_max)
    {//*3
        probe = 0;
        do{
            if (!probe)
                printf("%d  ", data.seq);
            struct timeval tv1, tv2, tv_nblock;
            gettimeofday(&tv1, NULL);

            if (setsockopt(data.send_sock, IPPROTO_IP, IP_TTL, &data.ttl, sizeof(data.ttl)) < 0)
                return (perror("setsockopt IP_TTL"), 1);
            
            tv_nblock.tv_sec = 3;
            tv_nblock.tv_usec = 0;
            if (setsockopt(data.recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_nblock, sizeof(tv_nblock)) < 0)
                return (perror("setsockopt NBLOCK"), 1);

            char payload[packet_size];
            memset(payload, 0, sizeof(payload));
            int bytes_send = sendto(data.send_sock, payload, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest));
            if (bytes_send < 0)
                perror("sendto");
            // printf("bytes sent = %d\n", bytes_send);

            unsigned char buf[1500];
            struct sockaddr_in from;
            socklen_t len = sizeof(from);
            int bytes_recv = recvfrom(data.recv_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
            gettimeofday(&tv2, NULL);
            if (bytes_recv < 0){
                printf("* ");
                probe++;
                if(probe == 3)
                {   
                    data.ttl++;
                    data.seq++;
                    printf("\n");
                }
                continue;
            }
            char ip_recv[INET_ADDRSTRLEN];
            inet_ntop(from.sin_family, &from.sin_addr, ip_recv, sizeof(ip_recv));
            if (!probe)
                printf("%s   ", ip_recv);

            char host[NI_MAXHOST];
            if (!probe)
            {
                if (getnameinfo((struct sockaddr *)&from, sizeof(from), host, sizeof(host), NULL, 0, NI_NAMEREQD) != 0)
                    printf("(%s) ", host);
                else
                    printf("(%s) ", ip_recv);
            }
            float rtt = (tv2.tv_sec - tv1.tv_sec) * 1000.0 + (tv2.tv_usec - tv1.tv_usec) / 1000.0;
            printf("%.3f ms ", rtt);
            probe++;
           // printf("PROBE = %d\n", probe);
            if(probe == 3)
            {   
                data.ttl++;
                data.seq++;
                printf("\n");
            }
        } while (probe < 3);
    }

    return 0;
}
