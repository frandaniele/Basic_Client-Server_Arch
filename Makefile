CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wconversion -pedantic -std=gnu11

all:	main

main: 	build_folders clientes server

clientes: clientes.o src/include/mysockets.c
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliA src/obj/clienteA.o src/include/mysockets.c
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliB src/obj/clienteB.o src/include/mysockets.c
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/cliC src/obj/clienteC.o src/include/mysockets.c

server: server.o src/include/mysockets.c
		$(CC) $(CFLAGS) $(OFLAGS) -o src/bin/server src/obj/server.o src/include/mysockets.c

clientes.o: src/clienteA.c src/clienteB.c src/clienteC.c src/include/mysockets.h
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteA.c -o src/obj/clienteA.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteB.c -o src/obj/clienteB.o
		$(CC) $(CFLAGS) $(OFLAGS) -c src/clienteC.c -o src/obj/clienteC.o

server.o: src/server.c src/include/mysockets.c src/include/mysockets.h
		$(CC) $(CFLAGS) $(OFLAGS) -c src/server.c -o src/obj/server.o

cppcheck: 
		cppcheck --enable=all --suppress=missingIncludeSystem src/ 2>err.txt

build_folders:
	mkdir -p ./src/obj ./src/bin 

clean:
	rm -f -r ./src/bin ./src/obj