#include "../headers/mysqlite.h"

/*
    funcion que recibe el nombre de una base de datos y un handler sqlite3
    y abre la conexion con la database en ese handler
    
    nota: revisar flags
*/
void open_db_connections(char *filename, sqlite3 *db){
    int rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
}

/*
    cierra las conexiones a database de los handlers pasados en una lista
*/
void close_db_connections(sqlite3 **db_list){
    sqlite3 *db = db_list[0];
    int i = 1;

    while(db != NULL){
        sqlite3_close(db);
        i++;
    }
}

/*
    revisa si hay conexion disponible mediante una lista de 1s y 0s
    si la hay le devuelve el indice del handler
    y sino devuelve -1
*/
int get_connection(int *list, int n){
    for(int i = 0; i < n; i++){
        if(list[i] == 1){
            list[i] = 0;
            return i;
        }
    }

    return -1;
}

/*
    libera la conexion de un handler poniendo en 1
    su valor en el vector segun su indice
*/
void release_connection(int *list, int index){
    list[index] = 1;
}