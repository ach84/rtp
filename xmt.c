#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "rtp.h"

#define MAXSIZE		1024
#define DEFAULT_PORT	5000
#define DEFAULT_TOUT	500
#define DEFAULT_LEN	160

int debug = 0;
int timeout  = DEFAULT_TOUT;
int framelen = DEFAULT_LEN;
unsigned int timestamp = 0;
unsigned int sequence = 0;

# define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)

void dump_msg(char *msg, int len)
{
	int i = 0;
	printf("\nlen=%d", len);
	for (i = 0; i < len; i++) {
		if ((i & 0x0f) == 0)
			printf("\n  [%03d] ", i);
		printf("%02x ", (unsigned char)msg[i]);
	}
	printf("\n");
}

int print_help(char *appname)
{

	printf("\n%s <PARAMS> <OPTIONS>\n",appname);
	printf("\nPARAMS:\n");
	printf(" -a  <IP>     - adres hosta\n");
	printf(" -f  <FILE>   - plik do zapisu\n");
	printf("\nOPTIONS:\n");
	printf(" -p   <PORT>  - port hosta (default=%d)\n",DEFAULT_PORT);
	printf(" -m   <MODE>  - tryb nadawania (patrz 'MODE')\n\n");
	printf("       MODE=0 - tryb nadawania danych < STDIN (default)\n");
	printf("       MODE=1 - tryb nadawania sekwencji 'C<CHN>N<SEQ>' timeout=<TOUT>\n");
	printf("       MODE=2 - tryb nadawania stringa <STR>, timeout=<TOUT>\n");
	printf("       MODE=3 - tryb nadawania danych 'laaa..':\n");
	printf("       MODE=4 - tryb nadawania urandom, timeout=<TOUT>\n\n");	
	printf(" -l   <LEN>   - rozmiar payload (default=%d)\n",DEFAULT_LEN);
	printf(" -t   <TOUT>  - okres nadawania [ms] (default=%d)\n",DEFAULT_TOUT);
	printf(" -s   <STR>   - wejsciowy ciag znakow\n");
	printf(" -c   <CHN>   - numer kanalu\n");
	printf(" -v           - gadatliwe wyjcie\n");
	printf("\n\n");
	return -1;
}

void rtp_write(int dev, char *src, int len)
{

	struct timeval start,end,sub;
	rtp_h *rtp = (rtp_h *) src;
	int ret;

	gettimeofday(&start, NULL);

	//naglowek rtp
	rtp->version = 2;
	rtp->p  = 0;
	rtp->x  = 0;
	rtp->cc = 0;
	rtp->m  = 0;
	rtp->pt = 0x8;
	rtp->ssrc = 0; //0xffeeddcc;
	rtp->seq = htons(sequence++);
	rtp->ts  = htonl(timestamp); //htonl(timev.tv_sec);

	timestamp += 160;

	ret = write(dev, src, RTPSIZE + len);
	if (debug) {
		printf(">>> rtp ... seq=%d ret=%d", sequence, ret);
		dump_msg(src, RTPSIZE + len);
		printf("len=%d\n", len);
	}

	gettimeofday(&end, NULL);
	timersub(&end, &start, &sub);

	if (sub.tv_sec || timeout * 1000 < sub.tv_usec)
		printf("ERROR: sub=%d,%d\n",(int)sub.tv_sec, (int)sub.tv_usec);
	else
		usleep(timeout * 1000 - sub.tv_usec);

}

//-----------------------------------------------------------------------
int main(int argc, char *argv[])
{
	char *appname = argv[0];
	char *host_ip = 0, *filename = 0;
	struct sockaddr_in snd, rcv;
	int dev, c, dev_random;
	int port = DEFAULT_PORT;
	int mode = 0;
	int channel = 0;
	char buf[MAXSIZE], *p, *in;
	int size, s = 0, delay;

	while ((c = getopt(argc, argv, "a:p:f:m:c:t:s:l:vh")) != -1) {

		switch (c) {
		case 'v':
			debug = 1;
			break;
		case 'a':
			host_ip = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'm':
			mode = atoi(optarg);
			break;
		case 'c':
			channel = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 's':
			in = optarg;
			break;
		case 'l':
			framelen = atoi(optarg);
			break;
		case 'h':
		case '?':
			return print_help(appname);
		default:
			break;
		}
	}

	if (!host_ip && !filename)
		return print_help(appname);

	printf("\n%s params:\n\n", appname);
	printf("   HOST:    '%s:%d'\n", host_ip,port);
	printf("   FILE:    '%s' \n", filename);
	printf("   debug:   %d\n", debug);
	printf("   mode:    %d\n", mode);
	printf("   timeout: %d ms\n", timeout);

	if (host_ip) {
		dev = socket(AF_INET, SOCK_DGRAM, 0);

		snd.sin_addr.s_addr = inet_addr(host_ip);
		snd.sin_port = htons(port);
		snd.sin_family = AF_INET;
		memset(&(snd.sin_zero), '\0', 8);

		rcv.sin_addr.s_addr = INADDR_ANY;
		rcv.sin_port = htons(port);
		rcv.sin_family = AF_INET;
		memset(&(rcv.sin_zero), '\0', 8);

#if 0
		if (bind(socket_f, (struct sockaddr *)&rcv, sizeof(struct sockaddr)) < 0)
		{
			printf(" Bind error ! exit\n");
			return -1;
		}
#endif
		if (connect(dev, (struct sockaddr *)&snd, sizeof(struct sockaddr)) < 0) {
			printf("connect error\n");
			return -1;
		}
	}

	if (filename) {
		dev = open(filename, O_RDWR);
		if (dev < 0) {
			printf("open '%s' failed\n", filename);
			return dev;
		}
	}

	p = buf + RTPSIZE;

	if (mode == 1) {
		while (1) {
			sprintf(p, "C%02dN%06d\n", channel, (s++ % 1000000));
			size = strlen(p);
			rtp_write(dev, buf, strlen(p));
		}
	} else if (mode == 2) {
		strcpy(p, in);
		while (1) rtp_write(dev, buf, strlen(p));
	} else if (mode == 3) {
		srand(channel);
		delay = rand() % 1000000;
		usleep(delay);

		while (1) {
			size = rand() % framelen;
			if (size < 1)
				continue;	
			rtp_write(dev, buf, size);
		}
	} else if (mode == 4) {
		dev_random = open("/dev/urandom", O_RDONLY);
		read(dev_random, p, 512);
		while (1) rtp_write(dev, buf, framelen);
	} else {
		printf("\nPodaj ciag znakow + ENTER :\n\n");
		while (1) {
			fgets(p, MAXSIZE - RTPSIZE, stdin);
			rtp_write(dev, buf, strlen(p));
		}
	}
}
