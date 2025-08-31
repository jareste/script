# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: jareste- <jareste-@student.42barcel>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/05/10 14:07:30 by jareste-          #+#    #+#              #
#    Updated: 2023/05/15 14:55:07 by jareste-         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = libftprintf.a

CC = cc
CFLAGS = -Wall -Wextra -Werror -I../libft
SRC_PATH = .
LIBFT_DIR = ../libft
LIBFT = $(LIBFT_DIR)/libft.a

SRCS = ft_printf.c \
	ft_print_ptr.c \
	ft_print_uinteger.c \
	ft_print_decimal.c \
	ft_print_hex.c \
	ft_print_string.c \
	ft_print_double.c \
	ft_print_char_fd.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean fclean re

all: $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(LIBFT) $(OBJS)
	cp $(LIBFT) $(NAME)
	ar rcs $(NAME) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all
