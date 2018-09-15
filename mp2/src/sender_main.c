#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

#define BUFSIZE 2000
#define HEADER_SIZE 50
#define MAXWINDOW 400
#define SLOW_GROWTH 8
#define SLOW_DECLINE 8
#define PACKET_SIZE 1472-HEADER_SIZE
#define DEBUG 1

struct sockaddr_in si_other;
int s, slen;
int rtt;
int counter = 0;
int dupAck = 0;
double alpha = 0.8;
int size = 1472;

int lastIndex = -1;
int lastBytes = -1;
int timeoutCount = 0;
double slowGrowth = 0.0;
long long int totalReadin = 0;
long long int bytesAlreadyRead = 0;

unsigned int expectedAck = 0;
unsigned int frameCount = 0;
unsigned long long totalBytes = 0;
unsigned long long MAXBYTES = 0;
// int firstFrame = 0;

int fileRead[MAXWINDOW];
char msgWindow[MAXWINDOW][PACKET_SIZE + HEADER_SIZE];
int msgFrame[MAXWINDOW];
int ackCount[MAXWINDOW];
int sizeFrame[MAXWINDOW];

int frameSent = 0;

FILE* fp;

/** Window variables */
int canSend = 1;

struct addrinfo *p;

void diep(char *s) {
    perror(s);
    exit(1);
}

// get sockaddr, IPv4 or IPv6:
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer);
int timeDiff(struct timeval a, struct timeval b);
void setSockTimeout(struct timeval timer);
int createSocket(char * hostname, unsigned short int hostUDPport);

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	unsigned long long int numBytes;
	
	if(argc != 5)
	{
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);
	
	reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
    return 0;
} 

// int totalCalls = 0;
// int totalCanSends = 0;

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer)
{
	totalReadin = bytesToTransfer;
	
	fp = fopen(filename, "rb");
	if (fp ==NULL) {
		fprintf(stderr, "File not opened\n");
		return;
	}
	
	struct sockaddr_in ret_addr;
	socklen_t addrlen = sizeof(ret_addr);
	
	s = createSocket(hostname, hostUDPport);

	memset(fileRead, 0, MAXWINDOW * sizeof(int));
	memset(msgWindow, 0, MAXWINDOW * (PACKET_SIZE + HEADER_SIZE));
	memset(msgFrame, 0, MAXWINDOW * sizeof(int));
    memset(sizeFrame, 0, MAXWINDOW * sizeof(int));

	int framePointer = 0;
	char buf[BUFSIZE];
	expectedAck = 0;
	rtt = 20 * 1000; // 20 milliseconds
    // canSend = 1;
	while (1) {

		// All the timers
		struct timeval RTT_TO;
		RTT_TO.tv_sec = 0;
		RTT_TO.tv_usec = 2 * rtt;

		
		struct timeval SENT, ACK_RECV, LATEST, LEFTOVER;
		gettimeofday(&SENT, 0); // When we sent the initial packet
		int expectedAcks = sendFiles(framePointer);

		int timeoutFlag = 0;
		setSockTimeout(RTT_TO);

		if (expectedAcks == 0) {
			fprintf(stderr, "No more expected acks\n");
			break;
		}

		//different circumstances of ack
		for (int ack = 0; ack < expectedAcks; ack++) {
			// How many bytes were received
			int recvbytes = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *) & ret_addr, &addrlen);
			gettimeofday(&ACK_RECV, 0); // When we get the ack

			/** Operation timed out */
			if (recvbytes < 0) {
				// newRtt = rtt;
				timeoutFlag = 1;
				counter++;
                if (canSend <= SLOW_DECLINE) {
                    canSend--;
                    canSend = max(canSend, 1);
                } else {
                    canSend /= 2;
                    canSend = max(canSend, 1);
                }
				if (counter >= 5) {
                    // RTT_TO.tv_sec = ACK_RECV.tv_sec - SENT.tv_sec;
                    RTT_TO.tv_usec = ACK_RECV.tv_usec - SENT.tv_usec;
					counter = 0;
				}

                break;
			} 
			else { /** Ack received */
				//int temp = timeDiff(ACK_RECV, SENT);
				buf[recvbytes] = '\0';
				int idx;
				char header[HEADER_SIZE];
				for (idx = 0; idx < recvbytes; idx++) {
					header[idx] = buf[idx];
					if (buf[idx] == ';') {
						header[idx] = '\0';
						idx++;
						break;
					}
				}

				int ackNum = 0;
				sscanf(header, "ack%d;", &ackNum);
				/** Ack is the last ack in the window */
				if (ackNum == expectedAck) {
					expectedAck++;
                    fileRead[ackNum % MAXWINDOW] = 0;
					framePointer++;
                    framePointer %= MAXWINDOW;
                    //slow ca
                    if (canSend > SLOW_GROWTH) { //step into CA phase
                        slowGrowth += 1.0/canSend;
                        if (slowGrowth > 1.0) {
                            canSend++;
                            canSend = min(canSend, MAXWINDOW);
                            slowGrowth = 0;
                        }
                    }
					else { //still in SS phase
                        canSend++;
                        canSend = min(canSend, MAXWINDOW);
                    }
				} 
				else if (ackNum > expectedAck) {
					fileRead[ackNum % MAXWINDOW] = 2;

                    if (canSend > SLOW_GROWTH) {
                        slowGrowth += 1.0/canSend;
                        if(slowGrowth > 1.0){
                            canSend++;
                            canSend = min(canSend, MAXWINDOW);
                            slowGrowth = 0;
                        }
                    }
                    else{
                        canSend++;
                        canSend = min(canSend, MAXWINDOW);
                    }
				} 
			}

		}
	}

	int eot;
	for (eot = 0; eot < 10; eot++) {
		mySend("EOT", sizeof("EOT"));
	}

	fclose(fp);
	close(s);
	return;
}

