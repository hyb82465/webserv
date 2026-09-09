NAME = webserv

DEBUG ?= 0

CXX = c++
CPPFLAGS = -Iinclude -DWEBSERV_DEBUG=$(DEBUG)
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++98 -MMD -MP

OBJ_DIR = .build

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
OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean
	$(MAKE) all DEBUG=$(DEBUG)

print:
	@echo "SRCS=$(SRCS)"
	@echo "OBJS=$(OBJS)"
	@echo "DEPS=$(DEPS)"

-include $(DEPS)

.PHONY: all clean fclean re print