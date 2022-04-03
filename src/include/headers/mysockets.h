#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <errno.h>       
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BUFFER 1024

#define IPV4 0
#define IPV6 1
#define UNIX 2

#define CLI_A IPV4
#define CLI_B IPV6
#define CLI_C UNIX

int get_tcp_socket(int domain);

int get_tcp_client_socket(int domain, struct sockaddr * address, socklen_t address_struct_len);

void make_listener_socket(int sfd, struct sockaddr * address, socklen_t address_struct_len);

int get_inet_server_socket(char * ip, char * port, int ipv6);

void instalar_handlers(__sighandler_t s, int signal);

void sigint_handler();

void sigchld_handler();

void error(char * msj);