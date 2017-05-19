/* arch/arm/mach-goldfish/goldfish_sensor.c
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/


/*********************************************************************/
/*
 *		written by Hongyu ZHOU
 *			16242950
 *
/*********************************************************************/
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#include <asm/types.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#ifdef	CONFIG_X86
#include <asm/mtrr.h>
#endif
#ifdef CONFIG_ARM
#include <mach/hardware.h>
#endif


#include <linux/delay.h>
MODULE_AUTHOR("Google, Inc.");
MODULE_DESCRIPTION("Android QEMU Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

struct goldfish_sensor {
	char __iomem *reg_base;  
	int irq;
	spinlock_t lock;
	wait_queue_head_t wait;

	unsigned long buffer_phys;
	
	char __iomem *read_buffer;      /* read buffer virtual address */
	int __iomem write_buffer;
	int buffer_status;

};


#define GOLDFISH_SENSOR_READ(data, addr)   (readl(data->reg_base + addr))
#define GOLDFISH_SENSOR_WRITE(data, addr, x)   (writel(x, data->reg_base + addr))

#define READ_BUFFER_SIZE 256 //
#define WRITE_BUFFER_SIZE 4 // 
#define COMBINED_BUFFER_SIZE (READ_BUFFER_SIZE + WRITE_BUFFER_SIZE)

/* temporary variable used between goldfish_sensor_probe() and goldfish_sensor_open() */
static struct goldfish_sensor* sensor_data;

enum {
	/* sensor status register */
	/* A write to this register enables interrupts for each of the sensors */
	INT_ENABLE 		= 0x00,
	
	/* These registers return the X,Y and Z values for the accelerometer */
	ACCEL_X			= 0x04,
	ACCEL_Y			= 0x08,
	ACCEL_Z			= 0x0c,

	/* These registers return the X,Y and Z values for the magnetometer */
	COMPASS_X		= 0x10,
	COMPASS_Y		= 0x14,
	COMPASS_Z		= 0x18,

	/* These registers return the X,Y and Z values for the gyroscope */
	GYRO_X			= 0x1c,
	GYRO_Y			= 0x20,
	GYRO_Z			= 0x24,
	
	/* this bit set when it is successful to read data from device */
	SENSOR_IRQ_ACCEL	= 1U << 0,
	SENSOR_IRQ_COMPASS	= 1U << 1,
	SENSOR_IRQ_GYRO      	= 1U << 2,
	
	SENSOR_MASK             = SENSOR_IRQ_ACCEL | 
                  		SENSOR_IRQ_COMPASS | 
                  		SENSOR_IRQ_GYRO,
};


static atomic_t open_count = ATOMIC_INIT(0);


static ssize_t goldfish_sensor_read(struct file *fp, char __user *buf,
							 loff_t *pos)
{
	struct goldfish_sensor* data = fp->private_data;
	int length;
	int result = 0;
	
	char temp_buffer[256];
	char temp_buffer1[256] = {0};
	char temp_buffer2[256] = {0};
	char temp_buffer3[256] = {0};
	msleep(1000);
	
	int* temp = kmalloc(sizeof(char), GFP_KERNEL);
	if(data->buffer_status != 0)
	{
		if((data->buffer_status & SENSOR_IRQ_ACCEL) != 0)
		{
			temp[0] = GOLDFISH_SENSOR_READ(data, ACCEL_X);
			temp[1] = GOLDFISH_SENSOR_READ(data, ACCEL_Y);
			temp[2] = GOLDFISH_SENSOR_READ(data, ACCEL_Z);
			int length = sprintf(temp_buffer1, "acceleration:%d:%d:%d ", temp[0], temp[1], temp[2]);
		}
		if((data->buffer_status & SENSOR_IRQ_COMPASS) != 0)
		{
			temp[3] = GOLDFISH_SENSOR_READ(data, COMPASS_X);
			temp[4] = GOLDFISH_SENSOR_READ(data, COMPASS_Y);
			temp[5] = GOLDFISH_SENSOR_READ(data, COMPASS_Z);
			int length = sprintf(temp_buffer2, "magnetic:%d:%d:%d ", temp[3], temp[4], temp[5]);
		}
		if((data->buffer_status & SENSOR_IRQ_GYRO) != 0)
		{
			temp[6] = GOLDFISH_SENSOR_READ(data, GYRO_X);
			temp[7] = GOLDFISH_SENSOR_READ(data, GYRO_Y);
			temp[8] = GOLDFISH_SENSOR_READ(data, GYRO_Z);
			int length = sprintf(temp_buffer3, "gyroscope:%d:%d:%d ", temp[6], temp[7], temp[8]);
		}
		int length = sprintf(temp_buffer, "%s%s%s", temp_buffer1, temp_buffer2, temp_buffer3);
		memcpy(data->read_buffer, temp_buffer, READ_BUFFER_SIZE);	//copy value from register to sensor_data

	} 
	else
	{	
		memcpy(data->read_buffer, 0, READ_BUFFER_SIZE);
	}
	wait_event_interruptible(data->wait, (data->buffer_status & SENSOR_MASK));  // turn irq
	result += READ_BUFFER_SIZE;	// Make sure that the result is READ_BUFFER_SIZE, otherwise release will be activited after this function

	
	/* copy data to user space */
	if (copy_to_user(buf, data->read_buffer, READ_BUFFER_SIZE))
	{
		printk("copy_from_user failed!\n");
		return -EFAULT;
	}
	kfree(temp);  //free memory
	return result;
}

static ssize_t goldfish_sensor_write(struct file *fp, const char __user *buf,
							  loff_t *pos)
{
	struct goldfish_sensor* data = fp->private_data;
	unsigned long irq_flags;
	int result = 0;

	/* copy from user space to the appropriate buffer */
	spin_lock_irqsave(&data->lock, irq_flags);
	

	if (copy_from_user(&(data->buffer_status), buf, WRITE_BUFFER_SIZE))
	{
		printk("copy_from_user failed!\n");
		result = -EFAULT;
	}
	spin_unlock_irqrestore(&data->lock, irq_flags);
	result += WRITE_BUFFER_SIZE;
	return result;
}

static int goldfish_sensor_open(struct inode *ip, struct file *fp)
{
	if (!sensor_data)
	{
		return -ENODEV;
	}
	
	if (atomic_inc_return(&open_count) == 1) 
	{
		fp->private_data = sensor_data;
		GOLDFISH_SENSOR_WRITE(sensor_data, INT_ENABLE, SENSOR_MASK);
		return 0;
	} 
	else 
	{
		atomic_dec(&open_count);
		return -EBUSY;
	}
}

static int goldfish_sensor_release(struct inode *ip, struct file* fp)
{
	atomic_dec(&open_count);
	GOLDFISH_SENSOR_WRITE(sensor_data, INT_ENABLE, 0);
	return 0;
}
	   
static long goldfish_sensor_ioctl(struct file* fp, unsigned int cmd, unsigned long arg)
{
	/* temporary workaround, until we switch to the ALSA API */
	if (cmd == 315)
		return -1;
	else
		return 0;
}

static irqreturn_t goldfish_sensor_interrupt(int irq, void *dev_id)
{
	unsigned long irq_flags;
	struct goldfish_sensor	*data = dev_id;
	uint32_t status;

	spin_lock_irqsave(&data->lock, irq_flags);	
	/* read buffer status flags */
	status = GOLDFISH_SENSOR_READ(data, INT_ENABLE);
	//the buffer_status is determined by userspace, even if the the device offer the data of certain sensor which is disabled by userspace, the kernel won't response.
	status &= data->buffer_status; 	

	/* if buffers are newly empty, wake up blocked goldfish_sensor_write() call */
	if(status) {//check status
		wake_up(&data->wait);
	}
	
	spin_unlock_irqrestore(&data->lock, irq_flags);
	return status ? IRQ_HANDLED : IRQ_NONE;
}

/* file operations for /dev/goldfish_sensor */
static struct file_operations goldfish_sensor_fops = {
	.owner = THIS_MODULE,
	.read = goldfish_sensor_read,
	.write = goldfish_sensor_write,
	.open = goldfish_sensor_open,
	.release = goldfish_sensor_release,
	.unlocked_ioctl = goldfish_sensor_ioctl,

};
	
static struct miscdevice goldfish_sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "goldfish_sensor",
	.fops = &goldfish_sensor_fops,
};

