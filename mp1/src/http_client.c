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
//@@change the port
#define PORT "80" // the port client will be connecting to 

#define MAXDATASIZE 500 // max number of bytes we can get at once 

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
	static char buf[1000];
	char recvingbuf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}
        //@@extract URL find the place of '/' and ':'
	int size=strlen(argv[1]);
	
	int line=0;
	int i;
	int beg=0;//where does URL path begin
	int which=0;
	int colon=0;
	int numofcolon=0;
	for(i=0;i<size;++i){
		if(argv[1][0]!='h'&&argv[1][i]==':')colon=i;
		if(argv[1][0]=='h'&&argv[1][i]==':'){numofcolon++;}
		if(argv[1][0]=='h'&&numofcolon==2){colon=i;numofcolon++;}
		if(argv[1][i]=='/')line++;
		if(argv[1][0]!='h'&&line==1){beg=i;//if begin with www
			which=0;break;}
		if(argv[1][0]=='h'&&line==3){beg=i;which=1;
			break;}//if begin with http
	}

	//find the assigned port number
	char portnumber[10]="";
	printf("new variable size %d\n",strlen(portnumber));
	if(colon!=0){
	  for(i=colon+1;i<size;++i){
	    if(argv[1][i]=='/')break;
	    portnumber[i-colon-1]=argv[1][i];
	  }
	  printf("Port number if assigned: %s\n",portnumber);  
	}
	//printf("%d number of begin\n",beg);
	char part2[size-beg+1];
	char part1[beg];
	part2[size-beg]='\0';

	for(i=beg;i<size;++i){
		part2[i-beg]=argv[1][i];
	}

	if(which==0){//begin with www
	  if(colon==0){
	    printf("www, no colon\n");
	    for(i=0;i<beg;++i){
		part1[i]=argv[1][i];
	    }
	    part1[beg]='\0';
	  }
	  else{
	    printf("www, colon\n");
	    for(i=0;i<colon;++i)part1[i]=argv[1][i];
	    part1[colon]='\0';
	  }
	}

	if(which==1){//if begin with http
	  if(colon==0){
	    printf("http, no colon\n");
	    for(i=7;i<beg;++i){part1[i-3]=argv[1][i];}
	    part1[beg-3]='\0';
	  }
	  else {
	    printf("http, colon\n");
	    printf("beg %d colon %d\n",beg,colon);
	    for(i=7;i<colon;++i){part1[i-3]=argv[1][i];}
	    part1[colon-3]='\0';
	  }
	  part1[0]='w';part1[1]='w';part1[2]='w';part1[3]='.';
	}
		
	//@@set the format of http information
	sprintf(buf,"GET ");
	strcat(buf,part2);
	strcat(buf," HTTP/1.1\r\n");
	strcat(buf,"User-Agent: Wget/1.12 (linux-gnu)\r\n");
	strcat(buf,"Host: ");
	strcat(buf,part1);
	strcat(buf,"\r\n");
	strcat(buf,"Connection: Keep-Alive\r\n\r\n");
	
	printf("Hostname: %s\n",part1);
	printf("URL: %s\n",part2);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	//use the default port or extracted port?
	int portsize=strlen(portnumber);
	char chooseport[10];
	if(portsize!=0){
	  printf("The size of assigned port: %d\n",portsize);
	  for(i=0;i<portsize;++i)chooseport[i]=portnumber[i];
	  chooseport[portsize]='\0';
	}
	else {
	  printf("use default port.\n");
	  chooseport[0]='8';chooseport[1]='0';chooseport[2]='\0';
	}
	printf("Decided port: %s\n",chooseport);

	if ((rv = getaddrinfo(part1, chooseport, &hints, &servinfo)) != 0) {
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
	
	//@@how to send a http request

	send(sockfd,(char*)buf,sizeof(buf),0);

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	//@@receive the message
	FILE *fp;
	fp=fopen("output","w");
	while(1){
		if((numbytes=recv(sockfd,recvingbuf,MAXDATASIZE-1,0))>0){
			fprintf(fp,"%s",recvingbuf);
			//printf("%s",recvingbuf);
			printf("num in line: %d\n",numbytes);
		}
		else {fclose(fp);break;}
	}
	close(sockfd);

	return 0;
}
