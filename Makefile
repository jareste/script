NAME = ft_script

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall -O3 -Iinc -Iinc/libft -Iinc/printf  -DDEBUG #-DNOECHO
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=address -O3 -I$(OPENSSL_BUILD_DIR)/include -I$(THIRD_PARTY_PATH)/cJSON -Iinc -DUSE_SSL #-DDEBUG
# CFLAGS = -Werror -Wextra -Wall -g -fsanitize=thread -O3 -I$(OPENSSL_BUILD_DIR)/include -I$(THIRD_PARTY_PATH)/cJSON -Iinc -DUSE_SSL #-DDEBUG
LDFLAGS = -Linc/libft -lft -Linc/printf -lftprintf
RELEASE_CFLAGS = -Werror -Wextra -Wall -g -O3
#########

#########
FILES = script log sighandlers filehandler

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs srcs/log srcs/sighandlers srcs/filehandler
#########

OBJ_DIR = objs

#########
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########

#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -Isrcs/parse_arg -Isrcs/server -c $< -o $@

all: .gitignore
	$(MAKE) -C inc/libft
	$(MAKE) -C inc/printf
	$(MAKE) $(NAME)

$(NAME): $(OBJ) $(CJSON_LIB)
	$(CC) $(CFLAGS) $(OBJ) $(CJSON_LIB) -o $(NAME) $(LDFLAGS)
	@echo "EVERYTHING DONE  "
#	@./.add_path.sh

release: CFLAGS = $(RELEASE_CFLAGS)
release: re
	@echo "RELEASE BUILD DONE  "

clean:
	$(RM) $(OBJ) $(DEP)
	$(RM) -r $(OBJ_DIR)
	$(MAKE) -C inc/libft clean
	$(MAKE) -C inc/printf clean
	@echo "OBJECTS REMOVED   "

fclean: clean
	$(RM) $(NAME)
	$(MAKE) -C inc/libft fclean
	$(MAKE) -C inc/printf fclean
	@echo "EVERYTHING REMOVED   "

re: fclean
	$(MAKE) all CFLAGS="$(CFLAGS)"

.gitignore:
	@if [ ! -f .gitignore ]; then \
		echo ".gitignore not found, creating it..."; \
		echo ".gitignore" >> .gitignore; \
		echo "$(NAME)" >> .gitignore; \
		echo "$(OBJ_DIR)/" >> .gitignore; \
		echo "*.o" >> .gitignore; \
		echo "*.d" >> .gitignore; \
		echo "*.a" >> .gitignore; \
		echo "*.pem" >> .gitignore; \
		echo ".vscode/" >> ../.gitignore; \
		echo ".gitignore" >> ../.gitignore; \
		echo "log.txt" >> .gitignore; \
		echo "typescript" >> .gitignore; \
		echo ".gitignore created and updated with entries."; \
	else \
		echo ".gitignore already exists."; \
	fi

.PHONY: all clean fclean re release .gitignore

-include $(DEP)