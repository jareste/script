# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: jareste- <jareste-@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/05/04 12:08:30 by jareste-          #+#    #+#              #
#    Updated: 2023/09/25 11:13:45 by jareste-         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#Variables
HEADER	=	libft.h

NAME	=	libft.a

SRCS	=	ft_atoi.c \
			ft_isdigit.c \
			ft_isalpha.c \
			ft_isalnum.c \
			ft_isascii.c \
			ft_isprint.c \
			ft_strlen.c \
			ft_memset.c \
			ft_bzero.c \
			ft_memcpy.c	\
			ft_memmove.c \
			ft_strlcpy.c \
			ft_strlcat.c \
			ft_toupper.c \
			ft_tolower.c \
			ft_memchr.c \
			ft_strchr.c \
			ft_strrchr.c \
			ft_strncmp.c \
			ft_memcmp.c \
			ft_strnstr.c \
			ft_itoa_nc.c \
			ft_striteri.c \
			ft_putchar_fd.c \
			ft_putstr_fd.c \
			ft_putendl_fd.c \
			ft_putnbr_fd.c \


OBJS	= 	$(SRCS:.c=.o) #$(SRCS_B:.c=.o)

OBJS_B	=	$(SRCS_B:.c=.o)

CC	= 	gcc

RM	= 	rm -f

LIBC	= 	ar -rcs

FLAGS	= 	-Wall -Wextra -Werror -O3

%.o: %.c libft.h
	$(CC) $(FLAGS) -I ./ -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS) ${HEADER}
	$(LIBC) $(NAME) $(OBJS)

fclean: clean
	$(RM) $(NAME)

clean:
	$(RM) $(OBJS) $(OBJS_B)

re: fclean all

.PHONY: all clean fclean re bonus
