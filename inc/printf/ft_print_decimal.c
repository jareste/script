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

int	ft_print_decimal(int fd, int n)
{
	char	*num;
	int		c_printed;
	char	buf[20];

	c_printed = 0;
	num = ft_itoa_nc(n, buf);
	if (!num)
		return (-1);
	c_printed = ft_print_string(fd, num);
	if (c_printed == -1)
	{
		free(num);
		return (-1);
	}
	free(num);
	return (c_printed);
}
