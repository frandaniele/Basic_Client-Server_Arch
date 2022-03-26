#include "mysockets.h"

int get_tcp_socket(int domain){
    int sfd = -1;
    
    if((sfd = socket(domain, SOCK_STREAM || SOCK_NONBLOCK, 0)) < 0){
		error("Error socket");
	}

    return sfd;
}

int get_tcp_client_socket(int domain, struct sockaddr * address, socklen_t address_struct_len){
    int sfd = get_tcp_socket(domain);

    if(connect(sfd, address, address_struct_len) < 0){
		close(sfd);
        error("Error connect");
	}

    return sfd;
}

void make_listener_socket(int sfd, struct sockaddr * address, socklen_t address_struct_len){
    if(bind(sfd, address, address_struct_len) < 0){
        close(sfd);
        error("Error bind"); 
    }

    if(listen(sfd, 5) == -1){
        close(sfd);
        error("Error listen");
    }
}

int get_inet_server_socket(char * ip, char * port, int ipv6){
    int sfd, status;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv6 ? AF_INET6 : AF_INET;  
    hints.ai_socktype = SOCK_STREAM;

    if((status = getaddrinfo(ip, port, &hints, &res)) != 0){
        fprintf(stderr, "Error getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    sfd = get_tcp_socket(res->ai_family);
    make_listener_socket(sfd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    return sfd;
}

void instalar_handlers(__sighandler_t s, int signal){
    struct sigaction sa;
    
    sa.sa_handler = s; // seteo mi sighandler para SIGCHLD
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(signal, &sa, NULL) == -1){
        error("Error sigaction");
    }
}

void sigint_handler(){
    exit(EXIT_SUCCESS);
}

void sigchld_handler(){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void error(char *msj){
    perror(msj);
    exit(EXIT_FAILURE);
}