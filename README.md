## Sistemas Operativos II - Laboratorio II IPC - Francisco Daniele
###  Ingeniería en Compitación - FCEFyN - UNC
# Interprocess Communication

## Desarrollo
Para realizar este trabajo organice los archivos de la siguiente manera:
```text
├── src
│    ├── bin
│    ├── include
│    │      ├── bodies
│    │      │      ├── mysockets.c
│    │      │      ├── mysqlite.c
│    │      │      └── sqlite3.c
│    │      └── headers
│    │             ├── mysockets.h
│    │             ├── mysqlite.h
│    │             └── sqlite3.h
│    ├── obj
│    ├── clienteA.c
│    ├── clienteB.c
│    ├── clienteC.c
│    └── server.c
├── Makefile
└── README.md
```
Los archivos _src/*.c_ son los que contienen el codigo que soluciona las actividades propuestas, la carpeta src/obj/ contendrá los archivos objeto y la carpeta src/bin/ los binarios ejecutables. Además en la carpeta src/include/ están las librerías propias _mysockets_ y _mysqlite_, y también _sqlite3_ la cuál es necesaria para el desarrollo del laboratorio.


## Clientes


## Server


## Diagramas de flujo
A continuación, se adjuntan los diagramas de flujo del servidor y de los clientes.