static int goldfish_sensor_probe(struct platform_device *pdev)
{

	int ret;
	struct resource *r;
	struct goldfish_sensor *data;
	dma_addr_t buf_addr;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if(data == NULL) {
		ret = -ENOMEM;
		goto err_data_alloc_failed;
	}

	spin_lock_init(&data->lock);
	init_waitqueue_head(&data->wait);

	// get the resource of device
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(r == NULL) {
		printk("platform_get_resource failed\n");
		ret = -ENODEV;
		goto err_no_io_base;
	}

#if defined(CONFIG_ARM)//mapping the addresss
	data->reg_base = (char __iomem *)IO_ADDRESS(r->start - IO_START);
#elif defined(CONFIG_X86) || defined(CONFIG_MIPS)
	data->reg_base = ioremap(r->start, PAGE_SIZE);
#else
#error NOT SUPPORTED
#endif
	data->irq = platform_get_irq(pdev, 0);//get irq handler
	if(data->irq < 0) {
		printk("platform_get_irq failed\n");
		ret = -ENODEV;
		goto err_no_irq;
	}
	
#if defined(CONFIG_ARM)//allocate memory 
	data->read_buffer = dma_alloc_writecombine(&pdev->dev, READ_BUFFER_SIZE,
							&buf_addr, GFP_KERNEL);
#elif defined(CONFIG_X86) || defined(CONFIG_MIPS)
	data->read_buffer = dma_alloc_coherent(NULL, READ_BUFFER_SIZE,
								&buf_addr, GFP_KERNEL);
#else
#error NOT SUPPORTED
#endif
	if(data->read_buffer == 0) {
		ret = -ENOMEM;
		goto err_alloc_write_buffer_failed;
	}

#ifdef CONFIG_X86
	mtrr_add(buf_addr, COMBINED_BUFFER_SIZE, MTRR_TYPE_WRBACK, 1);
#endif
	data->buffer_phys = buf_addr;
	ret = request_irq(data->irq, goldfish_sensor_interrupt, IRQF_SHARED, pdev->name, data);//set interrupt handler
	if(ret)
		goto err_request_irq_failed;

	if((ret = misc_register(&goldfish_sensor_device))) 
	{
		printk("misc_register returned %d in goldfish_sensor_init\n", ret);
		goto err_misc_register_failed;
	}

	platform_set_drvdata(pdev, data);
	//initialize the original buffer_status 111
	data->buffer_status = (SENSOR_IRQ_ACCEL | SENSOR_IRQ_COMPASS | SENSOR_IRQ_GYRO);  
	GOLDFISH_SENSOR_WRITE(data, INT_ENABLE, SENSOR_MASK);
	sensor_data = data;
	
	return 0;

err_misc_register_failed:
err_request_irq_failed:
#if defined(CONFIG_ARM)
	dma_free_writecombine(&pdev->dev, READ_BUFFER_SIZE, data->read_buffer, data->buffer_phys);
err_alloc_write_buffer_failed:
err_no_irq:
#elif defined(CONFIG_X86) || defined(CONFIG_MIPS)
	dma_free_coherent(NULL, READ_BUFFER_SIZE, data->read_buffer, data->buffer_phys);
err_alloc_write_buffer_failed:
err_no_irq:
	iounmap(data->reg_base);
#else
#error NOT SUPPORTED
#endif
err_no_io_base:
	kfree(data);
err_data_alloc_failed:

	return ret;
}

static int goldfish_sensor_remove(struct platform_device *pdev)
{
	struct goldfish_sensor *data = platform_get_drvdata(pdev);

	misc_deregister(&goldfish_sensor_device);
	free_irq(data->irq, data);

	kfree(data);
	sensor_data = NULL;
	return 0;
}

static struct platform_driver goldfish_sensor_driver = {
	.probe		= goldfish_sensor_probe,
	.remove		= goldfish_sensor_remove,
	.driver = {
		.name = "goldfish_sensor"
	}
};

static int __init goldfish_sensor_init(void)
{
	int ret;

	ret = platform_driver_register(&goldfish_sensor_driver);
	if (ret < 0)
	{
		printk("platform_driver_register returned %d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit goldfish_sensor_exit(void)
{
	int ret = 0;
	platform_driver_unregister(&goldfish_sensor_driver);
}

module_init(goldfish_sensor_init);
module_exit(goldfish_sensor_exit);
