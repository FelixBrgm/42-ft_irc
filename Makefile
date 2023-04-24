NAME = ircserv

CC = g++
CFLAGS = -Wall -Wextra -Werror

SRC_DIR = ./src/
INC_DIR = ./inc/
SERVER_DIR = $(SRC_DIR)server/

SRC_FILES = Channel.cpp Client.cpp main.cpp
SERVER_FILES = Server.cpp channel_management.cpp login.cpp parser.cpp runtime.cpp user_management.cpp utils.cpp

SRCS = $(addprefix $(SRC_DIR), $(SRC_FILES)) $(addprefix $(SERVER_DIR), $(SERVER_FILES))
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ -c $<

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re