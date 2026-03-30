
#include "ft_traceroute.h"

/**
 * @brief Initializes the network environment: resolves the host, sets up sockets, and configures receive timeout.
 * @param data Pointer to the traceroute data structure.
 * @return 0 on success, 1 on error.
 */
int init_env(t_data *data) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(data->host, NULL, &hints, &res) != 0)
        return (fprintf(stderr, "ft_traceroute: %s: Name or service not known\n", data->host), 1);

    data->dest = *(struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &data->dest.sin_addr, data->ip_str, sizeof(data->ip_str));
    freeaddrinfo(res);

    data->send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (data->send_sock < 0)
        return (fprintf(stderr, "ft_traceroute: socket: %s\n", strerror(errno)), 1);
        
    data->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (data->recv_sock < 0)
        return (fprintf(stderr, "ft_traceroute: socket: %s\n", strerror(errno)), 1);
    struct timeval tv = {1, 0};
    setsockopt(data->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return (data->send_sock < 0 || data->recv_sock < 0);
}

/**
 * @brief Checks if the received ICMP packet matches the sent UDP packet (by port).
 * @param buf Buffer containing the received packet.
 * @param len Length of the buffer.
 * @param expected_port The destination port expected in the original UDP packet.
 * @return 1 if match (Time Exceeded), 2 if match (Destination Unreachable), 0 otherwise.
 */
static int is_my_packet(unsigned char *buf, ssize_t len, uint16_t expected_port) {
    struct iphdr *ip = (struct iphdr *)buf;

    if (len < (ssize_t)sizeof(struct iphdr)) 
        return (0);
    int ip_hdr_len = ip->ihl * 4;
    if (len < ip_hdr_len + (ssize_t)sizeof(struct icmphdr)) return (0);

    struct icmphdr *icmp = (struct icmphdr *)(buf + ip_hdr_len);

    if (icmp->type == ICMP_TIME_EXCEEDED || icmp->type == ICMP_DEST_UNREACH) 
    {
        if (len < ip_hdr_len + 8 + (ssize_t)sizeof(struct iphdr) + 8) 
            return (0);

        struct iphdr *inner_ip = (struct iphdr *)(buf + ip_hdr_len + 8);
        int inner_ip_len = inner_ip->ihl * 4;

        if (len < ip_hdr_len + 8 + inner_ip_len + 8) 
            return (0);

        struct udphdr *inner_udp = (struct udphdr *)((unsigned char *)inner_ip + inner_ip_len);

        if (ntohs(inner_udp->dest) == expected_port)
            return (icmp->type == ICMP_DEST_UNREACH ? 2 : 1);
    }
    return (0);
}

/**
 * @brief Prints the host or IP address for each hop.
 * @param data Pointer to the traceroute data structure.
 * @param from Pointer to the sockaddr_in structure of the current hop.
 * @param last_ip String containing the last printed IP address to avoid duplicates.
 */
static void print_hop_host(t_data *data, struct sockaddr_in *from, char *last_ip)
{
    char                curr_ip[INET_ADDRSTRLEN];
    char                hostname[NI_MAXHOST];
    struct sockaddr_in  sa = {0};

    inet_ntop(AF_INET, &from->sin_addr, curr_ip, sizeof(curr_ip));
    if (strcmp(last_ip, curr_ip) == 0)
        return ;
    if (data->flag_n)
        printf(" %s", curr_ip);
    else
    {
        sa.sin_family = AF_INET;
        sa.sin_addr = from->sin_addr;
        if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hostname, sizeof(hostname), NULL, 0, 0) == 0)
            printf(" %s (%s)", hostname, curr_ip);
        else
            printf(" %s", curr_ip);
    }
    strcpy(last_ip, curr_ip);
}

/**
 * @brief Waits for a reply to a probe and returns the status.
 * @param data Pointer to the traceroute data structure.
 * @param t1 Pointer to the timeval structure marking the start time.
 * @param curr_port The current UDP port used for the probe.
 * @param last_ip String containing the last printed IP address to avoid duplicates.
 * @return Status code indicating the result of the probe.
 */
static int wait_for_probe_reply(t_data *data, struct timeval *t1, uint16_t curr_port, char *last_ip)
{
    struct timeval      now;
    struct timeval      t2;
    struct timeval      tv;
    struct sockaddr_in  from;
    unsigned char       buf[1500];
    socklen_t           len;
    long                elapsed_us;
    long                timeout_us;
    ssize_t             res;
    int                 status;

    len = sizeof(from);
    while (1)
    {
        gettimeofday(&now, NULL);
        elapsed_us = (now.tv_sec - t1->tv_sec) * USEC_PER_SEC
            + (now.tv_usec - t1->tv_usec);
        timeout_us = USEC_PER_SEC - elapsed_us;
        if (timeout_us <= 0)
            return (printf(" *"), 0);
        tv = (struct timeval){timeout_us / USEC_PER_SEC,
            timeout_us % USEC_PER_SEC};
        setsockopt(data->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        res = recvfrom(data->recv_sock, buf, sizeof(buf), 0,
                (struct sockaddr *)&from, &len);
        if (res < 0)
            return (printf(" *"), 0);
        status = is_my_packet(buf, res, curr_port);
        if (status > 0)
        {
            gettimeofday(&t2, NULL);
            print_hop_host(data, &from, last_ip);
            printf("  %.3f ms", (t2.tv_sec - t1->tv_sec) * 1000.0 + (t2.tv_usec - t1->tv_usec) / 1000.0);
            return (status == 2);
        }
    }
}

/**
 * @brief Sends a UDP probe for the current TTL and waits for an ICMP reply.
 * @param data Pointer to the main data structure.
 * @param probe Probe number (for the UDP port).
 * @param last_ip String containing the last displayed IP (avoids duplicates).
 * @return 1 if the destination is reached, 0 otherwise.
 */
static int send_probe(t_data *data, int probe, char *last_ip)
{
    struct timeval  t1;
    uint16_t        curr_port;
    char            payload[32];

    curr_port = data->start_port + (data->ttl * 3) + probe;
    data->dest.sin_port = htons(curr_port);
    memset(payload, 0, sizeof(payload));
    gettimeofday(&t1, NULL);
    sendto(data->send_sock, payload, sizeof(payload), 0,(struct sockaddr *)&data->dest, sizeof(data->dest));
    return (wait_for_probe_reply(data, &t1, curr_port, last_ip));
}

/**
 * @brief Processes all probes for a given TTL, displays results, and indicates if the destination is reached.
 * @param data Pointer to the main data structure.
 * @return 1 if the destination is reached, 0 otherwise.
 */
static int process_ttl(t_data *data)
{
    char    last_ip[INET_ADDRSTRLEN];
    int     reached;
    int     probe;

    memset(last_ip, 0, sizeof(last_ip));
    reached = 0;
    printf("%2d ", data->ttl);
    setsockopt(data->send_sock, IPPROTO_IP, IP_TTL, &data->ttl,
        sizeof(data->ttl));
    probe = 0;
    while (probe < data->probe_max)
    {
        if (send_probe(data, probe, last_ip))
            reached = 1;
        probe++;
    }
    printf("\n");
    return (reached);
}

/**
 * @brief Main traceroute loop: increments TTL, launches probes, and displays results.
 * @param data Pointer to the main data structure.
 */
void traceroute_loop(t_data *data) {
    printf("ft_traceroute to %s (%s), %d hops max, 60 byte packets\n", data->host, data->ip_str, data->hops_max);

    for (data->ttl = data->first_ttl ; data->ttl <= data->hops_max; data->ttl++) {
        if (process_ttl(data))
            break;
    }
}
