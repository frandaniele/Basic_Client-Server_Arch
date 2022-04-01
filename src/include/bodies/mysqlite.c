#include "../headers/mysqlite.h"
#include <string.h>

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

int exec_query(char *db_name, sqlite3 *db_connection, char *query){
    char *err_msg = 0;
    int rc = sqlite3_exec(db_connection, query, 0, 0, &err_msg);
    
    if(rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db_connection);
        return -1;
    }

    return 0;
}