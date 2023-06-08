#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Usage: %s <server_port>\n", argv[0]);
		return 1;
	}

	int server_fd, client_fds[MAX_CLIENTS], max_fd, activity, i, valread, new_socket, sd;
	int max_clients = MAX_CLIENTS;
	int client_count = 0;
	socklen_t addrlen;
	struct sockaddr_in address;
	char buffer[BUFFER_SIZE];

	// Set up server socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(argv[1]));

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, max_clients) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// Initialize client file descriptor set and set server socket as the highest file descriptor
	fd_set readfds;
	for (i = 0; i < max_clients; i++)
	{
		client_fds[i] = 0;
	}
	max_fd = server_fd;

	while (1)
	{
		FD_ZERO(&readfds);

		FD_SET(server_fd, &readfds);
		for (i = 0; i < max_clients; i++)
		{
			sd = client_fds[i];

			if (sd > 0)
			{
				FD_SET(sd, &readfds);
			}

			if (sd > max_fd)
			{
				max_fd = sd;
			}
		}

		// Wait for an activity on one of the sockets using select
		activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		// If the server socket has a new connection, accept it
		if (FD_ISSET(server_fd, &readfds))
		{
			addrlen = sizeof(address); // Definir tamanho de addrlen

			if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			else
			{
				if (client_count >= max_clients)
				{
					printf("Maximum connections reached. Rejecting new connection.\n");
					// Building message ERROR(04)
					memset(buffer, 0, 256);
					buffer[0] = 11; // Type of the message (Id Msg) - ERROR
					buffer[1] = 4;	// Number of the error - 04
					// Sending the error message
					send(new_socket, buffer, 256, 0);
					close(new_socket);
				}
				else
				{
					printf("New connection, socket fd is %d, IP is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

					// Add new socket to the array of client sockets
					for (i = 0; i < max_clients; i++)
					{
						if (client_fds[i] == 0)
						{
							client_fds[i] = new_socket;
							client_count++;
							break;
						}
					}
				}
			}
		}

		// Check for IO operations on other sockets (data from clients)
		for (i = 0; i < max_clients; i++)
		{
			sd = client_fds[i];

			if (FD_ISSET(sd, &readfds))
			{
				if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0)
				{
					// Client has disconnected
					getpeername(sd, (struct sockaddr *)&address, &addrlen);
					printf("Host disconnected, IP: %s, Port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

					close(sd);
					client_fds[i] = 0;
					client_count--;
				}
				else
				{
					// Echo back the received message
					buffer[valread] = '\0';
					send(sd, buffer, strlen(buffer), 0);
				}
			}
		}
	}

	return 0;
}
