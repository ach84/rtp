
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include "rtp.h"

#define DEFPORT 5000
#define MAXSIZE 1024
#define MAXDEVS 64

int debug = 0;

struct dev_t {
	int dev;
	char *devname;
	unsigned int timestamp;
	unsigned int sequence;
};

# define dump_data(data, len)				\
  do {							\
    int i = 0;						\
    for (i = 0; i < len; i++) {				\
       if ((i & 0x0f) == 0)				\
               printf("\n  [%03d] ", i);		\
       printf("%02x ", (unsigned char)data[i]);		\
    }							\
    printf("\n");					\
  } while (0)

//-----------------------------------------------------------------------
int print_help(char *appname)
{

	printf("\n%s <PARAMS> <OPTIONS>\n\n",appname);
	printf(" PARAMS:\n");
	printf("  -n  <DEVS>   - liczba plikow/socketow [1..n]\n");
	printf("  -a  <IP>     - adres hosta\n");
	printf("  -p  <PORT>   - port bazowy (default=%d)\n", DEFPORT);
	printf("  -d  <DIR>    - katalog\n");
	printf("  -w           - write loop\n");
	printf("  -r           - read loop\n");
	printf("\n");
	printf(" OPTIONS:\n");
	printf("  -v           - gadatliwe wyjcie\n");
	printf("\n\n");
	return -1;
}

void rtp_write(struct dev_t *dev, char *src, int len)
{
	rtp_h *rtp = (rtp_h *) src;
	int ret;

	//naglowek rtp
	rtp->version = 2;
	rtp->p = 0;
	rtp->x = 0;
	rtp->cc = 0;
	rtp->m = 0;
	rtp->pt = 0x8;
	rtp->ssrc = 0; //0xffeeddcc;
	rtp->seq = htons(dev->sequence);
	rtp->ts  = htonl(dev->timestamp); //htonl(timev.tv_sec);

	dev->sequence++;
	dev->timestamp += 160;

	ret = write(dev->dev, src, RTPSIZE + len);
	if (debug) {
		printf(">>> rtp ... seq=%d ret=%d len=%d\n", dev->sequence, ret, len);
	}

}

//-----------------------------------------------------------------------
int main(int argc, char *argv[])
{
	char *appname = argv[0];
	char *devpath;
	char *host = 0;
	struct dev_t dev[MAXDEVS];
	struct sockaddr_in send, recv;
	int dev_rand, max_dev, c, i, len, dev_no = 0, port = DEFPORT;
	fd_set fd;
	char buf[MAXSIZE];
	char mode;
	struct timeval start, end, sub;

	while ((c = getopt(argc, argv, "a:p:d:n:wrvh")) != -1) {
		switch (c) {
		case 'v':
			debug++;
			break;
		case 'a':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'd':
			devpath = strdup(optarg);
			break;
		case 'n':
			dev_no = atoi(optarg) + 1;
			break;
		case 'w':
			mode = 'w';
			break;
		case 'r':
			mode = 'r';
			break;
		case 'h':
		case '?':
			return print_help(appname);
		default:
			break;
		}
	}

	if (dev_no > MAXDEVS || !dev_no) {
		printf("device limit = [1,%d]\n", MAXDEVS);
		return -EINVAL;
	}

	for (i = 0; i < dev_no; i++) {
		if (host) {
			dev[i].dev = socket(AF_INET, SOCK_DGRAM, 0);

			if (dev[i].dev < 0) {
				printf("open socket failed\n");
				return -EINVAL;
			}

			send.sin_addr.s_addr = inet_addr(host);
			send.sin_port = htons(port + i);
			send.sin_family = AF_INET;
			memset(&send.sin_zero, '\0', 8);
			recv.sin_addr.s_addr = INADDR_ANY;
			recv.sin_port = htons(port + i);
			recv.sin_family = AF_INET;
			memset(&recv.sin_zero, '\0', 8);

			if (connect(dev[i].dev, (struct sockaddr *)&send, sizeof(struct sockaddr)) < 0) {
				printf("connect failed\n");
				return -EINVAL;
			}
		} else {
			sprintf(buf, "%s/data%02d", devpath, i);
			dev[i].dev = open(buf, O_RDWR);

			if (dev[i].dev < 0) {
				printf("open '%s' failed\n", buf);
				return -EINVAL;
			}
		}
		max_dev = dev[i].dev;
	}

	switch (mode) {
	case 'r':
		goto read_loop;
	case 'w':
		goto write_loop;
	default:
		return 0;
	}

read_loop:
	while (1) {
		FD_ZERO(&fd);

		for (i = 0; i < dev_no; i++)
			FD_SET(dev[i].dev, &fd);

		if ((select(max_dev + 1, &fd, NULL, NULL, NULL)) > 0) {
			for (i = 0; i < dev_no; i++)
				if (FD_ISSET(dev[i].dev, &fd)) {
					len = read(dev[i].dev, buf, sizeof(buf));

					if (len > 0 && debug) {
						printf("dev=%d len=%d",dev[i].dev,len);
						dump_data(buf, len);
					}
				}
		}
	}

write_loop:
	dev_rand = open("/dev/urandom", O_RDONLY);
	read(dev_rand, buf, 256);

	while (1) {
		gettimeofday(&start, NULL);

		for (i = 0; i < dev_no; i++)
			rtp_write(&dev[i], buf, 160);

		gettimeofday(&end, NULL);
		timersub(&end,&start,&sub);

		if (sub.tv_sec || 20000 < sub.tv_usec) {
			printf("error -- sub: %d.%06d [s]\n", (int)sub.tv_sec, (int)sub.tv_usec);
			usleep(1000000);
		} else {
			usleep(20000 - sub.tv_usec);
		}
	}
}
