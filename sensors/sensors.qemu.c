/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* this implements a sensors hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/sensors.goldfish.so
 *
 * it will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from com_android_server_SensorService.cpp
 */
/* we connect with the emulator through the "sensors" qemud service
 */
#define  SENSORS_SERVICE_NAME "sensors"
#define LOG_TAG "QemuSensors"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/native_handle.h>
#include <cutils/sockets.h>
#include <hardware/sensors.h>

#if 1
#define  D(...)  ALOGD(__VA_ARGS__)
#else
#define  D(...)  ((void)0)
#endif
#define  E(...)  ALOGE(__VA_ARGS__)
#include <hardware/qemud.h>
/** SENSOR IDS AND NAMES
 **/
#define MAX_NUM_SENSORS 3
#define SUPPORTED_SENSORS  ((1<<MAX_NUM_SENSORS)-1)
#define  ID_BASE           SENSORS_HANDLE_BASE
#define  ID_ACCELERATION   (ID_BASE+0)
#define  ID_MAGNETIC_FIELD (ID_BASE+1)
#define  ID_GYROSCOPE    (ID_BASE+2)


#define  SENSORS_ACCELERATION   (1 << ID_ACCELERATION)
#define  SENSORS_MAGNETIC_FIELD  (1 << ID_MAGNETIC_FIELD)
#define  SENSORS_GYROSCOPE     (1 << ID_GYROSCOPE)

#define  ID_CHECK(x)  ((unsigned)((x)-ID_BASE) < MAX_NUM_SENSORS)
#define  SENSORS_LIST  \
    SENSOR_(ACCELERATION,"acceleration") \
    SENSOR_(MAGNETIC_FIELD,"magnetic-field") \
    SENSOR_(GYROSCOPE,"gyroscope") \

#define DEVICENAME	"/dev/goldfish_sensor"

static const struct {
    const char*  name;
    int          id; } _sensorIds[MAX_NUM_SENSORS] =
{
#define SENSOR_(x,y)  { y, ID_##x },
    SENSORS_LIST
#undef  SENSOR_
};
static const char*
_sensorIdToName( int  id )
{
    int  nn;
    for (nn = 0; nn < MAX_NUM_SENSORS; nn++)
        if (id == _sensorIds[nn].id)
            return _sensorIds[nn].name;
    return "<UNKNOWN>";
}
static int
_sensorIdFromName( const char*  name )
{
    int  nn;
    if (name == NULL)
        return -1;
    for (nn = 0; nn < MAX_NUM_SENSORS; nn++)
        if (!strcmp(name, _sensorIds[nn].name))
            return _sensorIds[nn].id;
    return -1;
}
/** SENSORS POLL DEVICE
 **
 ** This one is used to read sensor data from the hardware.
 ** We implement this by simply reading the data from the
 ** emulator through the QEMUD channel.
 **/
typedef struct SensorPoll {
    struct sensors_poll_device_1  device;
    sensors_event_t               sensors[MAX_NUM_SENSORS];
    int                           events_fd;
    uint32_t                      pendingSensors;
    int64_t                       timeStart;
    int64_t                       timeOffset;
    int                           fd;
    uint32_t                      active_sensors;
} SensorPoll;
/* this must return a file descriptor that will be used to read
 * the sensors data (it is passed to data__data_open() below
 */
