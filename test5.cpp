#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>


/**********************************************************/
/*                     written by Hongyu ZHOU
/*				16242950

/*Compile: 
	arm-none-linux-gnueabi-gcc -static test.cpp -o test
/*Move executable file to device:  
	./adb push ~/Downloads/test /dev
/*Execute test program:	   
	.test
/**********************************************************/


#define DEVICENAME	"/dev/goldfish_sensor"
int main()
{
	static int fd = -1;
	char outfile[256] = {0};

	int infile = 5;
	int result = 0;
	int i = 0;
	fd = open(DEVICENAME, O_RDWR);
	printf("----------------start---------------------\n");
	if(fd<0)
	{
		perror("open");
		exit(1);
	}
	result = read(fd, (char*)outfile, sizeof(outfile));
	if(result<0)
	{
		perror("read error 1");
		printf("\n");
		exit(1);
	}
	printf("outfile = %s\n", outfile);
	char output1[256] = {0};
	char output2[256] = {0};
	char output3[256] = {0};
	if (sscanf(outfile, "%s %s %s", output1, output2, output3) == 4) 
	{
		printf("-------------output1= %s\n!", output1);
		printf("-------------output2= %s\n!", output2);
		printf("-------------output3= %s\n!", output3);		
	}
	else
	{
		printf("%s\n%s\n%s\n%s\n",outfile, output1, output2, output3);
	}
	memset(output1, 0, sizeof(output1));
	memset(output2, 0, sizeof(output3));
	memset(output3, 0, sizeof(output3));
	
	result = write(fd, &infile, sizeof(infile));
	printf("-------------inputfile= %d\n!", infile);
	if(result<0)
	{
		perror("write error 2");
		printf("\n");
		exit(1);
	}
	result = read(fd, (char*)outfile, sizeof(outfile));
	if(result<0)
	{
		perror("read error 1");
		printf("\n");
		exit(1);
	}
	printf("outfile = %s\n", outfile);
	if (sscanf(outfile, "%s-%s-%s", output1, output2, output3) == 3) 
	{
		printf("-------------output1= %s\n!", output1);
		printf("-------------output2= %s\n!", output2);
		printf("-------------output3= %s\n!", output3);		
	}
	printf("-------------------end------------------\n");
	
	close(fd);
	return 0;
}
