# Target programs
NAME = price_server
TEST_CLIENT = test_client
STRESS_TEST = stress_test
MALFORMED_TEST = malformed_test

# Color codes
RED = \033[1;7;31m
GREEN = \033[1;7;32m
YELLOW = \033[1;7;33m
BLUE = \033[1;7;34m
MAGENTA = \033[1;3;35m
CYAN = \033[0;36m
RESET = \033[0m

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
RM = rm -rf

# Libraries and Includes
INCLUDES = -I$(HEADER_DIR)

# Header
HEADER_DIR = ./include/
HEADER_LIST = server.h
HEADER = $(addprefix $(HEADER_DIR), $(HEADER_LIST))

SOURCES_DIR = ./src/
SOURCES_LIST =	main.c

TEST_DIR = ./tests/
TEST_LIST = test_client.c \
			stress_test.c \
			malformed_test.c

SOURCES = $(addprefix $(SOURCES_DIR), $(SOURCES_LIST))
TESTS = $(addprefix $(TEST_DIR), $(TEST_LIST))

OBJECTS_DIR = objects/
OBJECTS_LIST = $(patsubst %.c, %.o, $(SOURCES_LIST))
OBJECTS = $(addprefix $(OBJECTS_DIR), $(OBJECTS_LIST))

TEST_OBJ_DIR = test_objects/
TEST_OBJ_LIST = $(patsubst %.c, %.o, $(TEST_LIST))
TEST_OBJ = $(addprefix $(TEST_OBJ_DIR), $(TEST_OBJ_LIST))

#Build all target program
all: $(NAME) $(TEST_CLIENT) $(STRESS_TEST) $(MALFORMED_TEST)

$(NAME): $(OBJECTS_DIR) $(OBJECTS)
	@echo "$(YELLOW) Building $(BLUE) SERVER $(YELLOW) program... $(RESET)\n"
	@$(CC) $(OBJECTS) -o $@
	@echo "$(GREEN) Done $(RESET)\n"

# Build test client
$(TEST_CLIENT): $(TEST_OBJ_DIR)test_client.o
	@echo "$(YELLOW) Building $(BLUE) TEST CLIENT $(YELLOW) program... $(RESET)\n"
	@$(CC) $(TEST_OBJ_DIR)test_client.o -o $@
	@echo "$(GREEN) Done $(RESET)\n"

# Build stress test
$(STRESS_TEST): $(TEST_OBJ_DIR)stress_test.o
	@echo "$(YELLOW) Building $(BLUE) STRESS TEST $(YELLOW) program... $(RESET)\n"
	@$(CC) $(TEST_OBJ_DIR)stress_test.o -o $@
	@echo "$(GREEN) Done $(RESET)\n"

# Build malformed test
$(MALFORMED_TEST): $(TEST_OBJ_DIR)malformed_test.o
	@echo "$(YELLOW) Building $(BLUE) MALFORMED TEST $(YELLOW) program... $(RESET)\n"
	@$(CC) $(TEST_OBJ_DIR)malformed_test.o -o $@
	@echo "$(GREEN) Done $(RESET)\n"

# Create folder objects dir
$(OBJECTS_DIR):
	@mkdir -p $(OBJECTS_DIR)
	@echo "$(GREEN) Create objects folder $(RESET)\n"

# Create test objects directory
$(TEST_OBJ_DIR):
	@mkdir -p $(TEST_OBJ_DIR)
	@echo "$(GREEN) Create test objects folder $(RESET)\n"

# ADD OBJECTS FILES
$(OBJECTS_DIR)%.o: $(SOURCES_DIR)%.c $(HEADER)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "$(GREEN) OBJECTS ADDED $(RESET)\n"

# ADD TEST OBJECTS FILES
$(TEST_OBJ_DIR)%.o: $(TEST_DIR)%.c | $(TEST_OBJ_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(GREEN) TEST OBJECTS ADDED $(RESET)\n"

# This is a target that deletes all objects files
clean:
	@echo "$(RED) Deleting objects files... $(RESET)\n"
	@$(RM) $(OBJECTS_DIR) $(TEST_OBJ_DIR)

# Clean built programs
fclean:
	@echo "$(RED) Cleaning built program... $(RESET)\n"
	@$(RM) -f $(NAME) $(TEST_CLIENT) $(STRESS_TEST) $(MALFORMED_TEST) $(OBJECTS_DIR) $(TEST_OBJ_DIR)
	@echo "$(RED) ALL CLEAR $(RESET)\n"

# Rebuild all
re:
	@$(MAKE) -s fclean
	@$(MAKE) -s all
	@echo "$(GREEN) DELETED AND RECOMPILED $(RESET)\n"

# Test targets
# Helper targets for easier testing
server: all
	@echo "$(GREEN) Starting server on port 8080... $(RESET)\n"
	@echo "$(YELLOW) Press Ctrl+C to stop $(RESET)\n"
	./$(NAME) 8080

server-val: all
	@echo "$(GREEN) Starting server with valgrind on port 8080... $(RESET)\n"
	@echo "$(YELLOW) Press Ctrl+C to stop $(RESET)\n"
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./$(NAME) 8080

# Test targets (require server to be running separately)
test: $(TEST_CLIENT)
	@echo "$(CYAN) Running comprehensive test suite... $(RESET)\n"
	@echo "$(YELLOW) Connecting to server on 127.0.0.1:8080 $(RESET)\n"
	./$(TEST_CLIENT) 127.0.0.1 8080

stress: $(STRESS_TEST)
	@echo "$(CYAN) Running multi-client stress test... $(RESET)\n"
	@echo "$(YELLOW) Connecting to server on 127.0.0.1:8080 $(RESET)\n"
	./$(STRESS_TEST) 127.0.0.1 8080

malformed: $(MALFORMED_TEST)
	@echo "$(CYAN) Running malformed message test... $(RESET)\n"
	@echo "$(YELLOW) Connecting to server on 127.0.0.1:8080 $(RESET)\n"
	./$(MALFORMED_TEST) 127.0.0.1 8080

fulltest: $(TEST_CLIENT) $(STRESS_TEST) $(MALFORMED_TEST)
	@echo "$(CYAN) Running ALL tests... $(RESET)\n"
	@echo "$(YELLOW) Connecting to server on 127.0.0.1:8080 $(RESET)\n"
	@echo "$(GREEN) 1. Running comprehensive test... $(RESET)"
	./$(TEST_CLIENT) 127.0.0.1 8080
	@echo "\n$(GREEN) 2. Running stress test... $(RESET)"
	./$(STRESS_TEST) 127.0.0.1 8080
	@echo "\n$(GREEN) 3. Running malformed message test... $(RESET)"
	./$(MALFORMED_TEST) 127.0.0.1 8080
	@echo "\n$(GREEN) ALL TESTS COMPLETED! $(RESET)"

.PHONY: all clean fclean re server server-val test stress malformed fulltest