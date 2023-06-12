#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <ip> <peer_port> <client_port>\n", argv[0]);
        exit(1);
    }

    const char* ip = argv[1];
    int peer_port = atoi(argv[2]);
    int client_port = atoi(argv[3]);

    // Create peer socket
    int peer_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (peer_sockfd < 0) {
        perror("Failed to create peer socket");
        exit(1);
    }

    // Enable SO_REUSEADDR option for peer socket
    int optval = 1;
    if (setsockopt(peer_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Failed to set SO_REUSEADDR option for peer socket");
        exit(1);
    }

    // Bind peer socket to IP and peer port
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(ip);
    peer_addr.sin_port = htons(peer_port);

    if (bind(peer_sockfd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("Failed to bind peer socket");
        exit(1);
    }

    // Start listening for connections on peer socket
    if (listen(peer_sockfd, 1) < 0) {
        perror("Failed to listen on peer socket");
        exit(1);
    }

    printf("Server is listening on %s:%d (peer)\n", ip, peer_port);

    // Create client socket
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("Failed to create client socket");
        exit(1);
    }

    // Enable SO_REUSEADDR option for client socket
    if (setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Failed to set SO_REUSEADDR option for client socket");
        exit(1);
    }

    // Bind client socket to IP and client port
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ip);
    client_addr.sin_port = htons(client_port);

    if (bind(client_sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("Failed to bind client socket");
        exit(1);
    }

    printf("Server is listening on %s:%d (client)\n", ip, client_port);

    // Connect to peer server
    struct sockaddr_in peer_server_addr;
    memset(&peer_server_addr, 0, sizeof(peer_server_addr));
    peer_server_addr.sin_family = AF_INET;
    peer_server_addr.sin_addr.s_addr = inet_addr(ip);
    peer_server_addr.sin_port = htons(peer_port);

    if (connect(client_sockfd, (struct sockaddr*)&peer_server_addr, sizeof(peer_server_addr)) < 0) {
        printf("No peer found, starting to listen..\n");
    } else {
        printf("Connected to peer server\n");
    }

    // Close client socket
    close(client_sockfd);

    // Set of socket descriptors
    fd_set readfds;
    int max_sd, activity;

    // Array to store client socket descriptors
    int client_sockets[MAX_CLIENTS] = {0};

    // Initialize all client sockets to 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Server socket descriptors
    int peer_sd = peer_sockfd;

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to the set
        FD_SET(peer_sd, &readfds);
        max_sd = peer_sd;

        // Add client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on any of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Failed to select sockets for activity");
            exit(1);
        }

        // If there is incoming connection on the peer socket
        if (FD_ISSET(peer_sd, &readfds)) {
            struct sockaddr_in peer_client_addr;
            socklen_t peer_client_addrlen = sizeof(peer_client_addr);

            // Accept the incoming connection
            int peer_client_sockfd = accept(peer_sd, (struct sockaddr*)&peer_client_addr, &peer_client_addrlen);
            if (peer_client_sockfd < 0) {
                perror("Failed to accept incoming connection");
                exit(1);
            }

            printf("Accepted incoming connection from peer server\n");
            // Process the request from the peer server and send the response
            // ...

            // Close the connection
            close(peer_client_sockfd);
        }
    }

    // Close peer socket
    close(peer_sockfd);

    return 0;
}
