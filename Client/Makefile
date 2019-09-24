SHELL = /bin/bash
CC = clang
TARGET = ./bin
SRC = ./source/
OUT_NAME = client
CFLAGS = -I"includes"

server: target ./source
	$(CC) $(CFLAGS) $(SRC)/client.c -o "$(TARGET)/$(OUT_NAME)" -lrt

target:
	@if [ ! -d "$(TARGET)" ]; then mkdir "$(TARGET)"; fi

run: server
	$(TARGET)/$(OUT_NAME)