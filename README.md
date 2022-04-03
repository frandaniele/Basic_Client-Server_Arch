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
### Cliente A
Este tipo de cliente es reutilizado del laboratorio anterior, se conecta con el servidor mediante protocolo INET con conexión. Es practicamente igual, con la diferencia de que todo el tiempo, cada 2 segundos, está escribiendo la query "SELECT * FROM Mensajes;" en el socket para obtener dicha tabla de la base de datos, mediante el server, lee la respuesta y la imprime en pantalla.

### Cliente B
Es el cliente de protocolo INET6 con conexión del trabajo anterior, por lo que la conexión con el servidor mediante el socket se mantiene igual. Lo nuevo para este cliente es que mediante _fgets_ obtiene una query del user mediante terminal. Si es "quit" se desconecta, y si no la envía al servidor mediante el socket para que este realice la consulta a la base de datos y obtenga la respuesta, reenviandosela al cliente ya sea una consulta válida o no.

### Cliente C
Utilicé el cliente con protocolo UNIX con conexión del laboratorio 1 y entonces la conexión con el server mediante el socket es la misma. Este cliente, mediante _fopen_, abre un archivo de nombre "decarga" en modo escritura, recibe la cantidad de bytes del tamaño del archivo de la base de datos y empieza a leer los bytes que le mande el servidor hasta que el mismo haya terminado. A medida que van llegando los va escribiendo en este archivo y al final se obtiene la base de datos.

## Server
-   Para cumplir con los requerimientos del laboratorio anterior, la parte de creación de sockets y gestión de conexiones se mantiene igual (3 sockets: 1 por protocolo, _poll + accept_ para reaccionar a conexiones y fork para manejar nueva conexión.). 
-   Conexiones con la database: crea un arreglo de 5 punteros _sqlite3_ de memoria compartida mediante _mmap_ y un arreglo de 5 ints de la misma forma. Luego mediante _sqlite3_open_ abre las 5 conexiones a la database con los 5 punteros antes mencionados e inicia los 5 enteros con valor 1, indicando que las 5 conexiones están disponibles. Para manejar la concurrencia al acceso a las conexiones se crea un semaforo mediante _sem_open_ compartido entre todos los procesos. 
-   A continuación, mediante _exec_query_ (función propia, wrapper de _sqlite3_exec_ con control de errores) crea una tabla llamada "Mensajes" que registra el n° de mensaje, el emisor y el propio mensaje donde se irán registrando las consultas de los clientes.
-   Conexiones y desconexiones: 
    - Socket: se gestionan mediante _poll_ (escucha c/ socket), y _accept_. Una vez conectado el cliente se crea un proceso hijo para atenderlo, se identifica que tipo es y se atiende según cuál sea. Cuando un cliente se desconecta, el proceso que lo atendía hace su salida limpia y el proceso padre se encarga de limpiarlo. 
    - Database: mediante la funcion _get_connection_, la cual mediante un semaforo controla la obtencion de un entero que representa cual de los handlers de BD del arreglo compartido, se obtiene un numero de handler o -1, indicando que no hay disponibilidad. La desconexion se maneja con _release_connection_ la cual vuelve a indicar disponibilidad del handler liberado.
-   Una vez identificado el cliente, hay 2 opciones:
    -   CLI_A y CLI_B: se manejan de la misma forma, ya que ambos mandan una consulta y esperan la respuesta. Lo primero que se hace es intentar conseguir acceso a un handler de BD mediante _get_connection_, una vez conseguido se lee la query del cliente y se ejecuta en la BD mediante _exec_query_ con la funcion callback que se encarga de ir escribiendo la respuesta en el cliente mediante el socket. Luego si se trata de CLI_B se registra la query en la tabla "Mensajes" y se escribe en el socket "Ready" como un ACK. Esto se repite todo el tiempo hasta que el cliente se desconecte, con la particularidad de que cada 10 segundos se libera el handler de BD para darle lugar a otro cliente, y se vuelve a intentar obtener la misma u otra conexion.
    -   CLI_C: se intenta obtener una conexion a BD, luego se registra la solicitud de descarga en la tabla "Mensajes" y se suelta la conexion. A continuación se abre el archivo de la base de datos en modo lectura, se calcula su tamaño en bytes y se escribe en el socket para el cliente dicho tamaño. Por último se envía todo el archivo al cliente mediante _sendfile_

## Diagramas de flujo
A continuación, se adjuntan los diagramas de flujo del servidor y de los clientes.