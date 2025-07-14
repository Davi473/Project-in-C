#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")
#define MAX_CLIENTS 10
#define BUF_SIZE 1024
#define MAX_BLOCKS 1000

// Estrutura de bloco para o chat blockchain
typedef struct Block {
    int index;
    char user[64];
    char message[BUF_SIZE];
    char prev_hash[65];
    char hash[65];
    time_t timestamp;
} Block;

Block blockchain[MAX_BLOCKS];
int blockchain_size = 0;

// Função simples de hash (para exemplo, não seguro)
void simple_hash(const char *input, char *output) {
    unsigned long hash = 5381;
    int c;
    while ((c = *input++)) hash = ((hash << 5) + hash) + c;
    sprintf(output, "%lx", hash);
}

void calc_block_hash(Block *block, char *output) {
    char temp[BUF_SIZE + 256];
    snprintf(temp, sizeof(temp), "%d%s%s%s%ld", block->index, block->user, block->message, block->prev_hash, block->timestamp);
    simple_hash(temp, output);
}

void print_block(const Block *block) {
    struct tm *tm_info = localtime(&block->timestamp);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%d][%s] %s: %s\n", block->index, time_str, block->user, block->message);
}

void print_blockchain() {
    printf("--- Histórico do chat (blockchain) ---\n");
    for (int i = 0; i < blockchain_size; i++) {
        print_block(&blockchain[i]);
    }
    printf("--------------------------------------\n");
}

// Adiciona bloco à blockchain local
void add_block(const char *user, const char *message) {
    Block new_block;
    new_block.index = blockchain_size;
    strncpy(new_block.user, user, sizeof(new_block.user));
    strncpy(new_block.message, message, sizeof(new_block.message));
    new_block.timestamp = time(NULL);
    if (blockchain_size == 0) strcpy(new_block.prev_hash, "genesis");
    else strncpy(new_block.prev_hash, blockchain[blockchain_size-1].hash, sizeof(new_block.prev_hash));
    calc_block_hash(&new_block, new_block.hash);
    blockchain[blockchain_size++] = new_block;
}

// Serializa blockchain para string
void serialize_blockchain(char *out, size_t outsize) {
    out[0] = '\0';
    for (int i = 0; i < blockchain_size; i++) {
        char line[BUF_SIZE + 256];
        snprintf(line, sizeof(line), "%d|%s|%s|%s|%s|%ld\n", blockchain[i].index, blockchain[i].user, blockchain[i].message, blockchain[i].prev_hash, blockchain[i].hash, blockchain[i].timestamp);
        strncat(out, line, outsize - strlen(out) - 1);
    }
}

// Desserializa blockchain de string
void deserialize_blockchain(const char *in) {
    blockchain_size = 0;
    char *copy = strdup(in);
    char *line = strtok(copy, "\n");
    while (line && blockchain_size < MAX_BLOCKS) {
        Block b;
        sscanf(line, "%d|%63[^|]|%1023[^|]|%64[^|]|%64[^|]|%ld", &b.index, b.user, b.message, b.prev_hash, b.hash, &b.timestamp);
        blockchain[blockchain_size++] = b;
        line = strtok(NULL, "\n");
    }
    free(copy);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Falha ao inicializar o Winsock. Erro: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET server_fd = INVALID_SOCKET, new_socket;
    SOCKET client_sockets[MAX_CLIENTS] = {0};
    char buffer[BUF_SIZE*4];
    struct sockaddr_in address, peer_addr;
    int addrlen = sizeof(address);
    int port;
    char name_user[64];

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

    printf("Comandos:\n  /connect <ip> <porta>  - conectar a outro peer\n  /quit                  - sair\n  <mensagem>             - envia para todos conectados\n");

    fd_set readfds;
    int max_fd;

    // Gênesis
    add_block("system", "Bem-vindo ao chat blockchain!");
    print_blockchain();

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
                        // Solicita blockchain
                        send(s, "GETBLOCKS", 9, 0);
                        // Adiciona ao array
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
                printf("Comando desconhecido\n");
            } else {
                // Gera novo bloco e envia para todos, mostrando só o histórico formatado
                buffer[strcspn(buffer, "\n")] = 0;
                add_block(name_user, buffer);
                char msg[BUF_SIZE*4];
                serialize_blockchain(msg, sizeof(msg));
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    SOCKET sd = client_sockets[i];
                    if (sd > 0) {
                        send(sd, msg, (int)strlen(msg), 0);
                    }
                }
                print_blockchain();
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
                int valread = recv(sd, buffer, sizeof(buffer)-1, 0);
                if (valread <= 0) {
                    printf("Peer desconectado\n");
                    closesocket(sd);
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = 0;
                    if (strncmp(buffer, "GETBLOCKS", 9) == 0) {
                        // Peer pediu blockchain
                        char msg[BUF_SIZE*4];
                        serialize_blockchain(msg, sizeof(msg));
                        send(sd, msg, (int)strlen(msg), 0);
                    } else {
                        // Recebeu blockchain, atualiza se for maior
                        int old_size = blockchain_size;
                        deserialize_blockchain(buffer);
                        if (blockchain_size > old_size) {
                            print_blockchain();
                            // Repasse para outros peers
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
