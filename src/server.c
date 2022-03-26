#include "include/mysockets.h"
#include <poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/sendfile.h>

int main(int argc, char *argv[]){
    int sfdinet, sfdunix, sfdinet6;
    socklen_t address_size, server_struct_len;
    char msj[MAX_BUFFER];
    struct sockaddr_storage incoming_address;
	struct sockaddr_un server_address;
    struct pollfd *pfds = malloc(sizeof(*pfds) * 3); //3 sockets
    
    if(argc != 7){
        fprintf(stderr, "Uso: %s <archivo socket> <ipv4> <puerto ipv4> <ipv6> <puerto ipv6> <nombre archivo de datos>\n", argv[0]);
		exit(EXIT_SUCCESS);
    }

    /* preparando socket UNIX */
    unlink(argv[1]);

    memset((char *)&server_address, '0', sizeof(server_address));
    strcpy(server_address.sun_path, argv[1]);
    server_address.sun_family = AF_UNIX;
    server_struct_len = (socklen_t) (sizeof(server_address.sun_family) + strlen(server_address.sun_path));

    sfdunix = get_tcp_socket(PF_UNIX);
    make_listener_socket(sfdunix, (struct sockaddr *)&server_address, server_struct_len);

    /* sockets inet */
    sfdinet = get_inet_server_socket(argv[2], argv[3], IPV4);

    sfdinet6 = get_inet_server_socket(argv[4], argv[5], IPV6);

    /* reusar address de socket inmediatamente */
    int option = 1;
    setsockopt(sfdunix, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    setsockopt(sfdinet, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    setsockopt(sfdinet6, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    /* agrego los sockets a estructura para poll */
    pfds[0].fd = sfdinet;
    pfds[0].events = POLLIN; // se activa un flag cuando hay connect
    pfds[1].fd = sfdunix;
    pfds[1].events = POLLIN;
    pfds[2].fd = sfdinet6;
    pfds[2].events = POLLIN;
    
    /* seteo mi sighandler para SIGCHLD para matar zombies */
    instalar_handlers(sigchld_handler, SIGCHLD);

    while(1){
        errno = 0;
        int poll_count = poll(pfds, 3, -1); // hago polling a los fd de los sockets por si reciben conexion
        if(errno == EINTR) continue; //poll interrupted by signal

        if(poll_count < 0){
            error("Error poll");
        }else{ //never 0 because no timeout
            int nsfd;
            address_size = sizeof(incoming_address);

            if(pfds[0].revents & POLLIN){ //ipv4 socket
                nsfd = accept(sfdinet, (struct sockaddr *)&incoming_address, &address_size);
            }
            else if(pfds[1].revents & POLLIN){ //unix socket
                nsfd = accept(sfdunix, (struct sockaddr *)&incoming_address, &address_size);
            }else{ //ipv6 socket
                nsfd = accept(sfdinet6, (struct sockaddr *)&incoming_address, &address_size);
            }

            int pid2 = fork(); //creo un hijo para manejar la nueva conexion
            switch(pid2){
                case -1:
                    fprintf(stderr, "ERROR fork");
                    exit(EXIT_FAILURE);
                case 0:
                    close(sfdinet);//cierro los fd de sockets que no me interesan
                    close(sfdunix);
                    close(sfdinet6);
                    /*
                    man sendfile
                    tengo que hacer un send del file size (fstat) y desp mandar todo con sendfile
                    */
                    while(1){
                        int n = 0;
                        memset(msj, 0, MAX_BUFFER);
                        if((n = (int) recv(nsfd, msj, MAX_BUFFER - 1, 0)) <= 0){ //recibo un mensaje
                            printf("El cliente se desconectó.\n");
                            exit(EXIT_SUCCESS);
                        }
                    }
                    close(nsfd);
                    exit(EXIT_SUCCESS);
                default: //el padre avisa que se conecto alguien, cierra el nuevo socket fd y va a escuchar nuevas conexiones
                    printf("Nuevo cliente conectado. Atendido por proceso %i.\n", pid2);
                    close(nsfd);
            }
        }
    }
    close(sfdinet);
    close(sfdunix);
    close(sfdinet6);

    return 0;
}