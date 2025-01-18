#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define LISTENING_ADDRESS "127.0.0.1"

/* STRUCTS */
struct sHttpRequest{
	char method[8];
	char route[128];
};
typedef struct sHttpRequest httpreq;

/* GLOBAL VARIABLES */
char *error;


int server_init(int);
int client_accept(int s);
httpreq *parse_http(char*);
void client_connection(int);
char *client_read(int);
int main(int argc, char *argv[])
{
	int serverSocket;
	int clientSocket;
	char *port;

	if (argc < 2)
	{
		fprintf(stderr, "No port specified. Start like so: %s <listening port>\n", argv[0]);
		return -1;
	}
	else
	{
		port = argv[1];
	}


	serverSocket = server_init(atoi(port));

	if (serverSocket < 0)
	{
		fprintf(stderr, "%s\n", error);
		return -1;
	}

	printf("Listening on %s:%s\n", LISTENING_ADDRESS, port);
	while (1)
	{
		clientSocket = client_accept(serverSocket);	
		if (clientSocket < 0)
		{
			fprintf(stderr, "%s\n", error);
			continue;
		}

		printf("Incoming connection\n");

		//New process returns 0, the main has its PID
		if(!fork())
			client_connection(clientSocket);
	}

	return -1;
}

/*returns -1 on error. On sucess returns a server socket file descriptor */
int server_init(int port)
{
	int sock;
	struct sockaddr_in srv;


	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		error = "socket() failed";
		return -1;
	}

	srv.sin_addr.s_addr = inet_addr(LISTENING_ADDRESS);
	srv.sin_port = htons(port);
	srv.sin_family = AF_INET;


	if (bind(sock, (struct sockaddr *)&srv, sizeof(srv)))
	{
		error = "bind() failed";
		close(sock);
		return -1;
	}

	if (listen(sock, 5))
	{
		error = "listen() failed";
		close(sock);
		return -1;
	}

	return sock;
}


/* returns -1 on error. On success returns a client socket file descriptor */
int client_accept(int serverSock)
{
	int client;
	socklen_t addrlen = 0;
	struct sockaddr_in cli;
	
	memset(&cli, 0, sizeof(cli));

	client = accept(serverSock, (struct sockaddr *)&cli, &addrlen);
	if (client < 0)
	{
		error = "accept() failed";
		return -1;
	}

	return client;
}
/* returns NULL on error. On success returns a httpreq struct*/

httpreq *parse_http(char* str)
{
	httpreq *req = (httpreq*)malloc(sizeof(httpreq));
	memset(req, 0, sizeof(httpreq));
	char *pointer;

	for (pointer = str; *pointer && *pointer !=' '; pointer++);
	if (*pointer == ' ')
		*pointer = 0;
	else
	{
		error = "Failed to parse HTTP request (method)";
		free(req);
		return NULL;
	}

	strncpy(req->method, str, 7);

	for (str = ++pointer; *pointer && *pointer != ' '; pointer++);
	if (*pointer == ' ')
		*pointer = 0;
	else
	{
		error = "Failed to parse HTTP request (route)";
		free(req);
		return NULL;
	}
	
	strncpy(req->route, str, 127);

	return req;
}
void client_connection(int clientSocket)
{
	httpreq *req;
	char *pointer;

	pointer = client_read(clientSocket);

	if (pointer == 0)
	{
		fprintf(stderr, "%s\n", error);
		close(clientSocket);
		return;
	}

	req = parse_http(pointer);	
	if (req == NULL)
	{
		fprintf(stderr, "%s\n", error);
		close(clientSocket);
		return;
	}
	printf("'%s'\n'%s'\n", req->method, req->route);
	free(req);
	close(clientSocket);
	return;
}

/* On error returns 0. On success returns data*/
char *client_read(int clientSocket)
{
	static char buf[1024];
	memset(buf, 0, 1024);
	if (read(clientSocket, buf, 1023) < 0)
	{
		error = "read() failed";
		return 0;
	}
	else
		return buf;


}
