SHELL = /bin/bash
CC = clang
TARGET = ./bin
SRC = ./source/
OUT_NAME = server
CFLAGS = -I"includes"
run: server
	@./$(TARGET)/$(OUT_NAME)

server: target ./source
	$(CC) $(CFLAGS) $(SRC)/server.c -o "$(TARGET)/$(OUT_NAME)"

target:
	@if [ ! -d "$(TARGET)" ]; then mkdir "$(TARGET)"; fi