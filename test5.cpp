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


struct output_t{
	char output[256];
};

#define DEVICENAME	"/dev/goldfish_sensor"
int main()
{
	static int fd = -1;
	char outfile[256] = {0};

	int infile = 3;
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
	output_t * p_output = (output_t*)malloc(sizeof(output_t)*3);
	int output_value[9] = {0};
	printf("sscanf = %d\n", sscanf(outfile, "%s %s %s", output1, output2, output3));
	//sscanf(outfile, "%s %s %s", output1, output2, output3);
	//printf("%s\n%s\n%s\n%s\n",outfile, output1, output2, output3);
	sscanf(outfile, "%s %s %s", p_output->output, (p_output+1)->output, (p_output+2)->output);
	
	for(i=0;i<3;i++)
	//if (sscanf(outfile, "%s %s %s", output1, output2, output3) ==3 ) 
	{
		
		printf("-------------p_output= %s\n!", p_output);
		printf("-------------p_output+1= %s\n!", p_output+1);
		printf("-------------p_output+2= %s\n!", p_output+2);	
		if (sscanf((p_output+i)->output, "acceleration:%d:%d:%d", output_value+0, output_value+1, output_value+2) == 3)
		{
			continue;
		}
		if (sscanf((p_output+i)->output, "magnetic:%d:%d:%d", output_value+3, output_value+4, output_value+5) == 3)
		{
			continue;
		}
		if (sscanf((p_output+i)->output, "acceleration:%d:%d:%d", output_value+6, output_value+7, output_value+8) == 3)
		{
			continue;
		}
	}
	
	
	printf("acceleration.x = %.2f\n", (float)*(output_value+0)/255.0);
	printf("acceleration.y = %.2f\n", (float)*(output_value+1)/255.0);
	printf("acceleration.z = %.2f\n", (float)*(output_value+2)/255.0); 
	
	printf("gyroscope.x = %.2f\n", (float)*(output_value+6)/255.0);
	printf("gyroscope.y = %.2f\n", (float)*(output_value+7)/255.0);
	printf("gyroscope.z = %.2f\n", (float)*(output_value+8)/255.0);    
	
    printf("magnetic.x = %.2f\n", (float)*(output_value+3)/255.0);
	printf("magnetic.y = %.2f\n", (float)*(output_value+4)/255.0);
	printf("magnetic.z = %.2f\n", (float)*(output_value+5)/255.0);  
	 
	/*
	else
	{
		printf("%s\n%s\n%s\n%s\n",outfile, output1, output2, output3);
	}
	*/
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
	
	printf("-------------------end------------------\n");
	free(p_output);
	close(fd);
	return 0;
}
