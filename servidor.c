// servidor_exec.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

#define QLEN 2
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    struct sockaddr_in servidor, cliente;
    struct hostent* info_cliente;
    int fd_s, fd_c;
    socklen_t longClient;
    int num_cliente = 0;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        exit(1);
    }

    fd_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((u_short) atoi(argv[1]));

    bind(fd_s, (struct sockaddr *) &servidor, sizeof(servidor));
    listen(fd_s, QLEN);
    longClient = sizeof(cliente);

    while (1) {
        fd_c = accept(fd_s, (struct sockaddr *) &cliente, &longClient);
        num_cliente++;

        info_cliente = gethostbyaddr((char *) &cliente.sin_addr, sizeof(struct in_addr), AF_INET);

        time_t T = time(NULL);
        struct tm tm = *localtime(&T);
        printf("%02d/%02d/%04d %02d:%02d:%02d - Cliente conectado desde: %s\n",
               tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
               tm.tm_hour, tm.tm_min, tm.tm_sec,
               info_cliente ? info_cliente->h_name : inet_ntoa(cliente.sin_addr));

        if (fork() == 0) {
            close(fd_s);
            char buf_peticion[BUF_SIZE];
            while (1) {
                memset(buf_peticion, 0, BUF_SIZE);
                int n = recv(fd_c, buf_peticion, BUF_SIZE, 0);
                if (n <= 0) break;

                buf_peticion[n] = '\0';
                if (strncmp(buf_peticion, "salir", 5) == 0 || strncmp(buf_peticion, "exit", 4) == 0)
                    break;

                printf("Comando recibido del cliente %d: %s", num_cliente, buf_peticion);

                // Ejecutar el comando
                int pipefd[2];
                pipe(pipefd);
                pid_t pid = fork();

                if (pid == 0) {
                    // Hijo: redirigir salida al pipe y ejecutar el comando
                    dup2(pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDERR_FILENO);
                    close(pipefd[0]); close(pipefd[1]);

                    // Dividir comando en tokens
                    char *args[64];
                    int i = 0;
                    char *token = strtok(buf_peticion, " \n");
                    while (token != NULL && i < 63) {
                        args[i++] = token;
                        token = strtok(NULL, " \n");
                    }
                    args[i] = NULL;

                    execvp(args[0], args);
                    perror("execvp falló");
                    exit(1);
                } else {
                    // Padre: leer del pipe y enviar al cliente
                    close(pipefd[1]);
                    char buf_respuesta[BUF_SIZE];
                    int bytes_leidos;
                    while ((bytes_leidos = read(pipefd[0], buf_respuesta, BUF_SIZE)) > 0) {
                        send(fd_c, buf_respuesta, bytes_leidos, 0);
                    }
                    close(pipefd[0]);
                    wait(NULL);
                    // ⬇️ Marca de fin para que el cliente sepa que ya terminó la respuesta
                    char fin[] = "__END__\n";
                    send(fd_c, fin, strlen(fin), 0);
                }
            }

            printf("Se ha cerrado la conexión con el cliente... Este hijo termina\n");
            close(fd_c);
            exit(0);
        } else {
            close(fd_c);
        }
    }

    close(fd_s);
    shutdown(fd_s, SHUT_RDWR);
    return 0;
}
