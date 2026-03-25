#include "ft_traceroute.h"
/* --- RÉSOLUTION ET SOCKETS --- */
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
    data->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval tv = {1, 0};
    setsockopt(data->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    return (data->send_sock < 0 || data->recv_sock < 0);
}

/* --- P0 : FILTRAGE DES PAQUETS (PACKET MATCHING) --- 
** On vérifie que le paquet ICMP contient bien NOTRE paquet UDP original.
*/
int is_my_packet(unsigned char *buf, ssize_t len, uint16_t expected_port) {
    struct iphdr *ip = (struct iphdr *)buf;

    if (len < (ssize_t)sizeof(struct iphdr)) 
        return (0);
    int ip_hdr_len = ip->ihl * 4;
    if (len < ip_hdr_len + (ssize_t)sizeof(struct icmphdr)) return (0);

    struct icmphdr *icmp = (struct icmphdr *)(buf + ip_hdr_len);

    // Si c'est un Time Exceeded ou Destination Unreachable
    if (icmp->type == ICMP_TIME_EXCEEDED || icmp->type == ICMP_DEST_UNREACH) 
    {
        // Le "payload" de l'ICMP contient l'en-tête IP + 8 octets du paquet original
        if (len < ip_hdr_len + 8 + (ssize_t)sizeof(struct iphdr) + 8) 
            return (0);

        struct iphdr *inner_ip = (struct iphdr *)(buf + ip_hdr_len + 8);
        int inner_ip_len = inner_ip->ihl * 4;

        if (len < ip_hdr_len + 8 + inner_ip_len + 8) 
            return (0);

        struct udphdr *inner_udp = (struct udphdr *)((unsigned char *)inner_ip + inner_ip_len);
        
        // On vérifie si le port de destination correspond à celui envoyé
        if (ntohs(inner_udp->dest) == expected_port)
            return (icmp->type == ICMP_DEST_UNREACH ? 2 : 1);
    }
    return (0);
}

/* --- BOUCLE PRINCIPALE --- */
void traceroute_loop(t_data *data) {
    printf("ft_traceroute to %s (%s), %d hops max, 60 byte packets\n", data->host, data->ip_str, data->hops_max);

    for (data->ttl = 1; data->ttl <= data->hops_max; data->ttl++) {
        printf("%2d ", data->ttl);
        setsockopt(data->send_sock, IPPROTO_IP, IP_TTL, &data->ttl, sizeof(data->ttl));

        char last_ip[INET_ADDRSTRLEN] = "";
        int reached = 0;
        for (int probe = 0; probe < data->probe_max; probe++) 
        {
            struct timeval t1, t2;
            uint16_t curr_port = data->start_port + (data->ttl * 3) + probe;
            data->dest.sin_port = htons(curr_port);

            // Payload de 32 octets pour arriver à 60 octets au total (IP+UDP+Data)
            char payload[32] = {0};
            gettimeofday(&t1, NULL);
            sendto(data->send_sock, payload, sizeof(payload), 0, (struct sockaddr *)&data->dest, sizeof(data->dest));

            unsigned char buf[1500];
            struct sockaddr_in from;
            socklen_t len = sizeof(from);

            while (1) { // Boucle de lecture pour filtrer les paquets parasites
                struct timeval now;
                gettimeofday(&now, NULL);
                long elapsed_us = (now.tv_sec - t1.tv_sec) * USEC_PER_SEC + (now.tv_usec - t1.tv_usec);
                long timeout_us = USEC_PER_SEC - elapsed_us;

                if (timeout_us <= 0) { printf("  *"); break; }

                struct timeval tv = { timeout_us / USEC_PER_SEC, timeout_us % USEC_PER_SEC };
                setsockopt(data->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

                ssize_t res = recvfrom(data->recv_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
                if (res < 0) { printf("  *"); break; }

                int status = is_my_packet(buf, res, curr_port);
                if (status > 0) {
                    gettimeofday(&t2, NULL);
                    char curr_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &from.sin_addr, curr_ip, sizeof(curr_ip));

                    if (strcmp(last_ip, curr_ip) != 0) {
                        printf(" (%s)", curr_ip); // P0 : Pas de getnameinfo ici !
                        strcpy(last_ip, curr_ip);
                    }
                    printf("  %.3f ms", (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0);
                    if (status == 2)
                        reached = 1;
                    break;
                }
            }
        }
        printf("\n");
        if (reached) break;
    }
}
