SHELL = /bin/bash
CC = clang
TARGET = ./bin
SRC = ./source/
OUT_NAME = client
CFLAGS = -I"includes"

run: server
	$(TARGET)/$(OUT_NAME)
server: target ./source
	$(CC) $(CFLAGS) $(SRC)/client.c -o "$(TARGET)/$(OUT_NAME)"

target:
	@if [ ! -d "$(TARGET)" ]; then mkdir "$(TARGET)"; fi