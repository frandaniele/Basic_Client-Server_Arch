#include "sqlite3.h"
#include <stdlib.h>
#include <stdio.h>

void open_db_connections(char *filename, sqlite3 *db);

int get_connection(int *list, int n);

void release_connection(int *list, int index);

int exec_query(char *db_name, sqlite3 *db_connection, char *query);