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
		// Clear the buffer
		memset(buffer, 0, BUFFER_SIZE);

		// // Wait for user input
		// printf("Enter a message: ");
		// fgets(buffer, BUFFER_SIZE, stdin);

		// // Send message to the server
		// send(sock, buffer, strlen(buffer), 0);

		// Receive the response from the server
		memset(buffer, 0, BUFFER_SIZE);
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
				printf("Error: Equipament not found.\n");
				break;
			case 2:
				printf("Error: Source equipament not found.\n");
				break;
			case 3:
				printf("Error: Target equipament not found.\n");
				break;
			case 4:
				printf("Error: Equipament limit exceeded.\n");
				break;
			case 6:
				printf("Error: Peer limit exceed.\n");
				break;
			case 7:
				printf("Error: Peer not found.\n");
				break;
			}
		}
		if (msg_type == 7) // It's a RES_ADD
		{
			// Find Out the number of the new equipament
			int eq_code = buffer[1];
			// Print the message
			if (number_equipament == 0)
			{
				equipament_ids[0] = eq_code;
				printf("New ID: %d \n", eq_code);
			}
			else
			{
				equipament_ids[number_equipament++] = eq_code;
				printf("Equipament IdEq %d added\n", eq_code);
			}
		}
		if (msg_type == 8){ // It's a RES_LIST
			for (int i = 1; i < 16; i++)
			{
				if (buffer[i]!=0)
				{
					//printf("Numero a ser salvo %d\n", buffer[i]);
					equipament_ids[i]= buffer[i];
					number_equipament++;
				}
			}
				//printf("Number equipaments %d\n",number_equipament);
			for(int i=0; i<number_equipament; i++){
				//printf("Recebi a lista e registrei o equipamentos %d no i: %d\n",equipament_ids[i], i);
			}
		}
		// printf("Server response: %s\n", buffer);
	}

	// Close the socket
	close(sock);

	return 0;
}
