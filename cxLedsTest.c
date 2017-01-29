#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    int fd;
    int i = 9;
    char* leds = "/dev/cxledsDriver";

    if((fd = open(leds, O_RDWR | O_NDELAY)) < 0)
    {
        printf("Can't Open %s !\n", leds);
    }
    else
    {
        while(i--)
        {
            ioctl(fd, 0, 0);
            ioctl(fd, 1, 1);
            usleep(500000);
            ioctl(fd, 0, 1);
            ioctl(fd, 1, 0);
            usleep(500000);
        }

		printf("cxLedsTest Start!\n");
		ioctl(fd, 9, 9);
		
        close(fd);
    }

    return 0;
}
