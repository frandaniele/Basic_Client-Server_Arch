#include "include/mysockets.h"

int main(int argc, char *argv[]){
    int sfd;
	struct sockaddr_un client_address;
    char msj[MAX_MSJ];

    if(argc < 2){
        fprintf(stderr, "Necesitas especificar un archivo socket para comunicarte.\n");
        exit(EXIT_SUCCESS);
    }

    memset((char *)&client_address, '0', sizeof(client_address));
    strcpy(client_address.sun_path, argv[1]);
    client_address.sun_family = AF_UNIX;
  
    sfd = get_tcp_client_socket(PF_UNIX, (struct sockaddr *)&client_address, sizeof(client_address));

    instalar_handlers(sigint_handler, SIGINT);

    while(1){
        int n;
        
        memset(msj, '\0', MAX_MSJ);
        strcpy(msj, "hola unix");
		
        n = (int) write(sfd, msj, strlen(msj));
        if(n < 0){
            perror("Error write");
            exit(EXIT_FAILURE);
        }
    }
    close(sfd);
    exit(EXIT_SUCCESS);
}   