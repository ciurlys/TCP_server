#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define IP "0.0.0.0"
#define PORT 8080

int main()
{
	int serverSocket, clientSocket; //File descriptors
		
	socklen_t addresslength = 0; //Needed for accept()

	struct sockaddr_in srv, cli;

	char buf[512];
	char *data;


	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0)
	{
		printf("ERROR: socket returned -1\n");
		return -1;
	}

	srv.sin_addr.s_addr = inet_addr(IP); //Bind to all available intrefaces
	srv.sin_port = htons(PORT);
	srv.sin_family = AF_INET;

	//Returns 0 if it works
	if (bind(serverSocket, (struct sockaddr *)&srv, sizeof(srv)))
	{
		printf("ERROR: bind returned -1\n");
		close(serverSocket);
		return -1;
	}

	//2nd param - queue of simultaneous connections
	if (listen(serverSocket, 10))
	{
		printf("ERROR: listen returned -1\n");
		close(serverSocket);
		return -1;
	} 

	printf("We are listening on %s:%d\n", IP, PORT); 

    clientSocket = accept(serverSocket, (struct sockaddr *)&srv, &addresslength);

	if (clientSocket < 0)
	{
		printf("ERROR: accept returned -1\n");
		close(serverSocket);
		return -1;
	}
	
	printf("Client connected\n");
	
	int n;
	n = read(clientSocket, buf, 511);
	write(1, buf, n);

	data = "Success\n";
	write(clientSocket, data, strlen(data));
	close(clientSocket);
	close(serverSocket);

	return 0;
}
