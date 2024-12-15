#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"  
#define SERVER_PORT 8080      
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Client soketini oluştur
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Sunucu adresini ayarla
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_socket);
        return 1;
    }

    // Sunucuya bağlan
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        return 1;
    }

    printf("Connected to the server. Type 'exit' to disconnect.\n");

    // Mesaj al ve gönder döngüsü
    while (1) {
        // Kullanıcıdan giriş al
        printf("You: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Yeni satırı kaldır

   
        // Sunucuya mesaj gönder
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Failed to send message");
            break;
        }

        // Sunucudan yanıt al
        read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';  // Gelen veriyi sonlandır
            printf("Server: %s\n", buffer);
        } else if (read_size == 0) {
            printf("Server disconnected.\n");
            break;
        } else {
            perror("Failed to receive message");
            break;
        }
    }

    close(client_socket);
    return 0;
}
