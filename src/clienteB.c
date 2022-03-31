#include "include/headers/mysockets.h"

int main(int argc, char *argv[]){
    int sfd, port;
	struct sockaddr_in6 server_address;
    char buffer[MAX_BUFFER];
    memset(buffer, '\0', MAX_BUFFER);

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

    while(1){
        int n;
        
        fgets(buffer, MAX_BUFFER, stdin);
        buffer[strlen(buffer) - 1] = '\0';//reemplazo \n por \0
        if(strcmp(buffer, "quit") == 0) break;

        if((n = (int) write(sfd, buffer, strlen(buffer))) < 0) error("Error write");

        if((n = (int) read(sfd, buffer, MAX_BUFFER - 1)) < 0) error("Error read");

        printf("%s\n", buffer);
    }
    close(sfd);
    exit(EXIT_SUCCESS);
}