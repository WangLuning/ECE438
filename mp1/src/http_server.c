/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 500
#define BACKLOG 10	 // how many pending connections queue will hold
char img_path[500][1000];
int img_num=0;
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

int main(int argc,char*argv[])
{
	int sockfd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	int recvsize;
	char recvingbuf[MAXDATASIZE];
	char*recvbuf=recvingbuf;

	if(argc!=2){fprintf(stderr,"host port");exit(1);}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
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
		int new_fd;
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
			recvsize=recv(new_fd,recvbuf,MAXDATASIZE-1,0);
			printf("%s\n",recvbuf);
			
			//extract hostname and URL from the message
			char extract[1000];
			char extract2[1000];
			int count=0;
			int idx=0;
			int idx2=0;
			int numofzero=0;
			int numofret=0;
			
			if(recvbuf[0]!='G'||recvbuf[1]!='E'||recvbuf[2]!='T')
			  {
				printf("%c %c %c",recvbuf[0],recvbuf[1],recvbuf[2]);
				send(new_fd,"400 Bad Request",15,0);exit(0);
			  }			
				
			for(count=0;count<strlen(recvbuf);++count){
			  if(recvbuf[count]==' '){numofzero++;continue;}
			  if(recvbuf[count]=='\r'){numofret++;numofzero=0;continue;}
			  if(numofzero==1&&numofret==0){
				if(recvbuf[count]=='\r'||recvbuf[count]==' ')break;
				extract[idx]=recvbuf[count];idx++;}
			  if(numofzero==1&&numofret==2){
				if(recvbuf[count]=='\r'||recvbuf[count]==' ')break;
				extract2[idx2]=recvbuf[count];idx2++;}
			}
			char*extracts=extract;
			char*extracts2=extract2;
			printf("URL: %s\n",extracts);
			printf("Hostname: %s\n",extracts2);
			strcat(extracts2,extracts);
			printf("all URL: %s\n",extracts2);

			//But we only need URL as extracts;
			int numsent;
			char openpath[100];
			strncpy(openpath,extracts+1,strlen(extracts));
			printf("%s\n",openpath);
			FILE*pfile=fopen(openpath,"r");
			char line[500];
			if(pfile==NULL){
			  printf("HTTP/1.1 404 Not Found\r\n");
			  send(new_fd,"HTTP/1.1 404 Not Found\r\n",24,0);
			  
			}
			else{
			  send(new_fd,"HTTP/1.1 200 OK\r\n\r\n",19,0);
			  while(fgets(line,499,pfile)!=NULL){
			    line[strlen(line)]='\0';
			    send(new_fd,line,strlen(line),0);
		            printf("numof bytes sent:%d\n",strlen(line));
			    printf("%s\n",line);
			  }
			  fclose(pfile);
			
			}
		}
		close(new_fd);
	}

	return 0;
}

