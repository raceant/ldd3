/*************************************

NAME:misc.c
COPYRIGHT:www.ribeto.com

*************************************/

#include <string.h>
#include <sys/time.h>
#include "lordweb.h"
#include "misc.h"

//return '0' if success
int excute_cmd(long iCmd)
{
    int fd;

	fd = open("/dev/Lord-Control", 0);
	if (fd < 0) {
		perror("open device Lord-Control");
		return;
	}
	ioctl(fd, LORD_IO_WRITE, iCmd);
	close(fd);
}

int test_vd108(void)
{
    int fd;
    int rv;

	fd = open("/dev/Lord-Control", 0);
	if (fd < 0) {
		perror("open device Lord-Control");
		return;
	}
	rv = ioctl(fd, LORD_VEHICLE_STAT, 0);
	close(fd);
    return rv;
}

/***********************************************************/

//return current ticks (1tick = 1ms)
unsigned int GetTicks(void)
{
	unsigned int dwTicks;
    struct timeval tv;
    struct timezone tz;
	
	gettimeofday (&tv , &tz);
	dwTicks = tv.tv_sec * 1000 + tv.tv_usec/1000;
	return dwTicks;
}



