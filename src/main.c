
#include <stdio.h>  //printf()
#include <string.h> //strcmp()
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> //

typedef struct s_data{
    char *host;
    char *host_two;
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
    t_data data;
    data.host = NULL;
    data.host_two = NULL;
    if (parse_arg(ac, av, &data))
        return 2;
    printf("|%s|\n", data.host);

    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    //hints.ai_socktype = SOCK_DGRAM;
    //hints.ai_protocole = IPPROTO_UDP;

    //int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
    int ret = getaddrinfo(data.host, NULL, &hints, &res);
    
    if (ret != 0)
        return(fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret)));
    
    struct sockaddr_in dst = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dst.sin_addr, ip, sizeof(ip));
    printf("ip = |%s|\n", ip);
    return 0;
}