#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <syscall.h>

#define DEB(x,z...) printf("%s[%ld]: " x, __func__, syscall(SYS_gettid), ##z);

static pthread_mutex_t lock;

void usage(char *name)
{
	printf("\n Usage: %s [-h]\n\n", name);
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

	while (1) {
		int res = pthread_mutex_lock(&lock);
		DEB("res: %d count: %04d\n", res, count++);
		sleep(1);
		pthread_mutex_unlock(&lock);
	}

	return NULL;
}

void thread_init(void)
{
	DEB("pid: %d\n", getpid());

	int res = pthread_mutex_init(&lock, 0);
	DEB("res: %d\n", res);

	for (int i = 0; i < 4; i++) {
		pthread_t th;
		pthread_create(&th, NULL, &thread_func, NULL);
	}

	while(1) sleep(1);
}

int main(int argc, char *argv[])
{
	int c;

	DEB("init\n");

	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
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
