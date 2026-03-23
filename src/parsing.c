#include "ft_traceroute.h"

/* --- AIDE ET PARSING --- */
void print_help() {
    printf("Usage: ft_traceroute [--help] <destination>\n");
}

int parse_arg(int ac, char **av, t_data *data) {
    if (ac <= 1) 
        return (fprintf(stderr, "Specify \"host\" missing\n"), 1);
    for (int i = 1; i < ac; i++) 
    {
        if (strcmp(av[i], "--help") == 0) { print_help(); exit(0); }
        if (av[i][0] == '-') return (fprintf(stderr, "Bad option `%s'\n", av[i]), 1);
        if (!data->host) data->host = av[i];
        else
            return (fprintf(stderr, "ft_traceroute: extra operand `%s'\n", av[i]), 1);
    }
    return (data->host ? 0 : 1);
}
