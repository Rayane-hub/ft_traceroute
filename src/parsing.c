#include "ft_traceroute.h"

int	ft_atoi(const char *str)
{
	int		sign;
	long	n;

	n = 0;
	sign = 1;
	while ((*str == ' ') || (*str >= '\t' && *str <= '\r'))
		str++;
	if (*str == '+' || *str == '-')
	{
		if (*str == '-')
			sign *= -1;
		str++;
	}
	while (*str >= '0' && *str <= '9')
	{
		if (n < 0 && sign == 1)
			return (-1);
		if (n < 0 && sign == -1)
			return (0);
		n *= 10;
		n = n + *str - '0';
		str++;
	}
    if (*str != '\0'){
        return(-1);
    }
	return (n * sign);
}

int parse_arg(int ac, char **av, t_data *data) {
    if (ac <= 1) 
        return(printf("Usage: ft_traceroute [--help] <destination>\n"), 1);
    for (int i = 1; i < ac; i++) 
    {
        if (strcmp(av[i], "--help") == 0)
            return(printf("Usage: ft_traceroute [--help] <destination>\n"), 1);
        if (strcmp(av[i], "-m") == 0) 
        {
            if(av[++i])
            {
                int hops = ft_atoi(av[i]);
                if (hops <= 0)
                    return (printf("first hop out of range\n"), 1);
                printf("arg = |%s|   res = |%d|\n", av[i], hops);
            }
            else
                return (fprintf(stderr, "Option `-m' requires an argument: `-m max_ttl'\n"), 2);
            continue;
        }
        if (av[i] && av[i][0] == '-')
            return (fprintf(stderr, "Bad option `%s'\n", av[i]), 1);
        if (!data->host)
            data->host = av[i];
        else
            return (fprintf(stderr, "ft_traceroute: extra operand `%s'\n", av[i]), 1);
    }
    return (data->host ? 0 : 1);
}
