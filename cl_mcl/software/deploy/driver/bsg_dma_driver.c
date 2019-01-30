/*
 * Adapted from Amazon's PCIM driver; see license.txt. 
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
#define DDR_BAR 3

#define USE_FPGA
// #define DEBUG

static int slot = 0x1d; // needs to be changed depending on instance? 
module_param(slot, int, 0);
MODULE_PARM_DESC(slot, "The Slot Index of the F1 Card");

static struct cdev *kernel_cdev;
static dev_t dev_no;

int dma_open(struct inode *inode, struct file *flip);
int dma_release(struct inode *inode, struct file *flip);
long dma_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param);
int dma_mmap (struct file *filp, struct vm_area_struct *vma);
ssize_t dma_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

struct file_operations dma_fops = {
	.read =		   dma_read,
	.mmap = dma_mmap,
	.unlocked_ioctl = dma_ioctl,
	.open =		   dma_open,	
	.release =		   dma_release
};

int dma_major = 0;
struct pci_dev *dma_dev;
unsigned char *dma_buffer; /* DMA buffer */
unsigned char *phys_dma_buffer;
unsigned int head;

void __iomem *ocl_base; /* config space */

/* Helper functions for reading from/writing to config space. */
static void poke_ocl(unsigned int offset, unsigned int data) {
	unsigned int *phy_addr;
	#ifdef DEBUG
	printk("BSG DMA driver: poke() called with data = %d\n", data);
	#endif
	phy_addr = (unsigned int *)(ocl_base + offset);
	*phy_addr = data;
	//iowrite32(data, ocl_base + offset);
}

static unsigned int peek_ocl(unsigned int offset) {
	unsigned int *phy_addr; 
	#ifdef DEBUG
	printk("BSG DMA driver: peek() called with offset %x\n", offset);
	#endif
	phy_addr = (unsigned int *)(ocl_base + offset);
	return *phy_addr;
	//return ioread32(ocl_base + offset);
}

static int __init dma_init(void) {
	int result;
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG DMA driver: Initializing.\n");
	#endif

	dma_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));
  	
	if (dma_dev == NULL) {
		printk(KERN_ALERT "BSG DMA driver: Unable to locate PCI card.\n");
		return -1;
  	}

	#ifdef DEBUG
	printk(KERN_INFO "BSG DMA driver: vendor: %x, device: %x\n", dma_dev->vendor, dma_dev->device);
	#endif

  	result = pci_enable_device(dma_dev);
	#ifdef DEBUG
	printk(KERN_INFO "BSG DMA driver: Enable result: %x\n", result);
	#endif

 	result = pci_request_region(dma_dev, DDR_BAR, "DDR Region");
  	if (result < 0) {
		printk(KERN_ALERT "BSG DMA driver: Cannot obtain the DDR region.\n");
		return result;
	}
	
 	result = pci_request_region(dma_dev, OCL_BAR, "OCL Region");
  	if (result < 0) {
		printk(KERN_ALERT "BSG DMA driver: Cannot obtain the OCL region.\n");
		return result;
	}
	
	ocl_base = (void __iomem *) pci_iomap(dma_dev, OCL_BAR, 0);	// BAR=0 (OCL), maxlen = 0 (map entire bar)

	result = alloc_chrdev_region(&dev_no, 0, 1, "bsg_dma_driver");   // get an assigned major device number
	
	if (result <0) {
		printk(KERN_ALERT "BSG DMA driver: Cannot obtain major number.\n");
		return result;
  	}

	dma_major = MAJOR(dev_no);
	printk(KERN_INFO "BSG DMA driver: The major number is: %d\n", dma_major);
	
	kernel_cdev = cdev_alloc();
	kernel_cdev->ops = &dma_fops;
	kernel_cdev->owner = THIS_MODULE;
	
	result = cdev_add(kernel_cdev, dev_no, 1);
	
	if (result <0) {
	  printk(KERN_ALERT "BSG DMA driver: Unable to add cdev.\n");
	  return result;
	}
	
	dma_buffer = kmalloc(DMA_BUFFER_SIZE + 64, GFP_DMA | GFP_USER);	  // DMA buffer, do not swap memory
	phys_dma_buffer = (unsigned char *) virt_to_phys(dma_buffer);  // get the physical address for later
	
	#ifdef DEBUG
		printk(KERN_INFO "BSG DMA driver (debug): Buffer Physical Address: %p, Buffer Virtual Address: %p, Config Base Address: %p\n", phys_dma_buffer, dma_buffer, ocl_base);
	#endif	

	printk(KERN_INFO "BSG DMA driver: Initialization complete.\n");
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
		pci_release_region(dma_dev, DDR_BAR);
		pci_release_region(dma_dev, OCL_BAR);
		pci_dev_put(dma_dev);					 // free device memory
	}
	
	printk(KERN_NOTICE "BSG DMA driver: Removing bsg_dma module\n");
}

module_init(dma_init);

module_exit(dma_exit);

int dma_open(struct inode *inode, struct file *filp) {
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG DMA driver: opened. \n");
	#endif
	return 0;
}

int dma_release(struct inode *inode, struct file *filp) {
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG DMA driver: closed. \n");
	#endif
	return 0;
}

