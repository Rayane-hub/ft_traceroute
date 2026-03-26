#include "ft_traceroute.h"

int main(int ac, char **av) {
    t_data data = {0};
    data.hops_max = 30;
    data.probe_max = 3;
    data.start_port = 33434;
    data.flag_n = false;

    if (!parse_arg(ac, av, &data)) 
        return (2);
    if (init_env(&data)) 
        return (1);
    
    traceroute_loop(&data);

    close(data.send_sock);
    close(data.recv_sock);
    return (0);
}
