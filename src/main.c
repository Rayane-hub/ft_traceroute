#include <stdio.h>

int parse_arg(int ac, char **av)
{
    if (ac <= 1)
        return(printf("missing destination %s\n", av[0]), 1);
    return 0;
}

int main(int ac, char **av)
{
    parse_arg(ac, av);

    printf("Go traceroute =D\n");

    return 0;
}