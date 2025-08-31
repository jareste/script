#include "ft_printf.h"

static int ft_print_integer_part(int fd, long long n, int c_printed)
{
    if (n == 0)
    {
        if (ft_print_char_fd(fd, '0', 1) == -1)
            return (-1);
        return (c_printed + 1);
    }

    if (n > 9)
        c_printed = ft_print_integer_part(fd, n / 10, c_printed);
    
    if (c_printed == -1)
        return (-1);

    if (ft_print_char_fd(fd, '0' + (n % 10), 1) == -1)
        return (-1);
    
    return (c_printed + 1);
}

int ft_print_double(int fd, double n, int precision)
{
    int c_printed = 0;
    long long integer_part;
    double fractional_part;
    int i;
    char *inf_str = "inf";
    char *nan_str = "nan";

    if (n < 0)
    {
        if (ft_print_char_fd(fd, '-', 1) == -1)
            return (-1);
        c_printed++;
        n = -n;
    }

    if (n != n)
    {
        for (i = 0; nan_str[i]; i++)
        {
            if (ft_print_char_fd(fd, nan_str[i], 1) == -1)
                return (-1);
            c_printed++;
        }
        return (c_printed);
    }

    if (n == 1.0/0.0)
    {
        for (i = 0; inf_str[i]; i++)
        {
            if (ft_print_char_fd(fd, inf_str[i], 1) == -1)
                return (-1);
            c_printed++;
        }
        return (c_printed);
    }

    if (precision < 0)
        precision = 6;

    integer_part = (long long)n;
    fractional_part = n - integer_part;

    c_printed = ft_print_integer_part(fd, integer_part, c_printed);
    if (c_printed == -1)
        return (-1);

    if (precision > 0)
    {
        if (ft_print_char_fd(fd, '.', 1) == -1)
            return (-1);
        c_printed++;

        for (i = 0; i < precision; i++)
        {
            fractional_part *= 10;
            int digit = (int)fractional_part;
            if (ft_print_char_fd(fd, '0' + digit, 1) == -1)
                return (-1);
            c_printed++;
            fractional_part -= digit;
        }
    }

    return (c_printed);
}
