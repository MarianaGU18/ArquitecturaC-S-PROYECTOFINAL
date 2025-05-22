// cliente.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    struct hostent *sp;
    int sd, n;
    char buf_peticion[BUF_SIZE];
    char buf_respuesta[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP/host> <puerto>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((u_short) atoi(argv[2]));
    sp = gethostbyname(argv[1]);

    if (!sp) {
        perror("gethostbyname");
        exit(1);
    }

    memcpy(&server.sin_addr, sp->h_addr, sp->h_length);

    if (connect(sd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect");
        exit(1);
    }

    while (1) {
        memset(buf_peticion, 0, BUF_SIZE);
        printf("\n>> Comando: ");
        fgets(buf_peticion, BUF_SIZE, stdin);

        if (strncmp(buf_peticion, "salir", 5) == 0 || strncmp(buf_peticion, "exit", 4) == 0)
            break;

        send(sd, buf_peticion, strlen(buf_peticion), 0);

// Nueva forma de recibir múltiples bloques hasta detectar __END__
while (1) {
    n = recv(sd, buf_respuesta, sizeof(buf_respuesta)-1, 0);
    if (n <= 0) break;

    buf_respuesta[n] = '\0';
    if (strcmp(buf_respuesta, "__END__\n") == 0) break;

    printf("%s", buf_respuesta); // o write(1, buf_respuesta, n);
}

    }

    printf("\nConexión cerrada.\n");
    close(sd);
    return 0;
}
