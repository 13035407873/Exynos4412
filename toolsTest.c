#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm-generic/ioctl.h>

int main(int argc, char* argv[])
{
    int fd;
    char* tools = "/dev/tools";

    if((fd = open(tools, O_RDWR | O_NDELAY)) < 0)
    {
        printf("Can't Open %s !\n", tools);
    }
    else
    {
		

		ioctl(fd, _IOWR('A', 0, int), 0);
        ioctl(fd, _IOWR('A', 1, int), 0);
		ioctl(fd, _IOWR('A', 2, int), 0);
        usleep(500000);
		printf("ADC: %d\n", ioctl(fd, _IOWR('A', 3, int), 0));

		ioctl(fd, _IOWR('A', 0, int), 1);
        ioctl(fd, _IOWR('A', 1, int), 1);
		ioctl(fd, _IOWR('A', 2, int), 10);
        usleep(500000);
		printf("ADC: %d\n", ioctl(fd, _IOWR('A', 3, int), 0));

		ioctl(fd, _IOWR('A', 0, int), 0);
        ioctl(fd, _IOWR('A', 1, int), 0);
		ioctl(fd, _IOWR('A', 2, int), 20);
        usleep(500000);
		printf("ADC: %d\n", ioctl(fd, _IOWR('A', 3, int), 0));

		ioctl(fd, _IOWR('A', 0, int), 0);
        ioctl(fd, _IOWR('A', 1, int), 0);
		ioctl(fd, _IOWR('A', 2, int), 0);
        usleep(500000);
		usleep(500000);
		usleep(500000);
		usleep(500000);
		usleep(500000);
		usleep(500000);

		close(fd);
    }

    return 0;
}

