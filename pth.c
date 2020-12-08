#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <syscall.h>

#define DEB(x,z...) printf("%s[%ld]: " x, __func__, syscall(SYS_gettid), ##z);

int nthreads = 8;

void usage(char *name)
{
	printf("\n Usage: %s [-nh]\n\n", name);
	printf(" Options:\n");
	printf("\n\n");
}

void *thread_func(void *arg)
{
	struct tm *ptm; 
	time_t now;
	int count = 0;

	now = time(NULL);
	ptm = (struct tm *) malloc(sizeof(struct tm));
	localtime_r(&now, ptm);

	DEB("init\n");

	pthread_mutex_t lock;
	int res = pthread_mutex_init(&lock, 0);

	while (1) {
		res = pthread_mutex_lock(&lock);
		DEB("res: %d count: %04d\n", res, count++);
		pthread_mutex_unlock(&lock);
		sleep(1);
	}

	return NULL;
}

void thread_init(void)
{
	DEB("pid: %d\n", getpid());

	pthread_attr_t attr;
	size_t size;
	int res, i = 0;

	pthread_attr_init(&attr);
	res = pthread_attr_getstacksize(&attr, &size);
	DEB("res: %d stack: %lu\n", res, size);

	for (i = 0; i < nthreads; i++) {
		pthread_t th;
		res = pthread_create(&th, &attr, &thread_func, NULL);
		if (res) break;
	}

	DEB("i: %d\n", i);

	while(1) sleep(1);
}

int main(int argc, char *argv[])
{
	int c;

	DEB("init\n");

	while ((c = getopt(argc, argv, "n:h")) != -1) {
		switch (c) {
		case 'n':
			nthreads = atoi(optarg);
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	thread_init();
	return 0;
}
