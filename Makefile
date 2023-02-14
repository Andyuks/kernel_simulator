############
# Makefile #
############
# CC compiler options:
CC=gcc
CC_FLAGS=-pthread -Wall


## Project file structure ##
# Source file directory:
SRC_DIR = src
# Object file directory:
OBJ_DIR = obj
# Include header file diretory:
INC_DIR = include

## Make variables ##

# Target executable name:
BIN_DIR = bin
BIN = $(BIN_DIR)/kersim

# Object files:
SRC_C := $(wildcard $(SRC_DIR)/*.c)
OBJS_C := $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(BIN)

## Compile ##
# Link c and CUDA compiled object files to target executable:
$(BIN) : $(OBJS_C) | $(BIN_DIR)
	$(CC) $(CC_FLAGS) $(OBJS_C)  -o $@

# Compile main.c file to object files:
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CC_FLAGS) -c $< -o $@ $(CC_LIBS)


$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Clean objects in object directory.
clean:
	$(RM) -rv bin/kersim $(OBJ_DIR)

-include $(OBJ:.o=.d)
