
#include <hardware/hardware.h>
#include <hardware/sensors.h>  

#include <fcntl.h>
#include <stdio.h>

struct hw_module_t * module;
struct hw_device_t * device;
struct sensors_event_t *event;


int main(){
    int write_str=2;

    printf("----begin main------\n");
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
    event = (sensors_event_t*)malloc(sizeof(event));
    dev->poll(dev, event, 2);
	printf("acceleration.x = %.2f\n", event->data[0]/255.00);
	printf("acceleration.y = %.2f\n", event->data[1]/255.00);
	printf("acceleration.z = %.2f\n", event->data[2]/255.00);    
    
    
    
    dev->activate(dev, write_str, 1);
    printf("write data: %d\n",write_str);
    
    
    
    dev->poll(dev,event, 2);
    printf("acceleration.x = %.2f\n", event->data[0]/255.00);
	printf("acceleration.y = %.2f\n", event->data[1]/255.00);
	printf("acceleration.z = %.2f\n", event->data[2]/255.00); 
	
	sensors_close_1(dev);
    printf("----end main------\n");
    return 0;
}
