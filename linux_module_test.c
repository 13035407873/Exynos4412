#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc,char * argv [ ])
{
	int fd;
	char *hello = "/dev/helloDriver";

	fd = open(hello, O_RDWR | O_NDELAY);
	if(fd < 0)
	{
		printf("Error!\n");
		return -1;
	}

	printf("Test is Open Success!\n");
	ioctl(fd, 520, 999);
	close(fd);
	
	return 0;
}