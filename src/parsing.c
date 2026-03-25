#include "ft_traceroute.h"

int	ft_atoi(const char *str, bool *out)
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
    if (*str == '\0'){
        *out = true;
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
        *out = true;
    }
	return (n * sign);
}

bool flag_q(char **av, int i, t_data *data)
{
    bool out = false;
    if(av[i])
    {
        int probe = ft_atoi(av[i], &out);
    
        if (out)                        return(fprintf(stderr, "Cannot handle `-q' option with arg `%s' (argc %d)\n", av[i], i), false);
        if (probe <= 0 || probe > 10)   return (fprintf(stderr, "no more than 10 probes per hop\n"), false);
        data->probe_max = probe;
    }
    else                                return (fprintf(stderr, "Option `-q' (argc %d) requires an argument: `-q nqueries'\n", i), false);
    return (true);
}

bool flag_m(char **av, int i, t_data *data)
{
    bool out = false;
    if(av[i])
    {
        int hops = ft_atoi(av[i], &out);
        if (out)                return(fprintf(stderr, "Cannot handle `-m' option with arg `%s' (argc %d)\n", av[i], i), false);
        if (hops <= 0)          return (fprintf(stderr, "first hop out of range\n"), false);
        else if (hops > 255)    return (fprintf(stderr, "max hops cannot be more than 255\n"), false);
        data->hops_max = hops;
    }
    else                        return (fprintf(stderr, "Option `-m' requires an argument: `-m max_ttl'\n"), false);
    return (true);
}

bool parse_arg(int ac, char **av, t_data *data) {
    if (ac <= 1) 
        return(printf("Usage: ft_traceroute [--help] <destination>\n"), false);
    for (int i = 1; i < ac; i++) 
    {
        if (strcmp(av[i], "--help") == 0)
            return(printf("Usage: ft_traceroute [--help] <destination>\n"), false);
        if (strcmp(av[i], "-m") == 0) 
        {
            if (!flag_m(av, ++i, data)) return(false);
            continue;
        }
        if (strcmp(av[i], "-q") == 0) 
        {
            if (!flag_q(av, ++i, data)) return(false);
            continue;
        }
        if (av[i] && av[i][0] == '-')
            return (fprintf(stderr, "Bad option `%s'\n", av[i]), false);
        if (!data->host)
            data->host = av[i];
        else
            return (fprintf(stderr, "ft_traceroute: extra operand `%s'\n", av[i]), false);
    }
    return (data->host ? true : false);
}