void setSockTimeout(struct timeval timer)
{
	if (setsockopt(s, SOL_SOCKET,SO_RCVTIMEO,&timer,sizeof(timer)) < 0) {
		fprintf(stderr, "Error setting socket timeout\n");
		return;
	}
}

int sendFiles(int framePointer)
{	
	int i = framePointer;
	int readBytes;
	int ackCount = 0;

	for (int files = 0; files < canSend; files++) {
		char packet[PACKET_SIZE + HEADER_SIZE];//1472 total
		memset(packet, 0, PACKET_SIZE + HEADER_SIZE);
		
		// If file is not read then read it and mark it
		if (fileRead[i] == 0) {
			fileRead[i] = 1;
			if(totalReadin - bytesAlreadyRead < PACKET_SIZE){
				int left = totalReadin - bytesAlreadyRead;
				if(left<=0) break; //all is read
				readBytes = fread(msgWindow[i], 1, left, fp);
				bytesAlreadyRead += readBytes;
			}
			else{
				readBytes = fread(msgWindow[i], 1, PACKET_SIZE, fp);
				bytesAlreadyRead += readBytes;
			}
			// Signals the end of the file
			if (readBytes < PACKET_SIZE) {
				lastBytes = readBytes;
				lastIndex = i;
			}
			// Set the msg frame with the actual frame count
			msgFrame[i] = frameCount++;
			sizeFrame[i] = readBytes;
		}
		else if(fileRead[i] == 1){
			readBytes = sizeFrame[i];//time out pkts
		}
		else{ //if it is an advanced ack 2
			// Last byte and break
			if (i == lastIndex) {
				break;
			}
			i++;
			i %= MAXWINDOW;
			files--;
			continue;
		}
		// Mark the frame being transmitted
		sprintf(packet, "frame%d;", msgFrame[i]);
		int start = strlen(packet);
		memcpy(packet + start, msgWindow[i], readBytes);
		mySend(packet, start + readBytes);
		// Last byte and break
		if (i == lastIndex) {
			// Trasnmit the packet
			ackCount++;
			break;
		} else {
			ackCount++;
		}
		i++;
		i %= MAXWINDOW;
	}
	return ackCount;
}

int mySend(char * buf, int bytes)
{
	return sendto(s, buf, bytes, 0, p->ai_addr, p->ai_addrlen);
}

int createSocket(char * hostname, unsigned short int hostUDPport)
{
	int sockfd;
	int rv;
	struct addrinfo hints, *servinfo;
	char portStr[10];
	memset(&hints, 0, sizeof hints);
    sprintf(portStr, "%d", hostUDPport);
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    memset(&servinfo,0,sizeof servinfo);
    if ((rv = getaddrinfo(hostname, portStr, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "sender: failed to bind socket\n");
        exit(1);
    }

	return sockfd;
}