static native_handle_t*
control__open_data_source(struct sensors_poll_device_1 *dev)
{
    SensorPoll*  ctl = (void*)dev;
    native_handle_t* handle;
    if (ctl->fd < 0) {
        ctl->fd = qemud_channel_open(SENSORS_SERVICE_NAME);
    }
    D("%s: fd=%d", __FUNCTION__, ctl->fd);
    handle = native_handle_create(1, 0);
    handle->data[0] = dup(ctl->fd);
    return handle;
}
static int
control__activate(struct sensors_poll_device_1 *dev,
                  int handle,
                  int enabled)
{
    SensorPoll*     ctl = (void*)dev;
    uint32_t        mask, sensors, active, new_sensors, changed;
    char            command[128];
    int             ret;
    D("oscar write 3 %s: handle=%s (%d) fd=%d enabled=%d", __FUNCTION__,
        _sensorIdToName(handle), handle, ctl->fd, enabled);
    if (!ID_CHECK(handle)) {
        E("oscar write 4 %s: bad handle ID", __FUNCTION__);
        return -1;
    }
    mask    = (1<<handle);
    sensors = enabled ? mask : 0;
    active      = ctl->active_sensors;
    new_sensors = (active & ~mask) | (sensors & mask);
    changed     = active ^ new_sensors;
    if (!changed)
        return 0;
    snprintf(command, sizeof command, "set:%s:%d",
                _sensorIdToName(handle), enabled != 0);
    if (ctl->fd < 0) {
        ctl->fd = qemud_channel_open(SENSORS_SERVICE_NAME);
    }
    ret = qemud_channel_send(ctl->fd, command, -1);
    if (ret < 0) {
        E("oscar write 6 %s: when sending command errno=%d: %s", __FUNCTION__, errno, strerror(errno));
        return -1;
    }
    ctl->active_sensors = new_sensors;
    
    /********** open the file goldfish_sensor in the dev ****************/
    int file_name = open(DEVICENAME, O_WRONLY);
	if(file_name<0)
	{
		D("oscar write 1 %s: file %d cannot be opened ", __FUNCTION__, file_name);
		exit(1);
	}

	int result = 0;
	
	D("oscar %s: file name=%d ", __FUNCTION__, file_name);
    result = write(file_name, &enabled, sizeof(enabled));
	if(result<0)
	{
		D("oscar write 2 %s: result=%d", __FUNCTION__, result);
		close(file_name);
		exit(1);
	}
	close(file_name);
    /********************************************************************/
    return 0;
}
static int
control__set_delay(struct sensors_poll_device_1 *dev, int32_t ms)
{
    SensorPoll*     ctl = (void*)dev;
    char            command[128];
    D("%s: dev=%p delay-ms=%d", __FUNCTION__, dev, ms);
    snprintf(command, sizeof command, "set-delay:%d", ms);
    return qemud_channel_send(ctl->fd, command, -1);
}
static int
control__close(struct hw_device_t *dev)
{
    SensorPoll*  ctl = (void*)dev;
    close(ctl->fd);
    free(ctl);
    return 0;
}
/* return the current time in nanoseconds */
static int64_t
data__now_ns(void)
{
    struct timespec  ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}
