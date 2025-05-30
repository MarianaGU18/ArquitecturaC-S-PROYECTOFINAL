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
#define END_MARKER "__END__"

int main(int argc, char *argv[])
{
    struct sockaddr_in server;
    struct hostent *sp;
    int sd, n;
    char buf_peticion[BUF_SIZE];
    char buf_respuesta[BUF_SIZE];

    FILE *log = fopen("registro_cliente.txt", "a");
    if (!log)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <IP/host> <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((unsigned short)atoi(argv[2]));
    sp = gethostbyname(argv[1]);

    if (!sp)
    {
        perror("gethostbyname");
        exit(1);
    }

    memcpy(&server.sin_addr.s_addr, sp->h_addr_list[0], sp->h_length);

    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor.\n");
    fprintf(log, "Conectado al servidor %s:%s\n", argv[1], argv[2]);

    while (1)
    {
        memset(buf_peticion, 0, BUF_SIZE);
        printf("\n>> Comando: ");
        fgets(buf_peticion, BUF_SIZE, stdin);

        if (strncmp(buf_peticion, "salir", 5) == 0 || strncmp(buf_peticion, "exit", 4) == 0)
            break;

        // Guardar el comando enviado
        fprintf(log, "\n>> Comando enviado: %s", buf_peticion);

        // Enviar comando al servidor
        if (send(sd, buf_peticion, strlen(buf_peticion), 0) < 0)
        {
            perror("send");
            break;
        }

        // Leer respuesta del servidor hasta recibir el marcador de fin
        while (1)
        {
            memset(buf_respuesta, 0, BUF_SIZE);
            n = recv(sd, buf_respuesta, BUF_SIZE - 1, 0);
            if (n <= 0)
            {
                perror("recv");
                break;
            }
            buf_respuesta[n] = '\0';

            // Verificar si contiene el marcador de fin
            if (strstr(buf_respuesta, END_MARKER) != NULL)
            {
                char *pos = strstr(buf_respuesta, END_MARKER);
                *pos = '\0';
                printf("%s", buf_respuesta);
                fprintf(log, "%s", buf_respuesta);
                break;
            }

            printf("%s", buf_respuesta);
            fprintf(log, "%s", buf_respuesta);
        }
    }

    printf("\nConexión cerrada.\n");
    fprintf(log, "\nConexión cerrada.\n");

    close(sd);
    fclose(log);
    return 0;
}
