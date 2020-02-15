#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define BUFFER_LEN (128 * sizeof(uint64_t))
#define STATE_BUFFER_LEN 16

#define DRIVER_AUTHOR "Nguyen Huynh Thao Nhi <trinitynguyen479@gmail.com>"
#define DRIVER_DESC "A sample loadable kernel module"

static char randomData[16]; // buffer of random data
static char *curRandomData; // pointer to current random data position
static int isDeviceOpen; 	// variable check if device is open ?


static int random_open(struct inode *i, struct file *f) 
{
	printk(KERN_INFO "Drive: open()\n");
	if (isDeviceOpen) {
		return -EBUSY;	
	}
	isDeviceOpen++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int random_release(struct inode *i, struct file *f) 
{
	isDeviceOpen--;
	module_put(THIS_MODULE);
	printk(KERN_INFO "Driver: close()\n");
	return 0;
}

int rand_state_p;
uint64_t rand_state[STATE_BUFFER_LEN];


static uint64_t random_prng(void) 
{
	uint64_t s0 = rand_state[rand_state_p];
	uint64_t s1 = rand_state[rand_state_p = (rand_state_p+1)&15];

	s1 ^= s1 << 31;
	s1 ^- s1 >> 11;
	s0 ^= s0 >> 30;
	return (rand_state[rand_state_p] = s0^s1) * 1181783497276652981ULL;
}



static void random_fill_data(char *curRandomData, size_t len) 
{
	uint64_t randomNumber;
	while (len) {
		randomNumber = random_prng();
		*((uint64_t *) curRandomData) = randomNumber;
		curRandomData += sizeof(uint64_t);
		len -= sizeof(uint64_t);
	}	
}

static void random_seed(void) {
	uint64_t i;
	for (i = 0; i < STATE_BUFFER_LEN; i++) {
		rand_state[i] = i+1;
	}

	rand_state_p = i+1;
}

static ssize_t random_read(struct file *f, char __user *buf, size_t len, loff_t *off) 
{
	printk(KERN_INFO "Driver: read()\n");
	ssize_t bytes_read = len;
	ssize_t bytes_to_copy = 0;

	while (len) {
		if (curRandomData == randomData + sizeof(randomData)) {
			random_fill_data(randomData, sizeof(randomData));
			curRandomData = randomData;
		}

		bytes_to_copy = min(len, sizeof(randomData) - (curRandomData - randomData));
		if (copy_to_user(buf, curRandomData, bytes_to_copy))
			return -EFAULT;

		curRandomData += bytes_to_copy;
		buf += bytes_to_copy;
		len -= bytes_to_copy;
	
	}
	return bytes_read;
}

static ssize_t random_write(struct file *f, const char __user *buf, size_t len, loff_t *off) 
{
	printk(KERN_INFO "Driver: write()\n");	
	return len;
}


static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = random_open,
	.release = random_release, 
	.read = random_read,
	.write = random_write
};

static struct miscdevice random_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "randomDevice",
	.fops = &fops,
	.mode = S_IRUGO,
};



static int __init random_init(void)
{       
	int ret = misc_register(&random_miscdev);
	if (ret) {
		pr_err("%s: misc_register failed: %d\n", "randomDevice", ret);
	}

	pr_info("%s: registered\n", "randomDevice");
	pr_info("%s: created device file /dev/%s\n", "randomDevice", "randomDevice");
	isDeviceOpen = 0;

	random_seed();
	random_fill_data(randomData, sizeof(randomData));
	curRandomData = randomData;

	return 0;
}       

static void __exit random_exit(void)
{       
	int ret;
	misc_deregister(&random_miscdev);
	//if (ret) {
	//	pr_err("%s: misc_deregister failed: %d\n", "randomDevice", ret);
	//} else {
	pr_info("%s: unregisted\n", "randomDevice");
	pr_info("%s: deleted /dev/%s\n", "randomDevice", "randomDevice");
	//}
}    

module_init(random_init);
module_exit(random_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("randomDevice");

