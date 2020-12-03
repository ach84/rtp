#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "rtp.h"

#define IP4_VALUES(_XIP_)	( (_XIP_) ) & 0xff , \
				( (_XIP_) >> 8 ) & 0xff , \
				( (_XIP_) >> 16 ) & 0xff , \
				( (_XIP_) >> 24 ) & 0xff
#define MAXSIZE 1024

//-------------------------------------------------------
void hexdump(unsigned char *msg, int len)
{
	int i = 0;

	for (i = 0; i < len; i++) {
		if ((i & 0x0f) == 0)
			printf("\n  [%03d] ", i);
		printf("%02x ", msg[i]);
	}
	printf("\n\n");
}

void txtdump(unsigned char *msg, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		//if((i & 0x0f) == 0)printf("\n  ",i);   
		printf("%c", msg[i]);
	}
	//printf("\n");
}

//-----------------------------------------------------------------------
int print_help(char *appname)
{

	printf("\nUsage:");
	printf("\n%s <PARAM>\n", appname);
	printf("\nPARAM:\n");
	printf(" -p  <PORT>  - port nasluchowy\n");
	printf(" -v          - debug info\n");
	printf(" -vv         - dump w trybie txt\n");
	printf(" -x          - dump w trybie hex\n");
	printf("\n\n");
	return -1;
}

//-----------------------------------------------------------------------
int main(int argc, char *argv[])
{

	struct sockaddr_in sockin;
	int dev;
	int debug = 0, hex = 0;
	int port = 0;
	unsigned char buf[MAXSIZE];
	int len, c;
	char *appname = argv[0];
	struct timeval now;

	while ((c = getopt(argc, argv, "p:vxh")) != -1) {

		switch (c) {
		case 'v':
			debug++;
			break;
		case 'x':
			hex = 1;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
		case '?':
			return print_help(appname);

		default:
			break;
		}
	}

	if (!port)
		return print_help(appname);

	if (debug) {
		printf("\nparams:\n\n");
		printf("   PORT:  %d\n", port);
		printf("   debug: %d\n", debug);
		printf("   hex:   %d\n", hex);
	}

	dev = socket(AF_INET, SOCK_DGRAM, 0);
	sockin.sin_addr.s_addr = INADDR_ANY;
	sockin.sin_port = htons(port);
	sockin.sin_family = AF_INET;
	memset(&(sockin.sin_zero), '\0', 8);

	//setsockopt (sckt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));

	if (bind(dev, (struct sockaddr *)&sockin, sizeof(struct sockaddr)) < 0) {
		printf(" Bind error ! dev=%x\n", dev);
		return -1;
	}

	while (1) {

		len = read(dev, buf, MAXSIZE);

		gettimeofday(&now, NULL);

		if (debug)
			printf("recv port=%d len=%d time=%d,%d\n",
				port,len,(int)now.tv_sec,(int)now.tv_usec);

		if (debug>1)
			txtdump(buf, len);
		
		if (hex)
			hexdump(buf, len);
	
		fflush(stdout);
		//sleep(1);
	} //while true

	return 0;
}
