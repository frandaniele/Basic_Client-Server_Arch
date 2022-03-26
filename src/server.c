#include "include/mysockets.h"
#include <poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    int sfdinet, sfdunix, sfdinet6;
    socklen_t address_size, server_struct_len;
    char msj[MAX_MSJ];
    struct sockaddr_storage incoming_address;
	struct sockaddr_un server_address;
    struct pollfd *pfds = malloc(sizeof(*pfds) * 3); //3 sockets
    
    if(argc != 7){
        fprintf(stderr, "Uso: %s <archivo socket> <ipv4> <puerto ipv4> <ipv6> <puerto ipv6> <nombre archivo de datos>\n", argv[0]);
		exit(EXIT_SUCCESS);
    }

    /* mapeo en un archivo la estructura donde escribire el BW */
    float* datos = mmap(NULL, 4*sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if(datos == MAP_FAILED){
        perror("Mapping");
        exit(EXIT_FAILURE);
    }	

    for(int i=0; i < 4; i++){
        datos[i] = 0;
    }

    int pid = fork(); //creo un hijo para lectura de datos
    switch(pid){
        case -1:
            fprintf(stderr, "ERROR fork");
            exit(EXIT_FAILURE);
        case 0: ;
            /* creo un archivo para guardar datos */
            FILE *fptr;
            if((fptr = fopen(argv[6], "w")) == NULL){
                perror("ERROR freopen");
                exit(EXIT_FAILURE);
            }

            struct timeval curr_time, last_time;
            float segs = 0;
            double total = 0, ipv4 = 0, ipv6 = 0, b_unix = 0;
            double prom_total = 0, prom_ipv4 = 0, prom_ipv6 = 0, prom_unix = 0;

            gettimeofday(&last_time, NULL); // para medicion de BW  
            while(1){
                long int time_diff;
                gettimeofday(&curr_time, NULL); //para medir BW c/ segundo
                time_diff = (long int) curr_time.tv_sec - last_time.tv_sec;

                if(time_diff >= 1){ //paso 1 seg
                    gettimeofday(&last_time, NULL); //actualizo tiempo
                    
                    segs += 1; //segundos que pasan
                    ipv4 += datos[0]; //total de kbytes de c/ protocolo
                    prom_ipv4 = ipv4/segs; // promedio por seg de c/ protocolo
                    ipv6 += datos[1];
                    prom_ipv6 = ipv6/segs;
                    b_unix += datos[2];
                    prom_unix = b_unix/segs;
                    total += datos[3];
                    prom_total = total/segs;

                    fprintf(fptr, "Protocol\t\tBand Width\t\tAverage [kB/s]\n"); //escribo en el archivo
                    fprintf(fptr, "IPV4\t\t\t\t%.2f\t\t\t\t%.2f\n", datos[0], prom_ipv4);
                    fprintf(fptr, "IPV6\t\t\t\t%.2f\t\t\t\t%.2f\n", datos[1], prom_ipv6);
                    fprintf(fptr, "UNIX\t\t\t\t%.2f\t\t\t\t%.2f\n", datos[2], prom_unix);
                    fprintf(fptr, "TOTAL\t\t\t\t%.2f\t\t\t\t%.2f\n", datos[3], prom_total);
                    rewind(fptr); //para reemplazar lo escrito
                    fflush(fptr);

                    datos[0] = 0; //reinicio para ir teniendo c/ segundo datos actuales
                    datos[1] = 0;
                    datos[2] = 0;
                    datos[3] = 0;
                }
            }
            fclose(fptr);
            exit(EXIT_SUCCESS);
        default:
            printf("El proceso %i se encargara de leer informacion.\n", pid);
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
            perror("Error poll");
            exit(EXIT_FAILURE);
        }else{ //never 0 because no timeout
            int nsfd, conexion;
            address_size = sizeof(incoming_address);

            if(pfds[0].revents & POLLIN){ //ipv4 socket
                nsfd = accept(sfdinet, (struct sockaddr *)&incoming_address, &address_size);
                conexion = IPV4;
            }
            else if(pfds[1].revents & POLLIN){ //unix socket
                nsfd = accept(sfdunix, (struct sockaddr *)&incoming_address, &address_size);
                conexion = UNIX;
            }else{ //ipv6 socket
                nsfd = accept(sfdinet6, (struct sockaddr *)&incoming_address, &address_size);
                conexion = IPV6;
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

                    while(1){
                        int n = 0;
                        memset(msj, 0, MAX_MSJ);
                        if((n = (int) recv(nsfd, msj, MAX_MSJ - 1, 0)) <= 0){ //recibo un mensaje
                            printf("El cliente se desconectó.\n");
                            exit(EXIT_SUCCESS);
                        }
                        /* sumo la cantidad de kbytes que recibe de c/ protocolo */
                        datos[3] += (float) n/1000;
                        datos[conexion] += (float) n/1000;
                    }
                    close(nsfd);
                    exit(EXIT_SUCCESS);
                default: //el padre avisa que se conecto alguien, cierra el nuevo socket fd y va a escuchar nuevas conexiones
                    printf("Nuevo cliente conectado. Atendido por proceso %i.\n", pid2);
                    close(nsfd);
            }
        }
    }
    if(munmap(datos, 4*sizeof(float))){
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }	
    close(sfdinet);
    close(sfdunix);
    close(sfdinet6);

    return 0;
}