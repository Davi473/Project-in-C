#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    // Cria o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converte o IP (127.0.0.1 para local)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Endereço inválido\n");
        return -1;
    }

    // Conecta ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro na conexão");
        return -1;
    }

    printf("Conectado ao servidor.\n");

    fd_set readfds;
    int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);  // Entrada do terminal
        FD_SET(sock, &readfds);          // Resposta do servidor

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            break;
        }

        // Mensagem digitada pelo usuário
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }

        // Mensagem do servidor
        if (FD_ISSET(sock, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            int valread = read(sock, buffer, sizeof(buffer));
            if (valread <= 0) {
                printf("Servidor desconectado.\n");
                break;
            }
            printf("Servidor: %s", buffer);
        }
    }

    close(sock);
    return 0;
}
