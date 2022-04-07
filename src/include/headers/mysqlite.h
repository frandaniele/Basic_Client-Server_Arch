#include "sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

void open_db_connections(char *filename, sqlite3 **db, int count);

int get_connection(int *list, int n, sem_t *sem);

void release_connection(int *list, int index);

int exec_query(char *db_name, sqlite3 *db_connection, char *query, int (*callback)(void*, int, char**, char**), void *argToCback);

int callback(void *NotUsed, int count, char **data, char **columns);