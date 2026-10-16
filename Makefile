NAME = webserv
# CONFIG

CC = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -Iinclude -g3

# PATHING

SRC	= src/main.cpp \

SRC += src/engine/Booter.cpp \
	   src/engine/Engine.cpp \
	   src/engine/Engine_cgi.cpp \
	   src/engine/Engine_signals.cpp \
	   src/engine/Engine_help.cpp \
	   src/engine/Engine_state.cpp \
	   src/engine/Listener.cpp \

SRC += src/client/Client_cgi.cpp \
	   src/client/Client_path.cpp \
	   src/client/Client_state.cpp \
	   src/client/Client_utils.cpp \
	   src/client/Client.cpp \

SRC	+= src/config/ConfigLexer.cpp \
	   src/config/ConfigParser.cpp \
	   src/config/ConfigValidator.cpp \

SRC	+= src/http/HttpParser.cpp \
	   src/http/HttpParserUtils.cpp \
	   src/http/Request.cpp \
	   src/http/Cgi.cpp \
	   src/http/CgiResponse.cpp \
	   src/http/Response.cpp \

SRC +=	src/utils/AddrInfoGuard.cpp \
		src/utils/StringUtils.cpp \
		src/utils/execUtils.cpp \
		src/utils/Signals.cpp \
		src/utils/UniquePair.cpp \

SRC += src/debug/ConfigParserDebug.cpp \
	   src/debug/ConfigValidatorDebug.cpp \
	   src/debug/ListenerDebug.cpp \
	   src/debug/BooterDebug.cpp \
	   src/debug/RequestDebug.cpp \
	   src/debug/CgiDebug.cpp \
	   src/debug/ClientDebug.cpp \
	   src/debug/EngineDebug.cpp \
	   src/debug/debugUtils.cpp

OBJ_DIR = .build

OBJ     = $(addprefix $(OBJ_DIR)/,$(SRC:.cpp=.o))
DEPS    = $(OBJ:.o=.d)

GREEN   = \033[32m
RED     = \033[31m
BLUE    = \033[34m
BOLD    = \033[1m
RESET   = \033[0m

MAKEFLAGS += --no-print-directory

# TARGET RULES

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME)
	@echo "$(GREEN)Program is ready!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp Makefile
	@mkdir -p www/webserv.com/data
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

clean:
	rm -rf $(OBJ_DIR)
	@echo "$(RED)Object files deleted.$(RESET)"

fclean: clean
	rm -rf www/webserv.com/data
	rm -f $(NAME)
	@echo "$(RED)Executable deleted.$(RESET)"

re: fclean all

data:
	@rm -rf www/webserv.com/data
	@echo "$(RED)Data directory cleared.$(RESET)"
	@mkdir -p www/webserv.com/data
	@echo "$(GREEN)Data directory recreated.$(RESET)"

-include $(DEPS)

.PHONY: all clean fclean re data
