#include <poll.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "include/headers/sqlite3.h"
#include "include/headers/mysockets.h"
#include "include/headers/mysqlite.h"

/*
    INSERT INTO ?
    ROUND ROBIN ?
*/

int main(int argc, char *argv[]){
    int sfdinet, sfdunix, sfdinet6;
    socklen_t address_size, server_struct_len;
    struct sockaddr_storage incoming_address;
	struct sockaddr_un server_address;
    struct pollfd *pfds = malloc(sizeof(*pfds) * 3); //3 sockets
    sem_t *sem;    
    
    if(argc != 7){
        fprintf(stderr, "Uso: %s <archivo socket> <ipv4> <puerto ipv4> <ipv6> <puerto ipv6> <database>\n", argv[0]);
		exit(EXIT_SUCCESS);
    }

    char *db_name = argv[6];

    sem_unlink("semaforo");
    if((sem = sem_open("semaforo", O_CREAT | O_EXCL, 0644, 1)) == SEM_FAILED) error("semaphore");
    
    /* 5 handlers de db compartidos por todos */
    sqlite3 **db_connections = mmap(NULL, 5*sizeof(sqlite3 *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(db_connections == MAP_FAILED) error("Mapping");

    /* vector para controlar acceso a handlers */
    int* connections = mmap(NULL, 5*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(connections == MAP_FAILED) error("Mapping");

    for(int i = 0; i < 5; i++){
        //open_db_connections(db_name, db_connections[i]);
        int rc = sqlite3_open(db_name, &db_connections[i]);
        if(rc != SQLITE_OK){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_connections[i]));
            sqlite3_close(db_connections[i]);
            exit(EXIT_FAILURE);
        }
        connections[i] = 1;
    }

    char *query =   "DROP TABLE IF EXISTS Mensajes;"
                    "CREATE TABLE Mensajes(Id INTEGER PRIMARY KEY, Emisor TEXT, Mensaje TEXT);"
                    "INSERT INTO Mensajes(Emisor, Mensaje) VALUES ('Server', 'Creacion de tabla');";
    exec_query(db_name, db_connections[0], query, 0, 0);

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
            int nsfd, tipo_cliente = -1;
            address_size = sizeof(incoming_address);

            if(pfds[0].revents & POLLIN){ //ipv4 socket
                nsfd = accept(sfdinet, (struct sockaddr *)&incoming_address, &address_size);
                tipo_cliente = CLI_A;
            }
            else if(pfds[1].revents & POLLIN){ //unix socket
                nsfd = accept(sfdunix, (struct sockaddr *)&incoming_address, &address_size);
                tipo_cliente = CLI_C;
            }else{ //ipv6 socket
                nsfd = accept(sfdinet6, (struct sockaddr *)&incoming_address, &address_size);
                tipo_cliente = CLI_B;
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

                    int n = 0;
                    int n_conexion; 

                    switch(tipo_cliente){
                        case CLI_A: // estos clientes hacen lo mismo, mandan query y esperan respuesta
                        case CLI_B: ;
                            while((n_conexion = (get_connection(connections, 5, sem))) == -1); //si no hay handler disponible me quedo aca
                            char query2[MAX_BUFFER], ack[6] = "Ready\0";

                            while(1){
                                int *ptr_fd = &nsfd;

                                memset(query2, 0, MAX_BUFFER);

                                n = (int) read(nsfd, query2, MAX_BUFFER - 1); //recibo query
                                if(n <= 0) break; //cliente desconectado

                                exec_query(db_name, db_connections[n_conexion], query2, callback, ptr_fd);

                                if(tipo_cliente == CLI_B){ //registro solo los msjs del cliente tipo B
                                    char sql[2048];
                                    sprintf(sql, "INSERT INTO Mensajes(Emisor, Mensaje) VALUES ('Cliente B, atendido por %i', '%s');", getpid(), query2);
                                    exec_query(db_name, db_connections[n_conexion], sql, 0, 0);
                                }

                                n = (int) write(nsfd, ack, strlen(ack));
                                if(n < 0) error("Error write");
                            }

                            printf("El cliente se desconectó.\n");
                            release_connection(connections, n_conexion);
                            exit(EXIT_SUCCESS);

                        case CLI_C: ; // este cliente solicita descargar el archivo de la base de datos
                            while((n_conexion = (get_connection(connections, 5, sem))) == -1); //si no hay handler disponible me quedo aca

                            char sql[MAX_BUFFER];
                            sprintf(sql, "INSERT INTO Mensajes(Emisor, Mensaje) VALUES ('Cliente C, atendido por %i', 'Solicitud de descarga');", getpid());

                            exec_query(db_name, db_connections[n_conexion], sql, 0, 0); //registro solicitud de descarga del cliente C

                            release_connection(connections, n_conexion);

                            int out_fd = open(argv[6], O_RDONLY); //abro el archivo pasado como argumento
                            if(out_fd < 0) error("Error open");

                            struct stat finfo;// para calcular el tamaño del archivo
                            if(fstat(out_fd, &finfo) != 0) error("Error fstat");
                            
                            n = (int) write(nsfd, &finfo.st_size, sizeof(finfo.st_size));// le paso el tamaño
                            if(n < 0) error("Error write");

                            printf("Sending %ld bytes\n", finfo.st_size);

                            n = (int) sendfile(nsfd, out_fd, 0, (size_t) finfo.st_size); // le mando el archivo
                            if(n < 0) error("Error sendfile");

                            printf("Sended %i bytes\n", n);

                            printf("Descarga finalizada. El cliente se desconectó.\n");

                            close(out_fd);
                            exit(EXIT_SUCCESS);

                        default:
                            fprintf(stderr, "Error desconocido.\n");
                            exit(EXIT_FAILURE);
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
    sem_unlink("semaforo");
    sem_close(sem);

    for(int i = 0; i < 5; i++) sqlite3_close(db_connections[i]);

    return 0;
}