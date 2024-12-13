#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10
#define TABLE_COUNT 5

SOCKET clients[MAX_CLIENTS];
int client_count = 0;
CRITICAL_SECTION client_lock;

int tables[TABLE_COUNT]; 

void initialize_tables() {
    for (int i = 0; i < TABLE_COUNT; i++) {
        tables[i] = 0;
    }
}

void send_message(SOCKET client_socket, const char* message) {
    send(client_socket, message, strlen(message), 0);
}

void show_tables(SOCKET client_socket) {
    char message[256] = "Available Tables: ";
    char table_info[16];
    for (int i = 0; i < TABLE_COUNT; i++) {
        if (tables[i] == 0) {
            snprintf(table_info, sizeof(table_info), "[ %d ]", i + 1);
            strcat(message, table_info);
        }
    }
    strcat(message, "\n");
    send_message(client_socket, message);
}

void apply_table(SOCKET client_socket, int client_id) {
    for (int i = 0; i < TABLE_COUNT; i++) {
        if (tables[i] == 0) {
            tables[i] = client_id; // Store client ID instead of 1
            printf("Table %d assigned to Client %d\n", i + 1, client_id);
            char message[64];
            snprintf(message, sizeof(message), "Table %d assigned to you.\n", i + 1);
            send_message(client_socket, message);
            return;
        }
    }
    send_message(client_socket, "All tables are occupied.\n");
}

void release_table(int client_id) {
    for (int i = 0; i < TABLE_COUNT; i++) {
        if (tables[i] == client_id) {
            tables[i] = 0;
            printf("Table %d released by Client %d\n", i + 1, client_id);
            return;
        }
    }
}

void handle_client(void* client_socket_ptr) {
    SOCKET client_socket = *(SOCKET*)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[1024];
    int read_size;
    int client_id = client_socket;

    printf("New client connected: Client %d\n", client_id);

    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove newline characters
        printf("Client %d command: %s\n", client_id, buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("Client %d disconnected.\n", client_id);
            release_table(client_id);
            break;
        } else if (strcmp(buffer, "show") == 0) {
            show_tables(client_socket);
        } else if (strcmp(buffer, "apply") == 0) {
            apply_table(client_socket, client_id);
        } else {
            send_message(client_socket, "Invalid command. Available commands: exit, show, apply\n");
        }
    }

    EnterCriticalSection(&client_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            clients[i] = INVALID_SOCKET;
            client_count--;
            break;
        }
    }
    LeaveCriticalSection(&client_lock);

    closesocket(client_socket);
    _endthread();
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len;

    InitializeCriticalSection(&client_lock);
    initialize_tables();

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 3) == SOCKET_ERROR) {
        printf("Listen failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Restaurant Server Started. Port: %d\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = INVALID_SOCKET;
    }

    while (1) {
        addr_len = sizeof(client_addr);
        SOCKET* new_client_socket = malloc(sizeof(SOCKET));

        if ((*new_client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)) == INVALID_SOCKET) {
            printf("Accept failed. Error: %d\n", WSAGetLastError());
            free(new_client_socket);
            continue;
        }

        EnterCriticalSection(&client_lock);
        if (client_count >= MAX_CLIENTS) {
            send_message(*new_client_socket, "Server is at full capacity.\n");
            closesocket(*new_client_socket);
            free(new_client_socket);
        } else {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == INVALID_SOCKET) {
                    clients[i] = *new_client_socket;
                    client_count++;
                    break;
                }
            }
            _beginthread(handle_client, 0, new_client_socket);
        }
        LeaveCriticalSection(&client_lock);
    }

    DeleteCriticalSection(&client_lock);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
