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


MODULE_DESCRIPTION("Linux 4 Driver to interface with Manycore on F1.");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define DOMAIN 0
#define BUS 0
#define FUNCTION 0
#define OCL_BAR 0

// #define DEBUG

static int slot = 0x1d; // needs to be changed depending on instance? 
module_param(slot, int, 0);
MODULE_PARM_DESC(slot, "The Slot Index of the F1 Card");

static struct cdev *kernel_cdev;
static dev_t dev_no;

int manycore_open(struct inode *inode, struct file *flip);
int manycore_release(struct inode *inode, struct file *flip);
int manycore_mmap (struct file *filp, struct vm_area_struct *vma);

struct file_operations manycore_fops = {
	.mmap = manycore_mmap,
	.open = manycore_open,	
	.release = manycore_release
};

int manycore_major = 0;
struct pci_dev *manycore_dev;

void __iomem *ocl_base; /* config space */

static int __init manycore_init(void) {
	int result;
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG Manycore driver: Initializing.\n");
	#endif

	manycore_dev = pci_get_domain_bus_and_slot(DOMAIN, BUS, PCI_DEVFN(slot,FUNCTION));
  	
	if (manycore_dev == NULL) {
		printk(KERN_ALERT "BSG Manycore driver: Unable to locate PCI card.\n");
		return -1;
  	}

	#ifdef DEBUG
	printk(KERN_INFO "BSG Manycore driver: vendor: %x, device: %x\n", manycore_dev->vendor, manycore_dev->device);
	#endif

  	result = pci_enable_device(manycore_dev);
	#ifdef DEBUG
	printk(KERN_INFO "BSG Manycore driver: Enable result: %x\n", result);
	#endif

 	result = pci_request_region(manycore_dev, OCL_BAR, "OCL Region");
  	if (result < 0) {
		printk(KERN_ALERT "BSG Manycore driver: Cannot obtain the OCL region.\n");
		return result;
	}
	
	ocl_base = (void __iomem *) pci_iomap(manycore_dev, OCL_BAR, 0);	// BAR=0 (OCL), maxlen = 0 (map entire bar)

	result = alloc_chrdev_region(&dev_no, 0, 1, "bsg_manycore_driver");   // get an assigned major device number
	
	if (result < 0) {
		printk(KERN_ALERT "BSG Manycore driver: Cannot obtain major number.\n");
		return result;
  	}

	manycore_major = MAJOR(dev_no);
	printk(KERN_INFO "BSG Manycore driver: The major number is: %d\n", manycore_major);
	
	kernel_cdev = cdev_alloc();
	kernel_cdev->ops = &manycore_fops;
	kernel_cdev->owner = THIS_MODULE;
	
	result = cdev_add(kernel_cdev, dev_no, 1);
	
	if (result <0) {
	  printk(KERN_ALERT "BSG DMA driver: Unable to add cdev.\n");
	  return result;
	}
	
	#ifdef DEBUG
		printk(KERN_INFO "BSG Manycore Driver: Config Base Address: %p\n", ocl_base);
	#endif	

	printk(KERN_INFO "BSG DMA driver: Initialization complete.\n");
	return 0;
}

static void __exit manycore_exit(void) {

	cdev_del(kernel_cdev);
	
	unregister_chrdev_region(dev_no, 1);


  
	if (manycore_dev != NULL) {
		pci_iounmap(manycore_dev, ocl_base);
		pci_disable_device(manycore_dev);
		pci_release_region(manycore_dev, OCL_BAR);
		pci_dev_put(manycore_dev);					 // free device memory
	}
	
	printk(KERN_NOTICE "BSG Manycore driver: Removing module\n");
}

module_init(manycore_init);

module_exit(manycore_exit);

int manycore_open(struct inode *inode, struct file *filp) {
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG Manycore driver: opened. \n");
	#endif
	return 0;
}

int manycore_release(struct inode *inode, struct file *filp) {
	#ifdef DEBUG
	printk(KERN_NOTICE "BSG Manycore driver: closed. \n");
	#endif
	return 0;
}

int manycore_mmap (struct file *filp, struct vm_area_struct *vma) {
	unsigned long mmap_size, offset, phys_start;
	int err;
	
	printk(KERN_ALERT "BSG Manycore driver: in mmap() function.\n");	

	mmap_size = vma->vm_end - vma->vm_start;
	if (mmap_size > pci_resource_len(manycore_dev, OCL_BAR)) {
		printk(KERN_ERR "BSG Manycore driver: User tried to mmap %lu bytes, but OCL BAR is of size %lu.\n", mmap_size, (unsigned long) pci_resource_len(manycore_dev, OCL_BAR));
		return(-EINVAL); /* asked for too much memory */
	}
	offset = vma->vm_pgoff << PAGE_SHIFT; 
	phys_start = ((unsigned long) pci_resource_start(manycore_dev, OCL_BAR)) + offset;
	vma->vm_flags |= VM_LOCKED | VM_DONTEXPAND | VM_DONTDUMP | VM_IO;
	
	/* do the mapping */
	err = remap_pfn_range(vma, (unsigned long) vma->vm_start, phys_start >> PAGE_SHIFT, mmap_size, vma->vm_page_prot);
	if (err < 0) {
		printk(KERN_ERR "BSG Manycore driver: remap_pfn_range() failed.\n");
		return(-EAGAIN);
	}
	printk(KERN_INFO "BSG Manycore driver: mmap successful.\n");
	return 0;
}
