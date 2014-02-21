/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3510" // the port client will be connecting to 

#define MAXDATASIZE 10000 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char YesorNo[3];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	
	// additions here
	start:
	while(1)
	{	//char cmd; // variable for commands
		//char buf2[MAXDATASIZE]; // variable for command arguements
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		printf(" '%s' \n", buf); // server opening message
		memset(buf,0,sizeof buf); // set buf to 0
		//memset(buf2,0,sizeof buf2); // set buf2 to 0
		//scanf("%s", buf); // scan for command: l for list, c for check, g for get
		char *p;
		char buffer[MAXDATASIZE];
		fgets(buffer,sizeof(buffer),stdin);
		if((p = strchr(buffer, '\n')) != NULL)
			*p = '\0';
		// sending command to server
		
		// command for list
		if(strcmp(buffer,"list") == 0)
		{	send(sockfd,"list",20,0);
			//recv(sockfd, buf, MAXDATASIZE-1, 0);
		}

		// want to check if get is typed
		if( strncmp("get", buffer, 3) == 0 ) // code for get
		{	
			send(sockfd, buffer,20,0);
		}
		
		// want to check if download is typed
		if( strncmp("download", buffer, 8) == 0 ) // code for get
		{	if( (strchr(buffer, '/')) != NULL)
			{	printf("dont snoop with / \n");
				goto start; // this needs to send back so that a command is asked for again
			}
			
			printf("this is the command: %s\n",buffer);
			char d[50];
			sprintf(d,"%s",buffer+9);
			printf("This is the shortened string: %s\n",d);
			
			// check for existence of file
			// THIS IS NOT WORKING, IT DOES NOT LOOK IN THE CORRECT DIRECTORY?
			// It instead checks the current directory the SSH terminal is in?
			if ( access( d, F_OK ) == -1)
			{	printf("file does not exist"); 
				break;	// file does not exist, need to return and ask for another command
			}
			else // file exsists, we need to ask for an overwrite
			{	
				printf("do you want to overwrite? y/n \n");
				scanf("%s", YesorNo);
				if( YesorNo == "y" )
					send(sockfd, buffer,20,0);
				else
					break; // request a new command here
			}
		}

		else
			printf("did not understand command");
	}
	close(sockfd);
	return 0;
}

