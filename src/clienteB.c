#include "include/headers/mysockets.h"

int main(int argc, char *argv[]){
    int sfd, port;
	struct sockaddr_in6 server_address;
    char query[MAX_BUFFER], answer[MAX_BUFFER], ack[6] = "Ready\0";
    memset(query, '\0', MAX_BUFFER);

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
        
        memset(query, '\0', MAX_BUFFER);
        fgets(query, MAX_BUFFER, stdin);
        query[strlen(query) - 1] = '\0';//reemplazo \n por \0
        if(strcmp(query, "quit") == 0) break;
        if(strcmp(query, "") == 0) continue;

        n = (int) write(sfd, query, strlen(query));
        if(n < 0) error("Error write");
        
        do{
            memset(answer, '\0', MAX_BUFFER);
            n = (int) read(sfd, answer, MAX_BUFFER - 1);//recibo hasta que no haya mas
            if(n < 0) error("Error read");   
            printf("%s\n", answer);
        }while(strncmp(ack, answer, 5) != 0);

    }
    close(sfd);
    exit(EXIT_SUCCESS);
}