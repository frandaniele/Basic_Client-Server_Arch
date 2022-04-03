#include "../headers/mysqlite.h"
#include "../headers/mysockets.h"

/*
    funcion que recibe el nombre de una base de datos y un handler sqlite3
    y abre la conexion con la database en ese handler
    
    nota: no funca, ver. tambien revisar flags
*/
void open_db_connections(char *filename, sqlite3 *db){
    int rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    if(rc != SQLITE_OK){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

/*
    revisa si hay conexion disponible mediante una lista de 1s y 0s
    si la hay le devuelve el indice del handler
    y sino devuelve -1
*/
int get_connection(int *list, int n, sem_t *sem){
    sem_wait(sem);
    for(int i = 0; i < n; i++){// seccion critica
        if(list[i] == 1){
            list[i] = 0;
            sem_post(sem);
            return i;
        }
    }
    sem_post(sem);

    return -1;
}

/*
    libera la conexion de un handler poniendo en 1
    su valor en el vector segun su indice
*/
void release_connection(int *list, int index){
    list[index] = 1;
}

int exec_query(char *db_name, sqlite3 *db_connection, char *query, int (*callback)(void*, int, char**, char**), void *argToCback){
    char *err_msg = 0;
    int rc = sqlite3_exec(db_connection, query, callback, argToCback, &err_msg);
    
    if(rc != SQLITE_OK){
        int n;
        int *fd = (int *) argToCback;
        char str[2048];
        memset(str, '\0', 2048);

        sprintf(str, "SQL error: %s\n", err_msg);
        
        if((n = (int) write(*fd, str, strlen(str))) < 0) error("Error write");
        
        sqlite3_free(err_msg);        
        return -1;
    }

    return 0;
}

int callback(void *ptrToFD, int count, char **data, char **columns){
    int *fd = (int *) ptrToFD;
    int n;
    char str[MAX_BUFFER];

    for(int i = 0; i < count; i++){
        memset(str, '\0', MAX_BUFFER);
        sprintf(str, "%s = %s\n", columns[i], data[i] ? data[i] : "NULL");// me devuelve el nombre de la columna y el contenido
        if((n = (int) write(*fd, str, strlen(str))) < 0) error("Error write"); //escribo en el socket
    }
    if((n = (int) write(*fd, "\n", 1)) < 0) error("Error write");
    
    return 0;
}