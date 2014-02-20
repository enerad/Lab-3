/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3510"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints); //make sure the struct is empty
	hints.ai_family = AF_UNSPEC; // dont care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // use my IP

	// servinfo now points to a linked list of 1 or more struct addrinfos
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			/*
			if (send(new_fd, "Hello, world!", 13, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
			*/
			
			// this is where we add stuff
			
			// pipe variables
			int in[2], out[2], n, pid;
			char buf[1000];
			int buff[1000];
			int numbytes;
			char listcompare[10] = "list";
			char checkcompare[10] = "check";
			char getcompare[10] = "get";
			/* Creating two pipes: 'in' and 'out' */ 
			/* In a pipe, xx[0] is for reading, xx[1] is for writing */ 
			if (pipe(in) < 0) error("pipe in");  
			if (pipe(out) < 0) error("pipe out");
			
			// request command text
			commandrecieve:
			if (send(new_fd, "Type in your command (list, check, get):", 50, 0) == -1)
				perror("send");
			// recieve command code
			numbytes = recv(new_fd, buf, 99, 0);
			buf[numbytes] = '\0';
			printf("You entered: '%s' \n", buf);
			
			// now we want to check what command we received
			commandcheck:
			if (strcmp(listcompare,buf) == 0) // code for list
			{	if(!fork()) // this is a child's child process
				{	// close stdin, stdout, stderr
					close(0);
					close(1);
					close(2);
					// make our pipes our new stdin,stdout,stderr
					dup2(in[0],0);
					dup2(out[1],1);
					dup2(out[1],2);
					// close the other ends of the pipes that the parent will use
					close(in[1]);
					close(out[0]);
					
					execl("/usr/bin/ls", "ls",(char *)NULL); // runs ls
					printf("Could not execl ls"); // will print if execl doesnt run
				}
				// child process
				
				// close the pipe ends that the child uses
				close(in[0]);
				close(out[1]);
				
				// code to output execl
				close(in[1]);
				n = read(out[0], buff, 1000);
				buff[n] = '\0'; // reset buff
				printf("This was recieved by the child: %s", buff);
				send(new_fd,buff,1000,0);
				goto commandrecieve;
			}
			else
			{	printf("command not understood");
				goto commandrecieve;
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