/*
 * Copies data from DMA buffer to user's buf. Updates f_pos (head) and updates device's head register. 
 * in the future:
 *	checks on buf
 * */
ssize_t dma_read(struct file *filp, char __user *buf, size_t pop_size, loff_t *f_pos) {
	int ofs = (int) head;
	if (copy_to_user((void *) buf, (void *) &dma_buffer[ofs], pop_size) != 0)
		return 0;
	return pop_size;
}

int dma_mmap (struct file *filp, struct vm_area_struct *vma) {
	unsigned long mmap_size, offset, phys_start;
	int err;
	
	printk(KERN_ALERT "BSG DMA driver: in mmap() function.\n");	

	mmap_size = vma->vm_end - vma->vm_start;
	if (mmap_size > pci_resource_len(dma_dev, OCL_BAR)) {
		printk(KERN_ERR "BSG DMA driver: User tried to mmap %lu bytes, but OCL BAR is of size %lu.\n", mmap_size, (unsigned long) pci_resource_len(dma_dev, OCL_BAR));
		return(-EINVAL); /* asked for too much memory */
	}
	offset = vma->vm_pgoff << PAGE_SHIFT; 
	phys_start = ((unsigned long) pci_resource_start(dma_dev, OCL_BAR)) + offset;
	vma->vm_flags |= VM_LOCKED | VM_DONTEXPAND | VM_DONTDUMP | VM_IO;
	
	/* do the mapping */
	err = remap_pfn_range(vma, (unsigned long) vma->vm_start, phys_start >> PAGE_SHIFT, mmap_size, vma->vm_page_prot);
	if (err < 0) {
		printk(KERN_ERR "BSG DMA driver: remap_pfn_range() failed.\n");
		return(-EAGAIN);
	}
	printk(KERN_INFO "BSG DMA driver: mmap successful.\n");
	return 0;
}

long dma_ioctl(struct file *filp, unsigned int ioctl_num, unsigned long ioctl_param) {

	uint32_t val = (uint32_t) ioctl_param;
	int i;

	#ifdef DEBUG
	printk(KERN_INFO "BSG DMA Driver (debug): IOCTL parameter is %d.\n", val);
	switch (ioctl_num) {
		case (IOCTL_WR_ADDR_HIGH):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update high write address.\n");
			break;
		case (IOCTL_WR_ADDR_LOW):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update low write address.\n");
			break;
		case (IOCTL_WR_HEAD):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update write head.\n");
			break;
		case (IOCTL_WR_LEN):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update write length.\n");
			break;
		case (IOCTL_WR_BUF_SIZE):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update buffer size.\n");
			break;
		case (IOCTL_CFG):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update cfg reg.\n");
			break;
		case (IOCTL_CNTL):
			printk("BSG DMA Driver (Test 0): IOCTL call made to update cntl reg.\n");
			break;
		case (IOCTL_TAIL):
			printk("BSG DMA Driver (Test 0): IOCTL call made to read back the tail.\n");
			break;
		case (IOCTL_CLEAR_BUFFER):
			printk("BSG DMA Driver (Test 0): IOCTL call made to clear the buffer.\n");
			break;	
		default:
			printk("BSG DMA Driver Other test.\n");
	}
	#endif 

	switch (ioctl_num) {
		case (IOCTL_WR_ADDR_HIGH):
			poke_ocl(CROSSBAR_M1 + WR_ADDR_HIGH,   (unsigned int)((unsigned long)phys_dma_buffer >> 32l));
			break;
		case (IOCTL_WR_ADDR_LOW):
			poke_ocl(CROSSBAR_M1 + WR_ADDR_LOW,   ((unsigned int)(unsigned long)phys_dma_buffer & 0xffffffffl));
			break;
		case (IOCTL_WR_HEAD):
			head = val;
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
		case (IOCTL_READ_WR_ADDR_HIGH):
			val = peek_ocl(CROSSBAR_M1 + WR_ADDR_HIGH);
			printk(KERN_INFO "BSG DMA driver: WR ADDR HIGH is %d\n", val);
			if (copy_to_user((void *) ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_READ_WR_ADDR_LOW):
			val = peek_ocl(CROSSBAR_M1 + WR_ADDR_LOW);
			printk(KERN_INFO "BSG DMA driver: WR ADDR LOW is %d\n", val);
			if (copy_to_user((void *)ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_READ_WR_LEN):
			val = peek_ocl(CROSSBAR_M1 + WR_LEN);
			printk(KERN_INFO "BSG DMA driver: WR LEN is %d\n", val);
			if (copy_to_user((void *)ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_READ_WR_BUF_SIZE):
			val = peek_ocl(CROSSBAR_M1 + WR_BUF_SIZE);
			printk(KERN_INFO "BSG DMA driver: WR BUF SIZE is %d\n", val);
			if (copy_to_user((void *)ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_READ_CFG):
			val = peek_ocl(CROSSBAR_M1 + CFG_REG);
			if (copy_to_user((void *)ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
		case (IOCTL_READ_CNTL):
			val = peek_ocl(CROSSBAR_M1 + CNTL_REG);
			if (copy_to_user((void *)ioctl_param, (void *) &val, sizeof(uint32_t)) != 0)
				return -EFAULT;
			break;
                default: 
                        return -EINVAL;
	}
	return 0;
}
