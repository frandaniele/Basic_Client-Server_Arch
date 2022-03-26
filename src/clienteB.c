#include "include/mysockets.h"

int main(int argc, char *argv[]){
    int sfd, port;
	struct sockaddr_in6 server_address;
    char msj[MAX_MSJ];

    if(argc < 3){
		fprintf(stderr, "Uso: %s <host> <puerto>\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

    port = atoi(argv[2]);

    memset((char *)&server_address, '0', sizeof(server_address));
    inet_pton(AF_INET6, argv[1], &server_address.sin6_addr);
    server_address.sin6_family = AF_INET6;
    server_address.sin6_port = htons((uint16_t) port);

    sfd = get_tcp_client_socket(PF_INET6, (struct sockaddr *)&server_address, sizeof(server_address));

    instalar_handlers(sigint_handler, SIGINT);
    
    while(1){
        int n;
        memset(msj, '\0', MAX_MSJ);
        strcpy(msj, "hola ipv6");

        n = (int) write(sfd, msj, strlen(msj));
        if(n < 0){
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
    close(sfd);
    exit(EXIT_SUCCESS);
}