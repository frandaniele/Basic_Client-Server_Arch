#include <poll.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include "include/mysockets.h"
#include "include/sqlite3.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i = 0; i < argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");

    NotUsed = NotUsed;

    return 0;
}

void open_db_connections(char *filename, sqlite3 *db);

void close_db_connections(sqlite3 **db_list);

int get_connection(int *list, int n);

void release_connection(int *list, int index);

int main(int argc, char *argv[]){
    int sfdinet, sfdunix, sfdinet6;
    socklen_t address_size, server_struct_len;
    struct sockaddr_storage incoming_address;
	struct sockaddr_un server_address;
    struct pollfd *pfds = malloc(sizeof(*pfds) * 3); //3 sockets
    
    if(argc != 7){
        fprintf(stderr, "Uso: %s <archivo socket> <ipv4> <puerto ipv4> <ipv6> <puerto ipv6> <database>\n", argv[0]);
		exit(EXIT_SUCCESS);
    }

    /* open database connections */
    sqlite3 **db_connections = mmap(NULL, 5*sizeof(sqlite3 *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(db_connections == MAP_FAILED) error("Mapping");

    for(int i = 0; i < 5; i++) open_db_connections(argv[6], db_connections[i]);

    int* connections = mmap(NULL, 5*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(connections == MAP_FAILED) error("Mapping");

    for(int i = 0; i < 5; i++) connections[i] = 1;

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

                    int n_conexion; 
                    while((n_conexion = (get_connection(connections, 5))) == -1);

                    int n = 0;

                    switch(tipo_cliente){
                        case CLI_A: 
                        case CLI_B: ;
                            char query[MAX_BUFFER];

                            while(1){
                                memset(query, 0, MAX_BUFFER);
                                if((n = (int) recv(nsfd, query, MAX_BUFFER - 1, 0)) <= 0){ //recibo un mensaje
                                    break; //cliente desconectado
                                }
                                sqlite3_stmt *stmt;
                                int rc = sqlite3_prepare_v2(db_connections[n_conexion], query, -1, &stmt, NULL);
                                if(rc != SQLITE_OK){
                                    print("error: ", sqlite3_errmsg(db));
                                    exit(EXIT_FAILURE);
                                }

                                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                                    int id = sqlite3_column_int (stmt, 0);
                                    const char *name = sqlite3_column_text(stmt, 1);
                                    // ...
                                }
                                if(rc != SQLITE_DONE){
                                    print("error: ", sqlite3_errmsg(db));
                                }
                                sqlite3_finalize(stmt);


                                n = (int) write(nsfd, query, strlen(query));
                                if(n < 0) error("Error write");
                            }

                            printf("El cliente se desconectó.\n");
                            sqlite3_close(db_connections[n_conexion]);
                            release_connection(connections, n_conexion);
                            exit(EXIT_SUCCESS);
                        case CLI_C: ;
                             /*
                                man sendfile
                                tengo que hacer un send del file size (fstat) y desp mandar todo con sendfile
                                */
                            int out_fd = open(argv[6], O_RDONLY);
                            if(out_fd < 0) error("Error open");

                            struct stat finfo;
                            if(fstat(out_fd, &finfo) != 0) error("Error fstat");
                            n = (int) write(nsfd, &finfo.st_size, sizeof(finfo.st_size));
                            if(n < 0) error("Error write");

                            printf("Sending %ld bytes\n", finfo.st_size);

                            n = (int) sendfile(nsfd, out_fd, 0, (size_t) finfo.st_size);
                            if(n < 0) error("Error sendfile");

                            printf("Sended %i bytes\n", n);

                            printf("Descarga finalizada. El cliente se desconectó.\n");
                            sqlite3_close(db_connections[n_conexion]);
                            release_connection(connections, n_conexion);
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
    close_db_connections(db_connections);

    close(sfdinet);
    close(sfdunix);
    close(sfdinet6);

    return 0;
}

void open_db_connections(char *filename, sqlite3 *db){
    int rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

void close_db_connections(sqlite3 **db_list){
    sqlite3 *db = db_list[0];
    int i = 1;

    while(db != NULL){
        sqlite3_close(db);
        i++;
    }
}

int get_connection(int *list, int n){
    for(int i = 0; i < n; i++){
        if(list[i] == 1){
            list[i] = 0;
            return i;
        }
    }

    return -1;
}

void release_connection(int *list, int index){
    list[index] = 1;
}