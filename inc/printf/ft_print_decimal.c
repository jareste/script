/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_print_decimal.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jareste- <jareste-@student.42barcel>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/10 09:24:21 by jareste-          #+#    #+#             */
/*   Updated: 2023/05/15 16:20:43 by jareste-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"
#include <libft.h>

static int ft_print_decimal_recursive(int fd, long n)
{
    int count = 0;
    int temp;
    
    if (n > 9)
    {
        temp = ft_print_decimal_recursive(fd, n / 10);
        if (temp == -1)
            return (-1);
        count += temp;
    }
    
    if (ft_print_char_fd(fd, '0' + (n % 10), 1) == -1)
        return (-1);
    
    return (count + 1);
}

int ft_print_decimal(int fd, int n)
{
    int count = 0;
    long ln = n;
    
    if (n < 0)
    {
        if (ft_print_char_fd(fd, '-', 1) == -1)
            return (-1);
        count++;
        ln = -ln;
    }
    
    int temp = ft_print_decimal_recursive(fd, ln);
    if (temp == -1)
        return (-1);
    
    return (count + temp);
}