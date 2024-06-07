#include <stdio.h>
#include <syscall.h>

int my_atoi(const char *str)
{
    // NO ATOI IN GNU98 WHY
    int res = 0, sign = (*str == '-') ? -1 : 1;
    for (str += (*str == '-'); '0' <= *str && *str <= '9'; str++)
        res += 9 * res + *str - '0';
    return res * sign;
}

int main(int argc, char **argv)
{
    if (argc != 5)
    {
        printf("usage: additional <int> <int> <int> <int>\n");
        exit(-1);
    }

    int x = my_atoi(argv[1]),
        y = my_atoi(argv[2]),
        z = my_atoi(argv[3]),
        w = my_atoi(argv[4]);

    printf("%d %d\n", fibonacci(x), max_of_four_int(x, y, z, w));

    return 0;
}
