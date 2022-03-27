CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wconversion -pedantic -std=gnu11
LDFLAGS= -lpthread -ldl

all:	main

main: 	build_folders clientes server

clientes: clientes.o sockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliA src/obj/clienteA.o src/obj/mysockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliB src/obj/clienteB.o src/obj/mysockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliC src/obj/clienteC.o src/obj/mysockets.o

server: server.o sockets.o sqlite3.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/server src/obj/server.o src/obj/mysockets.o src/obj/sqlite3.o $(LDFLAGS)

clientes.o: src/clienteA.c src/clienteB.c src/clienteC.c 
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteA.c -o src/obj/clienteA.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteB.c -o src/obj/clienteB.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteC.c -o src/obj/clienteC.o

server.o: src/server.c 
		$(CC) $(CFLAGS) $(OFLAGS) -c src/server.c -o src/obj/server.o

sockets.o: src/include/mysockets.c src/include/mysockets.h
		$(CC) $(CFLAGS) $(OFLAGS) -c src/include/mysockets.c -o src/obj/mysockets.o

sqlite3.o: src/include/sqlite3.c src/include/sqlite3.h
		$(CC) $(OFLAGS) -c src/include/sqlite3.c -o src/obj/sqlite3.o

cppcheck: 
		cppcheck --enable=all --suppress=missingIncludeSystem src/ 2>err.txt

build_folders:
	mkdir -p ./src/obj ./src/bin 

clean:
	rm -f -r ./src/bin ./src/obj