#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 3001
#define MAX_CLIENTS 10


int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Falha ao inicializar o Winsock. Erro: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    SOCKET client_sockets[MAX_CLIENTS] = {0};
    char buffer[1024];

    // Cria socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Define endereço e porta
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associa socket à porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Escuta por conexões
    listen(server_fd, 3);
    printf("Servidor ouvindo na porta %d...\n", PORT);

    fd_set readfds;
    int max_fd;


    while (1) {
        // Lê do terminal sem select (Windows não suporta select para stdin)
        if (_kbhit()) {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            printf("Servidor escreveu: %s", buffer);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                SOCKET sd = client_sockets[i];
                if (sd > 0) {
                    send(sd, buffer, (int)strlen(buffer), 0);
                }
            }
        }

        // Prepara o conjunto de sockets
        FD_ZERO(&readfds);
        max_fd = (int)server_fd;
        FD_SET(server_fd, &readfds);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
                if ((int)sd > max_fd) max_fd = (int)sd;
            }
        }

        struct timeval tv = {0, 100000}; // 100ms
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity == SOCKET_ERROR) {
            printf("select error: %d\n", WSAGetLastError());
            continue;
        }

        // Nova conexão
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_socket == INVALID_SOCKET) {
                printf("accept failed: %d\n", WSAGetLastError());
                continue;
            }
            printf("Novo cliente conectado: socket %d\n", (int)new_socket);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Mensagens dos clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int valread = recv(sd, buffer, sizeof(buffer), 0);
                if (valread <= 0) {
                    printf("Cliente socket %d desconectado\n", (int)sd);
                    closesocket(sd);
                    client_sockets[i] = 0;
                } else {
                    printf("Mensagem do cliente %d: %s", (int)sd, buffer);
                }
            }
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
