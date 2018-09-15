/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>


#define BUFSIZE 2000
#define HEADER_SIZE 50
#define DEBUG 0
#define MAXWINDOW 350

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


    /* Now receive data and send acknowledgements */
    remove(destinationFile);
    //int file_fd = open(destinationFile, O_WRONLY | O_CREAT, 777);
	FILE* fp=fopen(destinationFile,"wb");
	if (fp ==NULL) {
        fprintf(stderr, "FILE NOT OPENDED\n");
        return;
    }

    char buf[BUFSIZE];
	
	///////////////////////////////////////////////////////////////////
	char charNotInOrder[BUFSIZE*300]; //not in order messages
	int indexOutOfOrder[300];
	int beginOutOfOrder[300];
	int endOutOfOrder[300];
	
	int frameNumber = 0;
	int sequenceNumber = 0;
	////////////////////////////////////////////////////////////////////
	
    struct sockaddr_in sender_addr; //the addr to send back ack
    socklen_t addrlen = sizeof(sender_addr);
    int lastAckSent = -1;

    int recvbytes = 0;

    while (1) {
        recvbytes = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr*)&sender_addr, &addrlen);
        if (recvbytes < 0) {
            fprintf(stderr, "Connection closed\n");
            break;
        }

        buf[recvbytes] = '\0';

        if (!strncmp(buf, "EOT", 3)) {
            fprintf(stderr, "end of transmission\n");
            break;
        }
        else {  //this is the package we should receive
            char header[HEADER_SIZE]; //put the header in it as "ack n"
            int k;
            char* curr;

            for (curr = buf, k = 0; *curr != ';' && k < BUFSIZE; curr++, k++) {
                header[k] = *curr;
            }

            curr++;     //pointing to first bit of real message
            header[k] = '\0';
            int frameIndex;
            sscanf(header, "frame%d", &frameIndex);

            if (frameIndex == lastAckSent + 1) {
				printf("success getting current one\n");
                lastAckSent++;
				fwrite(curr, 1, recvbytes - k - 1, fp);
				
				while(1){// how many can be eliminated from buffer
					int i;
					int storedlen = 0;
					for(i=0;i<frameNumber;++i){
						if(indexOutOfOrder[i]==lastAckSent + 1){
							printf("getting one from buffer\n");
							lastAckSent++;
							storedlen = endOutOfOrder[i]-beginOutOfOrder[i];
							fwrite(&charNotInOrder[beginOutOfOrder[i]], 1, storedlen, fp);
							for(int k=endOutOfOrder[i];k<sequenceNumber;++k){
								charNotInOrder[k-storedlen] = charNotInOrder[k];
							}
							for(int k=i+1;k<frameNumber;++k){
								beginOutOfOrder[k-1]=beginOutOfOrder[k];
								endOutOfOrder[k-1]=endOutOfOrder[k];
								indexOutOfOrder[k-1]=indexOutOfOrder[k];
							}
							break;
						}
					}
					if(i==frameNumber)break;
					else{
						sequenceNumber -= storedlen;
						frameNumber--;
					}
				
				}
				
				char ack[HEADER_SIZE];
                sprintf(ack, "ack%d;", lastAckSent); //+1 stored has been used

                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & sender_addr, sizeof(sender_addr));
                
            } 
            else if(frameIndex <= lastAckSent){
				printf("getting an old ack\n");
                char ack[HEADER_SIZE];
                sprintf(ack, "ack%d;", frameIndex); //frameIndex
                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & sender_addr, sizeof(sender_addr));
            }
			else {
				printf("future new ack stored\n");
				char ack[HEADER_SIZE];
				sprintf(ack, "ack%d;", frameIndex);
                sendto(s, ack, strlen(ack), 0, (struct sockaddr *) & sender_addr, sizeof(sender_addr));
				
				//if it is out of order then store it
				indexOutOfOrder[frameNumber] = frameIndex;
				beginOutOfOrder[frameNumber] = sequenceNumber;
				int i;
				for(i=0; i<recvbytes-k-1;++i){
					charNotInOrder[sequenceNumber] = *curr;
					curr++;
					sequenceNumber++;
				}
				endOutOfOrder[frameNumber] = sequenceNumber;
				frameNumber++;
			}

            memset(buf, 0, BUFSIZE);//buf reads in a packet every time

        }
    }

    close(s);
    fclose(fp);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

