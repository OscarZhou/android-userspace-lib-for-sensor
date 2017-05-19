
#include <hardware/hardware.h>
#include <hardware/sensors.h>  

#include <fcntl.h>
#include <stdio.h>

struct hw_module_t * module;
struct hw_device_t * device;
struct sensors_event_t *event;

int main(){
    int write0 = 0;
    int write1 = 1;
	int handler1 = 0;
	int handler2 = 1;
	int handler3 = 2;
	
    printf("----begin main------\n");
	//module = (hw_module_t *)malloc(sizeof(hw_module_t));
	//device = (hw_device_t *)malloc(sizeof(hw_device_t));
    if(hw_get_module(SENSORS_HARDWARE_MODULE_ID,(struct hw_module_t const **)&module)==0)	
    {
        printf("get module sucess\n");
    }else{
        printf("get module fail\n");
        return -1;
    }

    if(sensors_open_1(module,(struct hw_device_t const**)&device)==0){
        printf("open module sucess\n");
    }else{
        printf("open module error\n");
        return -2;
    }
    struct sensors_poll_device_1* dev = (struct sensors_poll_device_1 *)device;
    event = (sensors_event_t*)malloc(sizeof(sensors_event_t)*3);
    
    int i = 0;
   	
    
    dev->activate(dev, handler1, write0);
    dev->activate(dev, handler2, write0);
    dev->activate(dev, handler3, write0);
    printf("\ninput data: %d %d %d\n",write0 , write0, write0);

	dev->poll(dev, event, 3);
	printf("1.output data: \n");
   	for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }
    
    dev->activate(dev, handler1, write1);
    dev->activate(dev, handler2, write1);
    dev->activate(dev, handler3, write1);
    printf("\ninput data: %d %d %d\n",write1, write1, write1);

	dev->poll(dev, event, 3);
	printf("2.output data: \n");
    for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }

    dev->activate(dev, handler1, write0);
    dev->activate(dev, handler2, write0);
    dev->activate(dev, handler3, write1);
    printf("\ninput data: %d %d %d\n",write0, write0, write1);
    
	dev->poll(dev, event, 3);
	printf("3.output data: \n");
    for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }
    
    
    dev->activate(dev, handler1, write1);
    dev->activate(dev, handler2, write0);
    dev->activate(dev, handler3, write0);
    printf("\ninput data: %d %d %d\n",write1, write0, write0);
    
	printf("4.output data: \n");
    dev->poll(dev,event, 1);
	for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("AFTER: acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("AFTER: acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("AFTER: acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("AFTER: magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("AFTER: magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("AFTER: magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("AFTER: gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("AFTER: gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("AFTER: gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }
    
    
    dev->activate(dev, handler1, write0);
    dev->activate(dev, handler2, write1);
    dev->activate(dev, handler3, write0);
    printf("\ninput data: %d %d %d\n",write0, write1, write0);
    
	printf("5.output data: \n");
    dev->poll(dev,event, 1);
	for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("AFTER: acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("AFTER: acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("AFTER: acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("AFTER: magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("AFTER: magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("AFTER: magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("AFTER: gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("AFTER: gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("AFTER: gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }
	

	dev->activate(dev, handler1, write1);
    dev->activate(dev, handler2, write1);
    dev->activate(dev, handler3, write0);
    printf("\ninput data: %d %d %d\n",write1, write1, write0);
    
	printf("6.output data: \n");
    dev->poll(dev,event, 1);
	for(i=0; i<3; i++)
    {
		if((event+i)->type == SENSOR_TYPE_ACCELEROMETER)
		{
			printf("AFTER: acceleration.x = %.2f\n", (event+i)->acceleration.x);
			printf("AFTER: acceleration.y = %.2f\n", (event+i)->acceleration.y);
			printf("AFTER: acceleration.z = %.2f\n", (event+i)->acceleration.z); 
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_MAGNETIC_FIELD)
		{
			printf("AFTER: magnetic.x = %.2f\n", (event+i)->magnetic.x);
			printf("AFTER: magnetic.y = %.2f\n", (event+i)->magnetic.y);
			printf("AFTER: magnetic.z = %.2f\n", (event+i)->magnetic.z);  
			continue;
		}
		if((event+i)->type == SENSOR_TYPE_GYROSCOPE)
		{
			printf("AFTER: gyroscope.x = %.2f\n", (event+i)->gyro.x);
			printf("AFTER: gyroscope.y = %.2f\n", (event+i)->gyro.y);
			printf("AFTER: gyroscope.z = %.2f\n", (event+i)->gyro.z); 
			continue;
		}
		
    }
    
    free(event);
    event = NULL;
	//free(device);
	//device = NULL;
    

	sensors_close_1(dev);
	printf("----end main------\n");
  
  	//free(module);
    //module = NULL;
    return 0;
}
