#include "sqlite3.h"
#include <stdlib.h>
#include <stdio.h>

void open_db_connections(char *filename, sqlite3 *db);

void close_db_connections(sqlite3 **db_list);

int get_connection(int *list, int n);

void release_connection(int *list, int index);