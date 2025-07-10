#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 10

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int client_sockets[MAX_CLIENTS] = {0};
    char buffer[1024];

    // Cria socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Define endereço e porta
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associa socket à porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuta por conexões
    listen(server_fd, 3);
    printf("Servidor ouvindo na porta %d...\n", PORT);

    fd_set readfds;
    int max_fd;

    while (1) {
        FD_ZERO(&readfds);

        // Adiciona stdin
        FD_SET(STDIN_FILENO, &readfds);
        max_fd = STDIN_FILENO;

        // Adiciona socket do servidor
        FD_SET(server_fd, &readfds);
        if (server_fd > max_fd) max_fd = server_fd;

        // Adiciona todos os sockets dos clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_fd) max_fd = sd;
        }

        // Espera atividade
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            continue;
        }

        // Entrada do terminal (mensagem do servidor para todos)
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            printf("Servidor escreveu: %s", buffer);

            // Envia para todos os clientes
            for (int i = 0; i < MAX_CLIENTS; i++) {
                int sd = client_sockets[i];
                if (sd > 0) {
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }

        // Nova conexão
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept");
                continue;
            }
            printf("Novo cliente conectado: socket %d\n", new_socket);

            // Adiciona cliente no array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Mensagens dos clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int valread = read(sd, buffer, sizeof(buffer));
                if (valread <= 0) {
                    // Cliente desconectado
                    printf("Cliente socket %d desconectado\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    printf("Mensagem do cliente %d: %s", sd, buffer);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
