#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")
#define MAX_CLIENTS 10
#define BUF_SIZE 1024

void print_help() {
    printf("Comandos:\n");
    printf("  /connect <ip> <porta>  - conectar a outro peer\n");
    printf("  /quit                  - sair\n");
    printf("  <mensagem>             - envia para todos conectados\n");
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Falha ao inicializar o Winsock. Erro: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET server_fd = INVALID_SOCKET, new_socket;
    SOCKET client_sockets[MAX_CLIENTS] = {0};
    char buffer[BUF_SIZE];
    struct sockaddr_in address, peer_addr;
    int addrlen = sizeof(address);
    int port;
    char name_user[64];
    char ip[64];

    printf("Digite a porta para escutar: ");
    scanf("%d", &port);
    printf("Digite o nome do usuário: ");
    scanf("%63s", name_user);
    getchar(); // consume newline

    // Cria socket servidor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    listen(server_fd, 3);
    printf("Escutando na porta %d...\n", port);

    print_help();

    fd_set readfds;
    int max_fd;

    while (1) {
        // Lê do terminal sem select
        if (_kbhit()) {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            if (strncmp(buffer, "/connect", 8) == 0) {
                // /connect <ip> <porta>
                char ipstr[64];
                int cport;
                if (sscanf(buffer, "/connect %63s %d", ipstr, &cport) == 2) {
                    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
                    if (s == INVALID_SOCKET) {
                        printf("Erro ao criar socket\n");
                        continue;
                    }
                    memset(&peer_addr, 0, sizeof(peer_addr));
                    peer_addr.sin_family = AF_INET;
                    peer_addr.sin_port = htons(cport);
                    inet_pton(AF_INET, ipstr, &peer_addr.sin_addr);
                    if (connect(s, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == SOCKET_ERROR) {
                        printf("Erro ao conectar\n");
                        closesocket(s);
                    } else {
                        printf("Conectado a %s:%d\n", ipstr, cport);
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (client_sockets[i] == 0) {
                                client_sockets[i] = s;
                                break;
                            }
                        }
                    }
                } else {
                    printf("Uso: /connect <ip> <porta>\n");
                }
            } else if (strncmp(buffer, "/quit", 5) == 0) {
                break;
            } else if (buffer[0] == '/') {
                print_help();
            } else {
                // Envia mensagem para todos conectados com o nome do usuário
                char msg_envio[BUF_SIZE + 64];
                snprintf(msg_envio, sizeof(msg_envio), "%s: %s", name_user, buffer);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    SOCKET sd = client_sockets[i];
                    if (sd > 0) {
                        send(sd, msg_envio, (int)strlen(msg_envio), 0);
                    }
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
            printf("Novo peer conectado!\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        // Mensagens dos peers
        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                int valread = recv(sd, buffer, sizeof(buffer), 0);
                if (valread <= 0) {
                    printf("Peer desconectado\n");
                    closesocket(sd);
                    client_sockets[i] = 0;
                } else {
                    // Exibe mensagem recebida
                    printf("%s", buffer);
                    // Repasse para outros peers (menos quem enviou)
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        SOCKET other = client_sockets[j];
                        if (other > 0 && other != sd) {
                            send(other, buffer, valread, 0);
                        }
                    }
                }
            }
        }
    }
    // Fecha tudo
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) closesocket(client_sockets[i]);
    }
    if (server_fd != INVALID_SOCKET) closesocket(server_fd);
    WSACleanup();
    return 0;
}
