#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int equipament_ids[15];
int number_equipament = 0;

void list_equipament()
{
	printf("Equipamentos conectados: \n");
	for (int i = 0; i < number_equipament; i++)
	{
		printf("ID: %d\n", equipament_ids[i]);
	}
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

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
		return 1;
	}

	int sock;
	struct sockaddr_in server_address;
	char buffer[BUFFER_SIZE];

	// Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[2]));

	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0)
	{
		perror("invalid address / address not supported");
		exit(EXIT_FAILURE);
	}

	// Connect to the server
	if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("connection failed");
		exit(EXIT_FAILURE);
	}

	fd_set readfds;
	int max_sd;

	// Enviando o REQ_ADD
	memset(buffer, 0, 256);
	buffer[0] = 5; // Tipo da mensagem (Id Msg) - REQ_ADD
	int n = send(sock, buffer, 256, 0);
	if (n < 0)
	{
		printf("ERROR writing to socket");
	}
	printf("REQ_ADD sent\n");

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(sock, &readfds);
		max_sd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

		// Wait for activity on any of the file descriptors
		if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			perror("select error");
			exit(EXIT_FAILURE);
		}

		// Check if there is a message from the server
		if (FD_ISSET(sock, &readfds))
		{
			// Clear the buffer
			memset(buffer, 0, BUFFER_SIZE);

			// Receive the response from the server
			recv(sock, buffer, BUFFER_SIZE, 0);

			// Verify the type of the message
			int msg_type = buffer[0];

			if (msg_type == 11)
			{
				// Caso for um erro, pegar o código do erro
				int code = buffer[1];
				// Imprimir a mensagem de acordo com o código
				switch (code)
				{
				case 1:
					printf("Error: Equipment not found.\n");
					break;
				case 2:
					printf("Error: Source equipment not found.\n");
					break;
				case 3:
					printf("Error: Target equipment not found.\n");
					break;
				case 4:
					printf("Error: Equipment limit exceeded.\n");
					break;
				case 6:
					printf("Error: Peer limit exceeded.\n");
					break;
				case 7:
					printf("Error: Peer not found.\n");
					break;
				}
			}
			else if (msg_type == 7) // It's a RES_ADD
			{
				// Find out the number of the new equipment
				int eq_code = buffer[1];

				// Print the message
				if (number_equipament == 0)
				{
					equipament_ids[0] = eq_code;
					printf("New ID: %d \n", eq_code);
					number_equipament++;
				}
				else
				{
					equipament_ids[number_equipament++] = eq_code;
					printf("Equipment IdEq %d added\n", eq_code);
				}
			}
			else if (msg_type == 8)
			{ // It's a RES_LIST
				printf("Recebi RES_LIST\n");
				for (int i = 1; i < 16; i++)
				{
					if (buffer[i] != 0)
					{
						equipament_ids[i] = buffer[i];
						number_equipament++;
					}
				}
			}
			else if (msg_type == 12)
			{
				printf("Sucess removal\n");
				close(sock);
			}
			else if (msg_type == 6)
			{
				int num_eq = buffer[1];
				disconect_client(num_eq);
				printf("Equipament IdEq%d removed\n", num_eq);
			}
			else
			{
				printf("Mensagem desconhecida\n");
			}
		}

		// Check if there is a message from the user
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			// Clear the buffer
			memset(buffer, 0, BUFFER_SIZE);

			// Read the message from the user
			fgets(buffer, BUFFER_SIZE, stdin);

			// Verify whats is the message
			if (strncmp(buffer, "close connection", 16) == 0)
			{
				memset(buffer, 0, BUFFER_SIZE);
				buffer[0] = 6;				   // Id message
				buffer[1] = equipament_ids[0]; // number of id
				// Send message to the server
				send(sock, buffer, strlen(buffer), 0);
			}
			if (strncmp(buffer, "list equipament", 15) == 0)
			{
				list_equipament();
			}
		}
	}

	// Close the socket
	close(sock);

	return 0;
}
