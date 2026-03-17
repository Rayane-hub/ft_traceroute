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

typedef struct s_data {
    char                *host;
    int                 send_sock;
    int                 recv_sock;
    int                 ttl;
    int                 hops_max;
    int                 seq;
    struct sockaddr_in  dest;
    char                ip_str[INET_ADDRSTRLEN];
} t_data;

/* --- AIDE & PARSING --- */

void print_help() {
    printf("Usage: ft_traceroute [--help] <destination>\n");
    printf("Trace route to an IPv4 host (address or hostname).\n\n");
    printf("Options:\n");
    printf("  --help    Display this help and exit\n");
}

int parse_arg(int ac, char **av, t_data *data) {
    if (ac <= 1) {
        fprintf(stderr, "missing destination\n");
        return (1);
    }
    for (int i = 1; i < ac; i++) {
        if (av[i][0] == '-') {
            if (strcmp(av[i], "--help") == 0) {
                print_help();
                exit(0);
            } else {
                fprintf(stderr, "Bad option `%s' (argc %d)\n", av[i], i);
                return (1);
            }
        }
        if (data->host == NULL) {
            data->host = av[i];
        } else {
            fprintf(stderr, "Cannot handle extra arg `%s' at position %d\n", av[i], i);
            return (1);
        }
    }
    if (!data->host) {
        fprintf(stderr, "missing destination\n");
        return (1);
    }
    return (0);
}

/* --- RÉSOLUTION RÉSEAU --- */

int resolve_dest(t_data *data) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(data->host, NULL, &hints, &res) != 0) {
        fprintf(stderr, "ft_traceroute: %s: Name or service not known\n", data->host);
        return (1);
    }
    data->dest = *(struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &data->dest.sin_addr, data->ip_str, sizeof(data->ip_str));
    freeaddrinfo(res);
    return (0);
}

int init_sockets(t_data *data) {
    struct timeval tv = {1, 0};
    data->send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    data->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    
    if (data->send_sock < 0 || data->recv_sock < 0) {
        perror("socket");
        return (1);
    }
    if (setsockopt(data->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        return (1);
    }
    return (0);
}

/* --- ANALYSE ET AFFICHAGE --- */

int check_response(unsigned char *buf) {
    struct iphdr *ip = (struct iphdr *)buf;
    struct icmphdr *icmp = (struct icmphdr *)(buf + (ip->ihl * 4));

    // ICMP Port Unreachable (Type 3, Code 3) signifie que la cible est atteinte
    if (icmp->type == 3 && icmp->code == 3)
        return (1);
    // ICMP Echo Reply (Type 0) au cas où la cible répond directement
    if (icmp->type == ICMP_ECHOREPLY)
        return (1);
    return (0);
}

void print_hop_info(struct sockaddr_in *from, char *last_ip, int *got_reply) {
    char ip_res[INET_ADDRSTRLEN];
    char host[NI_MAXHOST];

    inet_ntop(AF_INET, &from->sin_addr, ip_res, sizeof(ip_res));
    if (!(*got_reply) || strcmp(last_ip, ip_res) != 0) {
        if (getnameinfo((struct sockaddr *)from, sizeof(*from), host, sizeof(host), NULL, 0, 0) == 0)
            printf(" %s (%s)", host, ip_res);
        else
            printf(" %s (%s)", ip_res, ip_res);
        strncpy(last_ip, ip_res, INET_ADDRSTRLEN);
        *got_reply = 1;
    }
}

/* --- BOUCLE PRINCIPALE --- */

void traceroute_loop(t_data *data) {
    int reached = 0;
    printf("ft_traceroute to %s (%s), %d hops max, 32 byte packets\n", data->host, data->ip_str, data->hops_max);

    while (++data->ttl <= data->hops_max && !reached) {
        printf("%2d ", data->ttl);
        if (setsockopt(data->send_sock, IPPROTO_IP, IP_TTL, &data->ttl, sizeof(data->ttl)) < 0)
            break;

        char last_ip[INET_ADDRSTRLEN] = "";
        int got_reply_for_hop = 0;

        for (int probe = 0; probe < 3; probe++) {
            struct timeval t1, t2;
            gettimeofday(&t1, NULL);

            data->dest.sin_port = htons(33434 + data->seq++);
            sendto(data->send_sock, "42", 2, 0, (struct sockaddr *)&data->dest, sizeof(data->dest));

            unsigned char buf[1500];
            struct sockaddr_in from;
            socklen_t len = sizeof(from);
            int res = recvfrom(data->recv_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
            gettimeofday(&t2, NULL);

            if (res < 0) {
                printf("  *");
            } else {
                print_hop_info(&from, last_ip, &got_reply_for_hop);
                double rtt = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
                printf("  %.3f ms", rtt);
                if (check_response(buf)) reached = 1;
            }
            fflush(stdout);
        }
        printf("\n");
    }
}

int main(int ac, char **av) {
    t_data data;
    memset(&data, 0, sizeof(t_data));
    data.hops_max = 30;

    if (parse_arg(ac, av, &data))
        return (2);

    if (resolve_dest(&data) != 0)
        return (1);
    
    if (init_sockets(&data) != 0)
        return (1);

    traceroute_loop(&data);

    close(data.send_sock);
    close(data.recv_sock);
    return (0);
}