/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_itoa.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jareste- <jareste-@student.42barcel>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/08 17:17:05 by jareste-          #+#    #+#             */
/*   Updated: 2023/05/08 22:22:11 by jareste-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

static char	*special(int n, char buf[20])
{
	if (n == -2147483648)
		ft_strlcpy(buf, "-2147483648", 20);
	if (n == 0)
		ft_strlcpy(buf, "0", 2);
	return (0);
}

static char	*get_num(int i, int n, char buf[20])
{
	if (n < 0)
	{
		n = -n;
		buf[0] = '-';
	}
	while (--i >= 0 && buf[i] == '\0')
	{
		buf[i] = n % 10 + '0';
		n = n / 10;
	}
	return (NULL);
}

char	*ft_itoa_nc(int n, char buf[20])
{
	int		i;
	int		temp;

	temp = n;
	i = 0;
	if (n == -2147483648 || n == 0)
		return (special(n, buf));
	if (n < 0)
		i++;
	while (++i && (temp > 9 || temp < -9))
		temp = temp / 10;
	return (get_num(i, n, buf));
}
/*
int main(void)
{
	printf("%s\n", ft_itoa(-10));
	printf("%s\n", ft_itoa(-9874));
	printf("%s\n", ft_itoa(8124));
	printf("%s\n", ft_itoa(8));
	return (0);


}*/
