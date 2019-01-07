/*
 * Adapted from Amazon's PCIM driver, which has the following notice:
 * See license.txt for details.
 *  __init: done, __exit: done
 *  fops: open: done, read: done,  write: unsupported, ioctl: done
 * */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>

#include <linux/slab.h>

#include "../../device.h"
#include "bsg_dma_driver.h"

MODULE_DESCRIPTION("Head/Tail DMA Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define DOMAIN 0
#define BUS 0
#define FUNCTION 0
#define OCL_BAR 0

static int slot = 0x0f; // needs to be changed depending on instance? 
module_param(slot, int, 0);
MODULE_PARM_DESC(slot, "The Slot Index of the F1 Card");

static struct cdev *kernel_cdev;
static dev_t dev_no;

int dma_open(struct inode *inode, struct file *flip);
int dma_release(struct inode *inode, struct file *flip);
long dma_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param);
ssize_t dma_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

struct file_operations dma_fops = {
 .read =           dma_read,
 // .write =          dma_write, (writes not exposed to user)
 .unlocked_ioctl = dma_ioctl,
 .open =           dma_open,
 .release =        dma_release
};

int dma_major = 0;
struct pci_dev *dma_dev;
unsigned char *dma_buffer; /* DMA buffer */
unsigned char *phys_dma_buffer;

void __iomem *ocl_base; /* config space */

/* Helper functions for reading from/writing to config space. */
static void poke_ocl(unsigned int offset, unsigned int data) {
  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
  *phy_addr = data;
}
//static unsigned int peek_ocl(unsigned int offset) {
//  unsigned int *phy_addr = (unsigned int *)(ocl_base + offset);
//  return *phy_addr;
//}
//
static int __init dma_init(void) {
  int result;

  printk(KERN_NOTICE "Installing BSG DMA module\n");

  dma_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));
  if (dma_dev == NULL) {
    printk(KERN_ALERT "BSG DMA driver: Unable to locate PCI card.\n");
    return -1;
  }

  printk(KERN_INFO "vendor: %x, device: %x\n", dma_dev->vendor, dma_dev->device);

  result = pci_enable_device(dma_dev);
  printk(KERN_INFO "Enable result: %x\n", result);

  result = pci_request_region(dma_dev, OCL_BAR, "OCL Region");
  if (result <0) {
    printk(KERN_ALERT "BSG DMA driver: Cannot obtain the OCL region.\n");
    return result;
  }

  ocl_base = (void __iomem *) pci_iomap(dma_dev, OCL_BAR, 0);   // BAR=0 (OCL), maxlen = 0 (map entire bar)


  result = alloc_chrdev_region(&dev_no, 0, 1, "bsg_dma_driver");   // get an assigned major device number

  if (result <0) {
    printk(KERN_ALERT "BSG DMA driver: Cannot obtain major number.\n");
    return result;
  }

  dma_major = MAJOR(dev_no);
  printk(KERN_INFO "BSG DMA driver; The major number is: %d\n", dma_major);

  kernel_cdev = cdev_alloc();
  kernel_cdev->ops = &dma_fops;
  kernel_cdev->owner = THIS_MODULE;

  result = cdev_add(kernel_cdev, dev_no, 1);

  if (result <0) {
    printk(KERN_ALERT "BSG DMA driver: Unable to add cdev.\n");
    return result;
  }

  dma_buffer = kmalloc(DMA_BUFFER_SIZE, GFP_DMA | GFP_USER);    // DMA buffer, do not swap memory
  phys_dma_buffer = (unsigned char *) virt_to_phys(dma_buffer);  // get the physical address for later

  return 0;
}

static void __exit dma_exit(void) {

  cdev_del(kernel_cdev);

  unregister_chrdev_region(dev_no, 1);

  if (dma_buffer != NULL)
    kfree(dma_buffer);
  

  if (dma_dev != NULL) {
    pci_iounmap(dma_dev, ocl_base);
    pci_disable_device(dma_dev);

    pci_release_region(dma_dev, OCL_BAR);

    pci_dev_put(dma_dev);                    // free device memory
  }

  printk(KERN_NOTICE "BSG DMA driver: Removing bsg_dma module\n");
}