static int
data__data_open(struct sensors_poll_device_1 *dev, native_handle_t* handle)
{
    SensorPoll*  data = (void*)dev;
    int i;
    D("%s: dev=%p fd=%d", __FUNCTION__, dev, handle->data[0]);
    memset(&data->sensors, 0, sizeof(data->sensors));
    data->pendingSensors = 0;
    data->timeStart      = 0;
    data->timeOffset     = 0;
    data->events_fd = dup(handle->data[0]);
    D("%s: dev=%p fd=%d (was %d)", __FUNCTION__, dev, data->events_fd, handle->data[0]);
    native_handle_close(handle);
    native_handle_delete(handle);
    return 0;
}
static int
data__data_close(struct sensors_poll_device_1 *dev)
{
    SensorPoll*  data = (void*)dev;
    D("%s: dev=%p", __FUNCTION__, dev);
    if (data->events_fd >= 0) {
        close(data->events_fd);
        data->events_fd = -1;
    }
    return 0;
}
static int
pick_sensor(SensorPoll*       data,
            sensors_event_t*  values)
{
	D("oscar read 2 %s: ", __FUNCTION__);
    uint32_t mask = SUPPORTED_SENSORS;
    D("oscar read 13 %b: ", __FUNCTION__, mask);
    while (mask) {
        uint32_t i = 31 - __builtin_clz(mask);
        mask &= ~(1<<i);
        if (data->pendingSensors & (1<<i)) {
            data->pendingSensors &= ~(1<<i);
            *values = data->sensors[i];
            values->sensor = i;
            D("oscar read 14 sensor index= %d: ", __FUNCTION__, i);
            values->version = sizeof(*values);
            D("oscar read 3 %s: %d [%f, %f, %f]", __FUNCTION__,
                    i,
                    values->data[0],
                    values->data[1],
                    values->data[2]);
            return i;
        }
    }
    ALOGE("No sensor to return!!! pendingSensors=%08x", data->pendingSensors);
    // we may end-up in a busy loop, slow things down, just in case.
    usleep(100000);
    return -EINVAL;
}
static int
data__poll(struct sensors_poll_device_1 *dev, sensors_event_t* values)
{
    SensorPoll*  data = (void*)dev;
    int fd = data->events_fd;
    //D("oscar read 1 %s: data=%p", __FUNCTION__, dev);
    // there are pending sensors, returns them now...
    if (data->pendingSensors) {
        return pick_sensor(data, values);
    }
    // wait until we get a complete event for an enabled sensor
    uint32_t new_sensors = 0;
    
    /************open the file goldfish_sensor in the dev************/
    int file_name = open(DEVICENAME, O_RDONLY);
    if(file_name<0)
	{
		D("oscar read 7 %s: file %d cannot be opened ", __FUNCTION__, file_name);
		exit(1);
	}
	
	char output[256] = {0};
	int result = 0;
    /****************************************************************/
    while (1) {
	    //D("oscar read 4 %s: filename=%d", __FUNCTION__, file_name);
        /* read the next event */
        char     buff[256];
        int      len = qemud_channel_recv(data->events_fd, buff, sizeof buff-1);
        float    params[3];
        int64_t  event_time;
        /*
        if (len < 0) {
            E("oscar read 8 %s: len=%d, errno=%d: %s", __FUNCTION__, len, errno, strerror(errno));
            return -errno;
        }
        */
        buff[len] = 0;
        /* "wake" is sent from the emulator to exit this loop. */
        if (!strcmp((const char*)data, "wake")) {
            return 0x7FFFFFFF;
        }
        
        /************   read the data from goldfish_sensor   ************/
		result = read(file_name, output, sizeof(output));
		if(result<0)
		{
			D("oscar read 9 %s: result=%d", __FUNCTION__, result);
			close(file_name);
			exit(1);
		}
		
		//D("oscar read 5 %s: output=%s", __FUNCTION__, output);
		char output1[256] = {0};
		char output2[256] = {0};
		char output3[256] = {0};
		int output_value[9] = {0};
		
		
		D("oscar read 9 %s: sscanf=%d ", __FUNCTION__, sscanf(output, "%s %s %s", output1, output2, output3));
		if (sscanf(output, "%s %s %s", output1, output2, output3) == 3) 
		{
			//D("oscar read 6 %s: output1=%s, output2=%s, output3=%s", __FUNCTION__, output1, output2, output3);
			//D("oscar read 10 %s: sscanf=%d output1=%s", __FUNCTION__, sscanf(output1, "acceleration:%d:%d:%d", output_value+0, output_value+1, output_value+2), output1);
			//D("oscar read 11 %s: sscanf=%d output2=%s", __FUNCTION__, sscanf(output2, "magnetic:%d:%d:%d", output_value+3, output_value+4, output_value+5), output2);
			//D("oscar read 12 %s: sscanf=%d output3=%s", __FUNCTION__, sscanf(output3, "gyroscope:%d:%d:%d", output_value+6, output_value+7, output_value+8), output3);
			/* "acceleration:<x>:<y>:<z>" corresponds to an acceleration event */
			if (sscanf(output1, "acceleration:%d:%d:%d", output_value+0, output_value+1, output_value+2) == 3) {
		        new_sensors |= SENSORS_ACCELERATION;
		        data->sensors[ID_ACCELERATION].acceleration.x = output_value[0];
		        data->sensors[ID_ACCELERATION].acceleration.y = output_value[1];
		        data->sensors[ID_ACCELERATION].acceleration.z = output_value[2];
		        data->sensors[ID_ACCELERATION].type = SENSOR_TYPE_ACCELEROMETER;
      		}
  	        /* "magnetic:<x>:<y>:<z>" is sent for the params of the magnetic field */
		    if (sscanf(output2, "magnetic:%d:%d:%d", output_value+3, output_value+4, output_value+5) == 3) {
		        new_sensors |= SENSORS_MAGNETIC_FIELD;
		        data->sensors[ID_MAGNETIC_FIELD].magnetic.x = output_value[3];
		        data->sensors[ID_MAGNETIC_FIELD].magnetic.y = output_value[4];
		        data->sensors[ID_MAGNETIC_FIELD].magnetic.z = output_value[5];
		        data->sensors[ID_MAGNETIC_FIELD].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
		        data->sensors[ID_MAGNETIC_FIELD].type = SENSOR_TYPE_MAGNETIC_FIELD;
		    }
		    
		    if (sscanf(output3, "gyroscope:%d:%d:%d", output_value+6, output_value+7, output_value+8) == 3) {
		        new_sensors |= SENSORS_GYROSCOPE;
		        data->sensors[ID_GYROSCOPE].gyro.x = output_value[6];
		        data->sensors[ID_GYROSCOPE].gyro.y = output_value[7];
		        data->sensors[ID_GYROSCOPE].gyro.z = output_value[8];
		        data->sensors[ID_GYROSCOPE].gyro.status = SENSOR_STATUS_ACCURACY_HIGH;
		        data->sensors[ID_GYROSCOPE].type = SENSOR_TYPE_MAGNETIC_FIELD;
		    }
		    pick_sensor(data, values);
		    break;
		}
		/****************************************************************/
        /*
        if (sscanf(buff, "acceleration:%g:%g:%g", params+0, params+1, params+2) == 3) {
            new_sensors |= SENSORS_ACCELERATION;
            data->sensors[ID_ACCELERATION].acceleration.x = params[0];
            data->sensors[ID_ACCELERATION].acceleration.y = params[1];
            data->sensors[ID_ACCELERATION].acceleration.z = params[2];
            data->sensors[ID_ACCELERATION].type = SENSOR_TYPE_ACCELEROMETER;
            continue;
        }
        if (sscanf(buff, "magnetic:%g:%g:%g", params+0, params+1, params+2) == 3) {
            new_sensors |= SENSORS_MAGNETIC_FIELD;
            data->sensors[ID_MAGNETIC_FIELD].magnetic.x = params[0];
            data->sensors[ID_MAGNETIC_FIELD].magnetic.y = params[1];
            data->sensors[ID_MAGNETIC_FIELD].magnetic.z = params[2];
            data->sensors[ID_MAGNETIC_FIELD].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
            data->sensors[ID_ACCELERATION].type = SENSOR_TYPE_MAGNETIC_FIELD;
            continue;
        }
        */
        /* "sync:<time>" is sent after a series of sensor events.
         * where 'time' is expressed in micro-seconds and corresponds
         * to the VM time when the real poll occured.
         */
         /*
        if (sscanf(buff, "sync:%lld", &event_time) == 1) {
            if (new_sensors) {
                data->pendingSensors = new_sensors;
                int64_t t = event_time * 1000LL;  /* convert to nano-seconds */
                /* use the time at the first sync: as the base for later
                 * time values *//*
                if (data->timeStart == 0) {
                    data->timeStart  = data__now_ns();
                    data->timeOffset = data->timeStart - t;
                }
                t += data->timeOffset;
                while (new_sensors) {
                    uint32_t i = 31 - __builtin_clz(new_sensors);
                    new_sensors &= ~(1<<i);
                    data->sensors[i].timestamp = t;
                }
                return pick_sensor(data, values);
            } else {
                D("huh ? sync without any sensor data ?");
            }
            continue;
        }
        */
        D("huh ? unsupported command");
    }
    
	close(file_name);
    return -1;
}
static int
data__close(struct hw_device_t *dev)
{
    SensorPoll* data = (SensorPoll*)dev;
    if (data) {
        if (data->events_fd >= 0) {
            //ALOGD("(device close) about to close fd=%d", data->events_fd);
            close(data->events_fd);
        }
        free(data);
    }
    return 0;
}
/** SENSORS POLL DEVICE FUNCTIONS **/
static int poll__close(struct hw_device_t* dev)
{
    SensorPoll*  ctl = (void*)dev;
    close(ctl->fd);
    if (ctl->fd >= 0) {
        close(ctl->fd);
    }
    if (ctl->events_fd >= 0) {
        close(ctl->events_fd);
    }
    free(ctl);
    return 0;
}
static int poll__poll(struct sensors_poll_device_1 *dev,
            sensors_event_t* data, int count)
{
    SensorPoll*  datadev = (void*)dev;
    int ret;
    int i;
    D("%s: dev=%p data=%p count=%d ", __FUNCTION__, dev, data, count);
	
	//ret = data__poll(dev, data);

	
    for (i = 0; i < count; i++)  {
        ret = data__poll(dev, data);
        data++;
        if (ret > MAX_NUM_SENSORS || ret < 0) {
           return i;
        }
        if (!datadev->pendingSensors) {
           return i + 1;
        }
    }
    
    return count;
}
static int poll__activate(struct sensors_poll_device_1 *dev,
            int handle, int enabled)
{
    int ret;
    native_handle_t* hdl;
    SensorPoll*  ctl = (void*)dev;
    D("oscar write 5 %s: dev=%p handle=%x enable=%d ", __FUNCTION__, dev, handle, enabled);
    if (ctl->fd < 0) {
        D("%s: OPEN CTRL and DATA ", __FUNCTION__);
        hdl = control__open_data_source(dev);
        ret = data__data_open(dev,hdl);
    }
    ret = control__activate(dev, handle, enabled);
    return ret;
}
static int poll__setDelay(struct sensors_poll_device_1 *dev,
            int handle, int64_t ns)
{
    // TODO
	D("%s: dev=%p handle=%x ", __FUNCTION__, dev, handle);
	ns = 1;
    return 0;
}
/** MODULE REGISTRATION SUPPORT
 **
 ** This is required so that hardware/libhardware/hardware.c
 ** will dlopen() this library appropriately.
 **/
