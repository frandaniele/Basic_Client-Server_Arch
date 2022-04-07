CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wconversion -pedantic -std=gnu11
LDFLAGS = -lpthread -ldl

all:	main

main: 	build_folders clientes server

clientes: clientes.o sockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliA src/obj/clienteA.o src/obj/mysockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliB src/obj/clienteB.o src/obj/mysockets.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliC src/obj/clienteC.o src/obj/mysockets.o

server: server.o sockets.o sqlite3.o mysqlite.o
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/server src/obj/server.o src/obj/mysockets.o src/obj/sqlite3.o src/obj/mysqlite.o $(LDFLAGS)

clientes.o: src/clienteA.c src/clienteB.c src/clienteC.c 
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteA.c -o src/obj/clienteA.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteB.c -o src/obj/clienteB.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteC.c -o src/obj/clienteC.o

server.o: src/server.c 
		$(CC) $(CFLAGS) $(OFLAGS) -c src/server.c -o src/obj/server.o

sockets.o: src/include/bodies/mysockets.c src/include/headers/mysockets.h
		$(CC) $(CFLAGS) $(OFLAGS) -c src/include/bodies/mysockets.c -o src/obj/mysockets.o

sqlite3.o: src/include/bodies/sqlite3.c src/include/headers/sqlite3.h
		$(CC) $(OFLAGS) -c src/include/bodies/sqlite3.c -o src/obj/sqlite3.o

mysqlite.o: src/include/bodies/mysqlite.c src/include/headers/mysqlite.h
		$(CC) $(OFLAGS) -c src/include/bodies/mysqlite.c -o src/obj/mysqlite.o

cppcheck: 
		cppcheck --enable=all --suppress=missingIncludeSystem src/clienteA.c src/clienteB.c src/clienteC.c src/server.c src/include/bodies/mysqlite.c src/include/bodies/mysockets.c 2>err.txt

build_folders:
	mkdir -p ./src/obj ./src/bin 

clean:
	rm -f -r ./src/bin ./src/obj