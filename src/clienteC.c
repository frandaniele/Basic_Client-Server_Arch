#include "include/headers/mysockets.h"

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

    fptr = fopen("descarga", "w"); //crea un archivo que sera copia del recibido
    
    memset(buffer, '\0', MAX_BUFFER);
        
    n = (int) read(sfd, &file_size, sizeof(double)); //recibe el tamaño del archivo en bytes
    if(n < 0) error("Error read");

    printf("Receiving: %i bytes\n", file_size);

    while(bytes_recvd < file_size){ // recibe el archivo
        n = (int) read(sfd, buffer, MAX_BUFFER);
        if(n < 0) error("Error read");

        n = (int) fwrite(buffer, sizeof(char), (size_t) n, fptr);
        if(n < 0) error("Error fwrite");

        bytes_recvd += n;
    }

    printf("Bytes received: %i\n", bytes_recvd);

    if(bytes_recvd == file_size) printf("Descarga finalizada.\n");
    else{
        fprintf(stderr, "Error en la descarga del archivo de la database.\n");
        exit(EXIT_FAILURE);
    }

    close(sfd);
    fclose(fptr);
    exit(EXIT_SUCCESS);
}   