module_init(dma_init);

module_exit(dma_exit);

int dma_open(struct inode *inode, struct file *filp) {
  printk(KERN_NOTICE "BSG DMA driver: opened. \n");
  return 0;
}

int dma_release(struct inode *inode, struct file *filp) {
  printk(KERN_NOTICE "BSG DMA driver: closed. \n");
  return 0;
}

/*
 * Copies data from DMA buffer to user's buf. Updates f_pos (head) and updates device's head register. 
 * in the future:
 * 	checks on buf
 * */
ssize_t dma_read(struct file *filp, char __user *buf, size_t pop_size, loff_t *f_pos) {
	uint32_t tail = dma_buffer[DMA_BUFFER_SIZE];
	
	uint32_t head = *f_pos; 
	
	bool can_read;
	uint32_t unused, num_cpy;
	unsigned long result;
	if (tail >= head)
		unused = tail - head;
	else
		unused = tail - head + DMA_BUFFER_SIZE;
	can_read = unused >= pop_size;

	if (!can_read) {
		printk(KERN_NOTICE "bsg dma driver: can't read %zd bytes because (Head, Tail) = (%lld, %u);\n only %u bytes available.\n",  pop_size, *f_pos, tail, unused); 
		return pop_size;
	}
	/* there is enough unread data; first, read data that lies before the end of system memory buffer */
	num_cpy = (DMA_BUFFER_SIZE - head >= pop_size) ? pop_size : DMA_BUFFER_SIZE - head; 
	result = copy_to_user(buf + head, dma_buffer + head, num_cpy);
    if (result)	{
		printk(KERN_INFO "BSG DMA driver: Could not copy %ld bytes\n", result);
		return pop_size;
	}
	head = (head + num_cpy) % DMA_BUFFER_SIZE;
	num_cpy = pop_size - num_cpy;  /* data that wraps over the end of system memory buffer */
	if (num_cpy > 0) { /* if there is still data to read */
		result = copy_to_user(buf, dma_buffer, num_cpy);
	    if (result)	{
			printk(KERN_INFO "Could not copy %ld bytes\n", result);
			return pop_size;
		}
		head = head + num_cpy; 	
	}

   	poke_ocl(CROSSBAR_M1 + WR_HEAD, head); /* update head register on device */
	*f_pos = head; /* update driver copy of head */	
	return 0;
}

long dma_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param) {
	int i;
	uint32_t val = (uint32_t) ioctl_param;
	switch (ioctl_num) {
		case (IOCTL_WR_ADDR_HIGH):
			val = (uint32_t) ((((uint64_t) phys_dma_buffer) & 0xffffffff00000000) >> 32);
			poke_ocl(CROSSBAR_M1 + WR_ADDR_HIGH, val);
			break;
		case (IOCTL_WR_ADDR_LOW):
			val = (uint32_t) (((unsigned long) phys_dma_buffer) & 0x00000000ffffffff);
			poke_ocl(CROSSBAR_M1 + WR_ADDR_LOW, val);
			break;
		case (IOCTL_WR_HEAD):
			poke_ocl(CROSSBAR_M1 + WR_HEAD, val);
			break;
		case (IOCTL_WR_LEN):
			poke_ocl(CROSSBAR_M1 + WR_LEN, val);
			break;
		case (IOCTL_WR_BUF_SIZE):
			poke_ocl(CROSSBAR_M1 + WR_BUF_SIZE, DMA_BUFFER_SIZE);
			break;
		case (IOCTL_CFG):
			poke_ocl(CROSSBAR_M1 + CFG_REG, val);
			break;
		case (IOCTL_CNTL):
			poke_ocl(CROSSBAR_M1 + CNTL_REG, val);
			break;
		case (IOCTL_TAIL):
			if (copy_to_user((void *)ioctl_param, dma_buffer + DMA_BUFFER_SIZE, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_CLEAR_BUFFER):
			for(i = 0; i < DMA_BUFFER_SIZE + 64; i++) 
				dma_buffer[i] = 0;
			break;
		default:
			return -EINVAL;			
	}
	return 0;
}
