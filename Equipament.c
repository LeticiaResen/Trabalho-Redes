#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2]));

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0) {
        perror("invalid address / address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Enter a message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send message to the server
        send(sock, buffer, strlen(buffer), 0);

        // Receive the response from the server
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("recv failed");
            break;
        }

        printf("Server response: %s\n", buffer);
    }

    // Close the socket
    close(sock);

    return 0;
}
