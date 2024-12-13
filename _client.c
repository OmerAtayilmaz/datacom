#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"  // Sunucunun IP adresi
#define SERVER_PORT 8080       // Sunucunun port numarası
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Windows soketlerini başlat
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    // İstemci soketini oluştur
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Sunucu adresini ayarla
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Sunucuya bağlan
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed. Error: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
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
        if (send(client_socket, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
            printf("Failed to send message. Error: %d\n", WSAGetLastError());
            break;
        }

        // Sunucudan yanıt al
        read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';  // Null-terminate gelen veriyi sonlandır
            printf("Server: %s\n", buffer);
        } else if (read_size == 0) {
            printf("Server disconnected.\n");
            break;
        } else {
            printf("Failed to receive message. Error: %d\n", WSAGetLastError());
            break;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
