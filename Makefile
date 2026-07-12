# Cross-platform Makefile for Insurance Management System
# Supports GCC on Linux, macOS, and MinGW on Windows

CC = gcc
# Note: -Wpedantic is intentionally omitted (GNU ##__VA_ARGS__ extension in logger.h).
# Note: -fstack-protector is excluded from this Makefile because MSYS2/MinGW64
# does not bundle the SSP runtime library. It is enabled in CMakeLists.txt for
# Linux builds where glibc provides the runtime support automatically.
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wstrict-prototypes -Wmissing-prototypes -std=c11 -Iinclude
LDFLAGS = -lsqlite3

# Source and Object files
SRC = src/main.c src/database.c src/customer.c src/policy.c src/claim.c src/logger.c src/error.c
OBJ = $(SRC:.c=.o)
TARGET = insurance_system

# Handle Windows executable extension
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q /F
    CLEAN_OBJ = src\*.o
else
    RM = rm -f
    CLEAN_OBJ = src/*.o
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(RM) $(CLEAN_OBJ) $(TARGET)

.PHONY: all clean
