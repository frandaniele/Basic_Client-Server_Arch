#include "include/mysockets.h"

int main(int argc, char *argv[]){
    int sfd, n, file_size, bytes_recvd = 0;
	struct sockaddr_un client_address;
    char buffer[MAX_BUFFER];
    FILE *fptr;

    if(argc < 2){
        fprintf(stderr, "Necesitas especificar un archivo socket para comunicarte.\n");
        exit(EXIT_SUCCESS);
    }

    memset((char *)&client_address, '0', sizeof(client_address));
    strcpy(client_address.sun_path, argv[1]);
    client_address.sun_family = AF_UNIX;
  
    sfd = get_tcp_client_socket(PF_UNIX, (struct sockaddr *)&client_address, sizeof(client_address));

    memset(buffer, '\0', MAX_BUFFER);
        
    n = (int) read(sfd, &file_size, sizeof(int)); //recibe el tamaño del archivo en bytes
    if(n < 0) error("Error write");

    printf("Elige un nombre para el archivo:\n");
    char path[256];
    fgets(path, 256, stdin);
    path[strlen(path) - 1] = '\0';

    fptr = fopen(path, "w"); //crea un archivo que sera copia del recibido

    while(bytes_recvd < file_size){
        n = (int) read(sfd, buffer, MAX_BUFFER);
        if(n < 0) error("Error write");

        fwrite(buffer, sizeof(char), (size_t) n, fptr);

        bytes_recvd += n;
    }

    close(sfd);
    fclose(fptr);
    exit(EXIT_SUCCESS);
}   