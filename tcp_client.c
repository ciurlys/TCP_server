#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define IP "142.250.203.206" /* www.google.com */
#define PORT 80


int main()
{

	int s; // This will be the return value for the socket. Like a file descriptor(STDIN, STDOUT, STDERR)

	struct sockaddr_in sock; //This is where we put the IP address etc.
	char buf[512];
	char *data;

	data = "HEAD / HTTP/1.0\r\n\r\n";
	
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		printf("ERROR: socket() returned -1\n");
		return -1;
	}


	sock.sin_addr.s_addr = inet_addr(IP); //From little endian to big endian
	sock.sin_port = htons(PORT);
	sock.sin_family = AF_INET;


	if(connect(s, (struct sockaddr *)&sock, sizeof(struct sockaddr_in)) !=0){
		printf("ERROR: connect() returned -1");
		close(s);
		return -1;
	}

	write(s, data, strlen(data));

	memset(buf, 0, 512);
	read(s, buf, 512);

	close(s);

	printf("\n%s\n", buf);

}
