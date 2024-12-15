#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define TABLE_COUNT 5

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_lock;

int tables[TABLE_COUNT];

// Function to log each command to a file
void log_command(int client_id, const char* command) {
    FILE* file = fopen("commands.log", "a");
    if (file == NULL) {
        perror("File open failed");
        return;
    }
    fprintf(file, "Client %d command: %s\n", client_id, command);
    fclose(file);
}

// Save table statuses to a file
void save_table_status() {
    FILE* file = fopen("tables.txt", "w");
    if (file == NULL) {
        perror("File open failed");
        return;
    }
    for (int i = 0; i < TABLE_COUNT; i++) {
        fprintf(file, "Table %d: %s\n", i + 1, tables[i] == 0 ? "Available" : "Occupied");
    }
    fclose(file);
}

// Initialize tables
void initialize_tables() {
    for (int i = 0; i < TABLE_COUNT; i++) {
        tables[i] = 0;
    }
    save_table_status();
}

void send_message(int client_socket, const char* message) {
    send(client_socket, message, strlen(message), 0);
}

void show_tables(int client_socket) {
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

void apply_table(int client_socket, int client_id) {
    for (int i = 0; i < TABLE_COUNT; i++) {
        if (tables[i] == 0) {
            tables[i] = client_id; // Store client ID instead of 1
            printf("Table %d assigned to Client %d\n", i + 1, client_id);
            char message[64];
            snprintf(message, sizeof(message), "Table %d assigned to you.\n", i + 1);
            send_message(client_socket, message);
            save_table_status(); // Save table statuses
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
            save_table_status(); // Save table statuses
            return;
        }
    }
}

void* handle_client(void* client_socket_ptr) {
    int client_socket = *(int*)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[1024];
    int read_size;
    int client_id = client_socket;

    printf("New client connected: Client %d\n", client_id);

    while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[read_size] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Remove newline characters
        printf("Client %d command: %s\n", client_id, buffer);

        // Log the received command
        log_command(client_id, buffer);

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

    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_socket) {
            clients[i] = -1;
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;

    pthread_mutex_init(&client_lock, NULL);
    initialize_tables();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Restaurant Server Started. Port: %d\n", PORT);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    while (1) {
        addr_len = sizeof(client_addr);
        int* new_client_socket = malloc(sizeof(int));

        if ((*new_client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)) < 0) {
            perror("Accept failed");
            free(new_client_socket);
            continue;
        }

        pthread_mutex_lock(&client_lock);
        if (client_count >= MAX_CLIENTS) {
            send_message(*new_client_socket, "Server is at full capacity.\n");
            close(*new_client_socket);
            free(new_client_socket);
        } else {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == -1) {
                    clients[i] = *new_client_socket;
                    client_count++;
                    break;
                }
            }
            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handle_client, new_client_socket);
            pthread_detach(client_thread);
        }
        pthread_mutex_unlock(&client_lock);
    }

    pthread_mutex_destroy(&client_lock);
    close(server_socket);
    return 0;
}
