CC = gcc

SRC = anchor-node peer-node

%:%.c
	$(CC) -o $@ $<

all: $(SRC)