/*
 * the following is the list of all supported sensors.
 * this table is used to build sSensorList declared below
 * according to which hardware sensors are reported as
 * available from the emulator (see get_sensors_list below)
 *
 * note: numerical values for maxRange/resolution/power were
 *       taken from the reference AK8976A implementation
 */
static const struct sensor_t sSensorListInit[] = {
        { .name       = "Goldfish 3-axis Accelerometer",
          .vendor     = "The Android Open Source Project",
          .version    = 1,
          .handle     = ID_ACCELERATION,
          .type       = SENSOR_TYPE_ACCELEROMETER,
          .maxRange   = 2.8f,
          .resolution = 1.0f/4032.0f,
          .power      = 3.0f,
          .reserved   = {}
        },
        { .name       = "Goldfish 3-axis Magnetic field sensor",
          .vendor     = "The Android Open Source Project",
          .version    = 1,
          .handle     = ID_MAGNETIC_FIELD,
          .type       = SENSOR_TYPE_MAGNETIC_FIELD,
          .maxRange   = 2000.0f,
          .resolution = 1.0f,
          .power      = 6.7f,
          .reserved   = {}
        },
	{ .name       = "Goldfish Gyroscope sensor",
          .vendor     = "The Android Open Source Project",
          .version    = 1,
          .handle     = ID_GYROSCOPE,
          .type       = SENSOR_TYPE_GYROSCOPE,
          .maxRange   = 1.0f,
          .resolution = 1.0f,
          .power      = 20.0f,
          .reserved   = {}
        },        

};
static struct sensor_t  sSensorList[MAX_NUM_SENSORS];
static int sensors__get_sensors_list(struct sensors_module_t* module,
        struct sensor_t const** list)
{
    int  fd = qemud_channel_open(SENSORS_SERVICE_NAME);
    char buffer[12];
    int  mask, nn, count;
    int  ret;
    if (fd < 0) {
        E("%s: no qemud connection", __FUNCTION__);
        return 0;
    }
    ret = qemud_channel_send(fd, "list-sensors", -1);
    if (ret < 0) {
        E("%s: could not query sensor list: %s", __FUNCTION__,
          strerror(errno));
        close(fd);
        return 0;
    }
    ret = qemud_channel_recv(fd, buffer, sizeof buffer-1);
    if (ret < 0) {
        E("%s: could not receive sensor list: %s", __FUNCTION__,
          strerror(errno));
        close(fd);
        return 0;
    }
    buffer[ret] = 0;
    close(fd);
    /* the result is a integer used as a mask for available sensors */
    mask  = atoi(buffer);
    count = 0;
    for (nn = 0; nn < MAX_NUM_SENSORS; nn++) {
        if (((1 << nn) & mask) == 0)
            continue;
        sSensorList[count++] = sSensorListInit[nn];
    }
    D("%s: returned %d sensors (mask=%d)", __FUNCTION__, count, mask);
    *list = sSensorList;
    return count;
}
static int
open_sensors(const struct hw_module_t* module,
             const char*               name,
             struct hw_device_t*      *device)
{
    int  status = -EINVAL;
    D("%s: name=%s", __FUNCTION__, name);
    if (!strcmp(name, SENSORS_HARDWARE_POLL)) {
        SensorPoll *dev = malloc(sizeof(*dev));
        memset(dev, 0, sizeof(*dev));
        dev->device.common.tag     = HARDWARE_DEVICE_TAG;
        dev->device.common.version = SENSORS_DEVICE_API_VERSION_1_0;
        dev->device.common.module  = (struct hw_module_t*) module;
        dev->device.common.close   = poll__close;
        dev->device.poll           = poll__poll;
        dev->device.activate       = poll__activate;
        dev->device.setDelay       = poll__setDelay;
        dev->events_fd             = -1;
        dev->fd                    = -1;
        *device = &dev->device.common;
        status  = 0;
    }
    return status;
}
static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};
struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Goldfish SENSORS Module",
        .author = "The Android Open Source Project",
        .methods = &sensors_module_methods,
    },
    .get_sensors_list = sensors__get_sensors_list
};
