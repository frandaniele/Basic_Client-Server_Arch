#include "include/headers/mysockets.h"

int main(int argc, char *argv[]){
    int sfd = -1, port;
	struct sockaddr_in server_address;
    char query[MAX_BUFFER], answer[MAX_BUFFER];

    if(argc < 3){
		fprintf(stderr, "Uso: %s <host> <puerto>\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

    port = atoi(argv[2]);

    memset((char *)&server_address, '0', sizeof(server_address));
    inet_pton(AF_INET, argv[1], &server_address.sin_addr);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((uint16_t) port);

    sfd = get_tcp_client_socket(PF_INET, (struct sockaddr *)&server_address, sizeof(server_address));

    instalar_handlers(sigint_handler, SIGINT);
    
    memset(query, '\0', MAX_BUFFER);
    strcpy(query, "SELECT * FROM Cars;");
    while(1){
        int n;

        n = (int) write(sfd, query, strlen(query));
        if(n < 0) error("Error write");

        memset(answer, '\0', MAX_BUFFER);
        n = (int) read(sfd, answer, MAX_BUFFER - 1);//recibo hasta que no haya mas
        if(n < 0) error("Error read");   

        printf("%s\n", answer);
        sleep(2);
    }

    close(sfd);
    exit(EXIT_SUCCESS);
}