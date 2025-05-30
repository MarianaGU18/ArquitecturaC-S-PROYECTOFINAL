// servidor_exec.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define QLEN 5
#define BUF_SIZE 1024
int fd_s; // Global para poder cerrar en la señal

void cerrar_servidor(int sig)
{
    printf("\n[INFO] Servidor detenido manualmente (Ctrl+C). Cerrando socket...\n");
    if (fd_s >= 0)
    {
        close(fd_s);
        shutdown(fd_s, SHUT_RDWR);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    int fd_c;
    struct sockaddr_in servidor, cliente;
    struct hostent *info_cliente;
    socklen_t longClient;
    int num_cliente = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, cerrar_servidor); // Captura Ctrl+C

    // Crear socket
    if ((fd_s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((unsigned short)atoi(argv[1]));

    // Asignar dirección al socket
    if (bind(fd_s, (struct sockaddr *)&servidor, sizeof(servidor)) < 0)
    {
        perror("bind");
        close(fd_s);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(fd_s, QLEN) < 0)
    {
        perror("listen");
        close(fd_s);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %s...\n", argv[1]);

    longClient = sizeof(cliente);

    while (1)
    {
        fd_c = accept(fd_s, (struct sockaddr *)&cliente, &longClient);
        if (fd_c < 0)
        {
            perror("accept");
            continue;
        }

        num_cliente++;

        info_cliente = gethostbyaddr((char *)&cliente.sin_addr, sizeof(struct in_addr), AF_INET);

        time_t T = time(NULL);
        struct tm tm = *localtime(&T);
        printf("%02d/%02d/%04d %02d:%02d:%02d - Cliente conectado desde: %s\n",
               tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
               tm.tm_hour, tm.tm_min, tm.tm_sec,
               info_cliente ? info_cliente->h_name : inet_ntoa(cliente.sin_addr));

        if (fork() == 0)
        {
            // Proceso hijo
            close(fd_s);
            char buf_peticion[BUF_SIZE];

            while (1)
            {
                memset(buf_peticion, 0, BUF_SIZE);
                int n = recv(fd_c, buf_peticion, BUF_SIZE, 0);
                if (n <= 0)
                    break;

                buf_peticion[n] = '\0';
                if (strncmp(buf_peticion, "salir", 5) == 0 || strncmp(buf_peticion, "exit", 4) == 0)
                    break;

                printf("Cliente %d ejecuta: %s", num_cliente, buf_peticion);

                int pipefd[2];
                pipe(pipefd);

                pid_t pid = fork();
                if (pid == 0)
                {
                    // Nieto: ejecutar comando
                    dup2(pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDERR_FILENO);
                    close(pipefd[0]);
                    close(pipefd[1]);

                    char *args[64];
                    int i = 0;
                    char *token = strtok(buf_peticion, " \n");
                    while (token && i < 63)
                    {
                        args[i++] = token;
                        token = strtok(NULL, " \n");
                    }
                    args[i] = NULL;

                    execvp(args[0], args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    // Hijo: recibe y reenvía la respuesta
                    close(pipefd[1]);
                    char buf_respuesta[BUF_SIZE];
                    int bytes_leidos;

                    while ((bytes_leidos = read(pipefd[0], buf_respuesta, BUF_SIZE)) > 0)
                    {
                        send(fd_c, buf_respuesta, bytes_leidos, 0);
                    }
                    close(pipefd[0]);
                    wait(NULL);
                    send(fd_c, "__END__\n", 8, 0);
                }
            }

            printf("Conexión con el cliente finalizada. Cerrando hijo...\n");
            close(fd_c);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // Proceso padre
            close(fd_c);
        }
    }

    close(fd_s);
    return 0;
}