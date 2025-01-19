#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LISTENING_ADDRESS "127.0.0.1"

/* STRUCTS */
struct sHttpRequest{
	char method[8];
	char route[128];
};
typedef struct sHttpRequest httpreq;

struct sFile{
	char filename[64];
	char *fileContent;
	int size;
};
typedef struct sFile File;

/* GLOBAL VARIABLES */
char *error;

/* DECLARATIONS OF FUNCTIONS*/
int server_init(int);
int client_accept(int s);
httpreq *parse_http(char*);
void client_connection(int);
char *client_read(int);
void http_header(int,int);
void http_response(int,char*,char*);
int sendfile(int,char*,File*);
File *readfile(char*);

/* MAIN */
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

/* FUNCTIONS */


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

	printf("Method: %s\nRoute: %s\n", req->method, req->route);	

	return req;
}
void client_connection(int clientSocket)
{
	httpreq *req;
	char *pointer;
	char filename[96];
	File *file;
	
	char *res;

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

	if (!strcmp(req->method, "GET") && !strncmp(req->route, "/img/", 5))
	{	
		memset(filename, 0, 96);
		snprintf(filename, 95, ".%s", req->route);		
	
		printf("OPENING: %s\n", filename);
		file = readfile(filename);
		if (!file)
		{	
			res = "File not found";
			http_header(clientSocket, 404);
		    http_response(clientSocket, "text/plain", res);
		}
		else
		{
			http_header(clientSocket, 200);
			if (!sendfile(clientSocket, "image/png", file))
			{
				res = "Server error";
				// http_header(clientSocket, 500);
			    http_response(clientSocket, "text/plain", res);			
			}
		}

	} 	
	else if (!strcmp(req->method, "GET") && !strcmp(req->route, "/home")) 
	{

		res = "<img src=\"img/test.png\"/><h1>Give me that man that is not passion's slave and I will wear him in my heart's core, ay, in my heart of heart.</h1>";
		http_header(clientSocket, 200);
		http_response(clientSocket, "text/html", res);
	}
	else
	{
		res = "<h1>404: Habe nun, ach! Philosophie, Juristerei und Medizin, und leider auch Theologie durchaus studiert, mit heissem Bemuehn</h1>";
		http_header(clientSocket, 404);
		http_response(clientSocket, "text/html", res);
	}	



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
void http_header(int clientSocket, int statusCode)
{
	char buf[1024];
	memset(buf, 0, 1024);	
	int length;
	snprintf(buf, 1023,
		"HTTP/1.0 %d OK\n"
		"Server: httpd.c\n"
		"Cache-Control: no-store, no-cache, max-age=0, private\n"
		"Content-Language: en\n"
		"Expires = -1\n"
		"X-Frame-Options: SAMEORIGIN\n", statusCode);

	//We do not set Content-Type nor Length here	
	//They are set in the function http_response()

	length = strlen(buf);
	write(clientSocket, buf, length);
	return;
}

void http_response(int clientSocket,char* contentType, char* data)
{
	char buf[1024];
	int length;
	int contentLength = strlen(data);
	memset(buf, 0, 1024);

	snprintf(buf, 1023,
		"Content-Type: %s\n"
		"Content-Length: %d\n"
		"\n%s\n", contentType, contentLength, data); 
	length = strlen(buf);
	write(clientSocket, buf, length);
	return;
}
/*On error returns 0. On successfully sending file returns 1*/
int sendfile(int clientSocket, char* contentType, File *file)
{
	char buf[512];
	memset(buf, 0, 512);
	char *pointer;

	int bytesLeft = file->size;
	int bytesWritten;
	
	if (!file)
		return 0;

	snprintf(buf, 512,
		"Content-Type: %s\n"
		"Content-Length: %d\n\n",
		contentType, file->size); 
	
	
	int	length = strlen(buf);
	write(clientSocket, buf, length);

	pointer = file->fileContent;
	while (1)
	{
		bytesWritten = write(clientSocket, pointer, (bytesLeft < 512) ? bytesLeft: 512);
		if (bytesWritten < 1)
			return 0;		

		bytesLeft -= bytesWritten;
		if (bytesLeft < 1)
			break;
		pointer += bytesWritten;
	}

	return 1;

}
/*On error returns 0. On success returns File struct*/
File *readfile(char *filename)
{
	char buf[512];
	char *pointer;

	int bytesSum = 0;
	int bytesRead = 0;

	int fileDescriptor;
	File *file;	

	fileDescriptor = open(filename, O_RDONLY);
	if (fileDescriptor < 0)
		return 0;

	file = malloc(sizeof(struct sFile));	
	if (file == 0)
	{
		close(fileDescriptor);
		return 0;
	}
	
	strcpy(file->filename, filename);
	file->fileContent = malloc(512);

	while (1)
	{
		memset(buf, 0, 512);	
		bytesRead = read(fileDescriptor, buf, 512);	
		if (bytesRead == 0)
			break;
		else if (bytesRead == -1)
		{
			close(fileDescriptor);
			free(file->fileContent);
			free(file);
			
			return 0;	
		}
		memcpy(file->fileContent + bytesSum, buf, bytesRead);
		bytesSum += bytesRead;
		file->fileContent = realloc(file->fileContent, (512 + bytesSum));

	}
	
	file->size = bytesSum;
	close(fileDescriptor);

	return file;
}
