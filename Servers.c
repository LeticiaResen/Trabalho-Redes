#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

int equipament_ids[MAX_CLIENTS];
int number_equipament = 0;

int verify_client(int id)
{
	for (int i = 0; i < number_equipament; i++)
	{
		if (equipament_ids[i] == id)
		{
			return 1;
		}
	}
	return 0;
}

void disconect_client(int id)
{
	for (int i = 0; i < number_equipament; i++)
	{
		if (equipament_ids[i] == id)
		{
			// Shift the remaining IDs to fill the gap
			for (int k = i; k < number_equipament - 1; k++)
			{
				equipament_ids[k] = equipament_ids[k + 1];
			}
			number_equipament--;
			break;
		}
	}
}

void broadcast(char* message){
	for(int i=0; i<number_equipament; i++){
		send(equipament_ids[i],message,256,0);
	}
}

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
			perror("select error");
		}

		// If the server socket has a new connection, accept it
		if (FD_ISSET(server_fd, &readfds))
		{
			addrlen = sizeof(address);
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
					memset(buffer, 0, BUFFER_SIZE);
					buffer[0] = 11; // Type of the message (Id Msg) - ERROR
					buffer[1] = 4;	// Number of the error - 04
					// Sending the error message
					send(new_socket, buffer, 2, 0);
					close(new_socket);
				}
				else
				{
					printf("Equipment IdEq%d added\n", new_socket);
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
					// Creating the RES_ADD
					memset(buffer, 0, BUFFER_SIZE);
					buffer[0] = 7;			// Type of the message (Id Msg) - RES_ADD
					buffer[1] = new_socket; // Number of the equipment

					// Save the equipament ID
					equipament_ids[number_equipament++] = new_socket;

					// Broadcast the new connection
					for (i = 0; i < max_clients; i++)
					{
						if (client_fds[i] != 0)
						{
							// Send RES_ADD
							int n = send(client_fds[i], buffer, 2, 0);
							if (n <= 0)
							{
								printf("Erro ao enviar RES_ADD");
							}
							else
							{
								printf("RES_ADD enviado com sucesso para %d \n", client_fds[i]);
							}
						}
					}
					// Send RES_LIST for the new equipament
					memset(buffer, 0, BUFFER_SIZE);
					buffer[0] = 8; // Type of the message (Id Msg) - RES_LIST
					// printf("Tenho %d equipamentos\n",number_equipament);
					for (int i = 0; i < number_equipament; i++)
					{
						// printf("i: %d  Number equipament: %d \n",i,equipament_ids[i]);
						buffer[i + 1] = equipament_ids[i];
						// printf("%d\n",buffer[i+1]);
					}
					send(new_socket, buffer, 256, 0);
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

					// Remove the disconnected equipament ID
					for (int j = 0; j < number_equipament; j++)
					{
						if (equipament_ids[j] == sd)
						{
							// Shift the remaining IDs to fill the gap
							for (int k = j; k < number_equipament - 1; k++)
							{
								equipament_ids[k] = equipament_ids[k + 1];
							}
							number_equipament--;
							break;
						}
					}
				}
				else
				{
					// Echo back the received message
					buffer[valread] = '\0';
					printf("Received message from client %d: %s\n", sd, buffer);
					if (buffer[0] == 6)
					{
						int num_eq = buffer[1];
						// Verify if its a valid client
						if (verify_client(num_eq))
						{
							// Remove from equipaments list
							disconect_client(num_eq);
							// Send the OK message
							memset(buffer, 0, BUFFER_SIZE);
							buffer[0] = 12; // Type of the message (Id Msg) - OK
							send(num_eq, buffer, 256, 0);
							// Print the message
							printf("Equipament IdEq%d removed\n", num_eq);
							printf("Number of clients: %d\n", number_equipament);
							// Broadcast REQ_REM
							memset(buffer, 0, BUFFER_SIZE);
							buffer[0] = 6; // Type of the message (Id Msg) - REM
							buffer[1] = num_eq; //Num eq
							broadcast(buffer);
						}
						else
						{
							memset(buffer, 0, BUFFER_SIZE);
							buffer[0] = 11; // Type of the message (Id Msg) - ERROR
							buffer[1] = 1;  // Number of error
							send(num_eq, buffer, 256, 0);
						}
					}
				}
			}
		}
	}

	return 0;
}
