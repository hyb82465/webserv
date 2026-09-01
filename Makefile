# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: zhma <zhma@student.42.fr>                  +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/08/04 16:46:58 by yihe              #+#    #+#              #
#    Updated: 2026/08/24 09:41:26 by zhma             ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CXX = c++
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98

INCLUDES = -Iinclude
SRCS = src/main.cpp \
	   src/Server.cpp \
	   src/Client.cpp \
	   src/HttpRequest.cpp \
	   src/RequestParser.cpp \
	   src/HttpResponse.cpp \
	   src/RequestHandler.cpp \
	   src/CgiHandler.cpp \
	   src/config/ConfigParser.cpp \
	   src/config/ServerConfig.cpp \
	   src/config/Tokenizer.cpp \
	   src/config/TokenStream.cpp \
	   src/config/LocationConfig.cpp \
	   src/config/ListenConfig.cpp \
	   src/config/ConfigValidator.cpp \
	   src/Utils.cpp 
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
print:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"