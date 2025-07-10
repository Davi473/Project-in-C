
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 3001

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Falha ao inicializar o Winsock. Erro: %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    // Cria o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converte o IP (127.0.0.1 para local)
    if (inet_pton(AF_INET, "192.168.1.58", &serv_addr.sin_addr) <= 0) {
        printf("Endereço inválido\n");
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Conecta ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Erro na conexão: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    printf("Conectado ao servidor.\n");


    fd_set readfds;
    int max_fd;

    while (1) {
        // Lê do terminal sem select (Windows não suporta select para stdin)
        if (_kbhit()) {
            memset(buffer, 0, sizeof(buffer));
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                send(sock, buffer, (int)strlen(buffer), 0);
            }
        }

        // Prepara o conjunto de sockets
        FD_ZERO(&readfds);
        max_fd = (int)sock;
        FD_SET(sock, &readfds);

        struct timeval tv = {0, 100000}; // 100ms
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
        if (activity == SOCKET_ERROR) {
            printf("select error: %d\n", WSAGetLastError());
            break;
        }

        // Mensagem do servidor
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            int valread = recv(sock, buffer, sizeof(buffer), 0);
            if (valread <= 0) {
                printf("Servidor desconectado.\n");
                break;
            }
            printf("Servidor: %s", buffer);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
