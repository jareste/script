/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jareste- <jareste-@student.42barcel>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/09 12:46:25 by jareste-          #+#    #+#             */
/*   Updated: 2023/05/15 14:55:14 by jareste-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PRINTF_H
# define FT_PRINTF_H

# include <stdarg.h>
# include <unistd.h>
# include <stdlib.h>

int	ft_print_decimal(int fd, int n);
int	ft_print_uinteger(int fd, unsigned int n);
int	ft_print_string(int fd, char *str);
int	ft_printf(const char *s, ...);
int	ft_dprintf(int fd, const char *s, ...);
int	ft_vdprintf(int fd, const char *s, va_list args);
int	ft_print_ptr(int fd, void *ptr);
int	ft_print_hex(int fd, unsigned long int n, char format, int c_printed);
int	ft_print_char_fd(int fd, int c, int fd2);
int ft_print_double(int fd, double n, int precision);

#endif
